#include "CppParser.h"


namespace CE
{
	void CppParser::AddFlag(EParsedTypeFlags& Flags, EParsedTypeFlags Flag)
	{
		Flags = static_cast<EParsedTypeFlags>(static_cast<uint8>(Flags) | static_cast<uint8>(Flag));
	}

	void CppParser::AddFlag(EParsedVariableFlags& Flags, EParsedVariableFlags Flag)
	{
		Flags = static_cast<EParsedVariableFlags>(static_cast<uint16>(Flags) | static_cast<uint16>(Flag));
	}

	void CppParser::AddFlag(EParsedFunctionFlags& Flags, EParsedFunctionFlags Flag)
	{
		Flags = static_cast<EParsedFunctionFlags>(static_cast<uint16>(Flags) | static_cast<uint16>(Flag));
	}

	bool CppParser::IsTypeQualifier(const String& Value)
	{
		return Value == TEXT("const") || Value == TEXT("volatile") || Value == TEXT("mutable") ||
			Value == TEXT("signed") || Value == TEXT("unsigned");
	}

	bool CppParser::IsDeclarationSpecifier(const String& Value)
	{
		return Value == TEXT("static") || Value == TEXT("thread_local") || Value == TEXT("extern") ||
			Value == TEXT("mutable") || Value == TEXT("constexpr") || Value == TEXT("consteval") ||
			Value == TEXT("inline") || Value == TEXT("virtual") || Value == TEXT("explicit") ||
			Value == TEXT("friend") || Value == TEXT("register");
	}

	bool CppParser::IsBuiltinType(const String& Value)
	{
		return Value == TEXT("void") || Value == TEXT("bool") || Value == TEXT("char") ||
			Value == TEXT("wchar_t") || Value == TEXT("char8_t") || Value == TEXT("char16_t") ||
			Value == TEXT("char32_t") || Value == TEXT("short") || Value == TEXT("int") ||
			Value == TEXT("long") || Value == TEXT("float") || Value == TEXT("double") ||
			Value == TEXT("auto") || Value == TEXT("__int8") || Value == TEXT("__int16") ||
			Value == TEXT("__int32") || Value == TEXT("__int64");
	}

	void CppParser::AppendTokenText(String& Text, const TextToken& Token)
	{
		if (Text.Size() > 0 && Token.Whitespaces.Size() > 0)
		{
			Text += TEXT(" ");
		}
		Text += Token.RawText.Size() > 0 ? Token.RawText : Token.Value_Text;
	}

	void CppParser::AddNameSegment(ParsedName& Name, const String& Value)
	{
		if (Value.Size() > 0)
		{
			Name.Segments.EmplaceRef().Name = Value;
		}
	}

	CppParser::CppParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer) : Preprocessor(Path, Tokenizer)
	{
		Tokenizer.Config.SymbolPairs.AddUnique({ '[', '[' });
		Tokenizer.Config.SymbolPairs.AddUnique({ ']', ']' });
		Tokenizer.Config.SymbolPairs.AddUnique({ ':', ':' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '-', '>' });
	}

	void CppParser::Advance(TextToken& Token, bool& HasToken)
	{
		HasToken = GetToken(Token);
	}

	bool CppParser::ConsumeEllipsis(TextToken& Token, bool& HasToken)
	{
		if (!HasToken) return false;
		if (Token.Value_Text == TEXT("..."))
		{
			Advance(Token, HasToken);
			return true;
		}
		if (Token.Value_Text != TEXT(".")) return false;
		Advance(Token, HasToken);
		if (!HasToken || Token.Value_Text != TEXT(".")) return false;
		Advance(Token, HasToken);
		if (!HasToken || Token.Value_Text != TEXT(".")) return false;
		Advance(Token, HasToken);
		return true;
	}

	void CppParser::Expect(const String& Value, TextToken& Token, bool& HasToken)
	{
		if (!HasToken || Token.Value_Text != Value)
		{
			ThrowError(TEXT("Expected '") + Value + TEXT("'"), Token, HasToken);
		}
		Advance(Token, HasToken);
	}

	void CppParser::ThrowError(const String& Message, const TextToken& Token, bool HasToken) const
	{
		if (HasToken)
		{
			throw TextTokenizerError(Message, Token, CurrentFile());
		}
		throw TextTokenizerError(Message, CurrentTokenizer(), 0, CurrentFile());
	}

	void CppParser::SkipBalanced(const String& Open, const String& Close, TextToken& Token, bool& HasToken, String* Text)
	{
		if (!HasToken || Token.Value_Text != Open)
		{
			ThrowError(TEXT("Expected '") + Open + TEXT("'"), Token, HasToken);
		}
		int32 Depth = 0;
		do
		{
			if (Token.Value_Text == Open) ++Depth;
			else if (Token.Value_Text == Close) --Depth;
			else if (Close == TEXT(">") && Token.Value_Text == TEXT(">>")) Depth -= 2;
			if (Text != nullptr) AppendTokenText(*Text, Token);
			Advance(Token, HasToken);
		} while (HasToken && Depth > 0);
		if (Depth != 0)
		{
			ThrowError(TEXT("Unterminated balanced token sequence"), Token, HasToken);
		}
	}

	void CppParser::ReadExpressionUntil(String& Text, const String& EndA, const String& EndB, TextToken& Token, bool& HasToken)
	{
		int32 Parens = 0;
		int32 Brackets = 0;
		int32 Braces = 0;
		while (HasToken)
		{
			const String Value = Token.Value_Text;
			if (Parens == 0 && Brackets == 0 && Braces == 0 && (Value == EndA || (EndB.Size() > 0 && Value == EndB)))
			{
				break;
			}
			if (Value == TEXT("(")) ++Parens;
			else if (Value == TEXT(")") && Parens > 0) --Parens;
			else if (Value == TEXT("[") || Value == TEXT("[[")) ++Brackets;
			else if ((Value == TEXT("]") || Value == TEXT("]]")) && Brackets > 0) --Brackets;
			else if (Value == TEXT("{")) ++Braces;
			else if (Value == TEXT("}") && Braces > 0) --Braces;
			AppendTokenText(Text, Token);
			Advance(Token, HasToken);
		}
		Text.Trim();
	}

	void CppParser::Parse()
	{
		OnParseBegin();
		m_Scopes.Clear();
		m_PendingAttributes.Clear();
		m_PendingDeclaredType = {};
		m_PendingVariableFlags = EParsedVariableFlags::None;

		TextToken Token;
		bool HasToken = GetToken(Token);
		while (HasToken)
		{
			if (Token.Value_Text == TEXT(";"))
			{
				Advance(Token, HasToken);
				continue;
			}
			if (Token.Value_Text == TEXT("}"))
			{
				if (m_Scopes.Size() == 0)
				{
					ThrowError(TEXT("Unexpected scope end"), Token, HasToken);
				}
				Scope ClosedScope = std::move(m_Scopes[m_Scopes.Size() - 1]);
				m_Scopes.RemoveAt(m_Scopes.Size() - 1);
				Advance(Token, HasToken);
				OnParsed_ScopeEnd();
				if (ClosedScope.Type == EScopeType::Class && HasToken && Token.Value_Text != TEXT(";") && Token.Value_Text != TEXT("}"))
				{
					ParseClosedClassDeclarators(ClosedScope, Token, HasToken);
				}
				continue;
			}
			ParseDeclaration(Token, HasToken);
		}

		if (m_Scopes.Size() != 0)
		{
			ThrowError(TEXT("Expected scope end"), Token, HasToken);
		}
		OnParseEnd();
	}

	void CppParser::ParseAttributes(Array<ParsedAttribute>& Attributes, TextToken& Token, bool& HasToken)
	{
		while (HasToken)
		{
			if (Token.Value_Text == TEXT("[["))
			{
				Advance(Token, HasToken);
				while (HasToken && Token.Value_Text != TEXT("]]"))
				{
					ParsedAttribute& Attribute = Attributes.EmplaceRef();
					Attribute.Kind = ParsedAttribute::EKind::Standard;
					ReadExpressionUntil(Attribute.Text, TEXT(","), TEXT("]]"), Token, HasToken);
					if (Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
				}
				Expect(TEXT("]]"), Token, HasToken);
				continue;
			}

			ParsedAttribute::EKind Kind;
			if (Token.Value_Text == TEXT("alignas")) Kind = ParsedAttribute::EKind::Alignas;
			else if (Token.Value_Text == TEXT("__declspec")) Kind = ParsedAttribute::EKind::Declspec;
			else if (Token.Value_Text == TEXT("__attribute__")) Kind = ParsedAttribute::EKind::Gnu;
			else break;

			Advance(Token, HasToken);
			if (!HasToken || Token.Value_Text != TEXT("(")) break;
			ParsedAttribute& Attribute = Attributes.EmplaceRef();
			Attribute.Kind = Kind;
			Advance(Token, HasToken);
			String Text;
			int32 Depth = 1;
			while (HasToken && Depth > 0)
			{
				if (Token.Value_Text == TEXT("(")) ++Depth;
				else if (Token.Value_Text == TEXT(")") && --Depth == 0)
				{
					Advance(Token, HasToken);
					break;
				}
				AppendTokenText(Text, Token);
				Advance(Token, HasToken);
			}
			Text.Trim();
			if (Kind == ParsedAttribute::EKind::Declspec)
			{
				AddNameSegment(Attribute.Name, Text);
			}
			else
			{
				Attribute.Arguments.EmplaceRef().Text = std::move(Text);
			}
		}
	}

	void CppParser::ParseDeclaration(TextToken& Token, bool& HasToken)
	{
		Array<ParsedAttribute> Attributes;
		ParseAttributes(Attributes, Token, HasToken);
		if (Attributes.Size() > 0) m_PendingAttributes = std::move(Attributes);
		if (!HasToken) return;

		if (Token.Value_Text == TEXT("inline"))
		{
			Advance(Token, HasToken);
			if (HasToken && Token.Value_Text == TEXT("namespace"))
			{
				Advance(Token, HasToken);
				ParseNamespace(true, Token, HasToken);
				return;
			}
			ParsedType Type;
			EParsedVariableFlags Flags = EParsedVariableFlags::IsInline;
			ParseGeneral(Token, HasToken, std::move(Type), Flags);
			return;
		}
		if (Token.Value_Text == TEXT("namespace"))
		{
			Advance(Token, HasToken);
			ParseNamespace(false, Token, HasToken);
		}
		else if (Token.Value_Text == TEXT("class") || Token.Value_Text == TEXT("struct") || Token.Value_Text == TEXT("union"))
		{
			const String Value = Token.Value_Text;
			Advance(Token, HasToken);
			ParseClass(Value == TEXT("class") ? EClassType::Class : Value == TEXT("struct") ? EClassType::Struct : EClassType::Union, false, Token, HasToken);
		}
		else if (Token.Value_Text == TEXT("friend"))
		{
			Advance(Token, HasToken);
			if (HasToken && (Token.Value_Text == TEXT("class") || Token.Value_Text == TEXT("struct") || Token.Value_Text == TEXT("union")))
			{
				const String Value = Token.Value_Text;
				Advance(Token, HasToken);
				ParseClass(Value == TEXT("class") ? EClassType::Class : Value == TEXT("struct") ? EClassType::Struct : EClassType::Union, true, Token, HasToken);
			}
			else ParseGeneral(Token, HasToken);
		}
		else if (Token.Value_Text == TEXT("enum"))
		{
			Advance(Token, HasToken);
			ParseEnum(Token, HasToken);
		}
		else if (Token.Value_Text == TEXT("public") || Token.Value_Text == TEXT("protected") || Token.Value_Text == TEXT("private"))
		{
			const String Value = Token.Value_Text;
			Advance(Token, HasToken);
			Expect(TEXT(":"), Token, HasToken);
			OnParsed_Access(Value == TEXT("public") ? EAccessSpecifier::Public : Value == TEXT("protected") ? EAccessSpecifier::Protected : EAccessSpecifier::Private);
		}
		else if (Token.Value_Text == TEXT("using")) ParseUsing(false, Token, HasToken);
		else if (Token.Value_Text == TEXT("typedef")) ParseUsing(true, Token, HasToken);
		else if (Token.Value_Text == TEXT("template")) ParseTemplate(Token, HasToken);
		else if (Token.Value_Text == TEXT("concept")) ParseConcept(Token, HasToken);
		else if (Token.Value_Text == TEXT("static_assert")) ParseStaticAssert(Token, HasToken);
		else if (Token.Value_Text == TEXT("extern"))
		{
			Advance(Token, HasToken);
			if (HasToken && Token.Type == ETextTokenType::Constant && Token.ConstantType == ETextTokenConstantType::Text) ParseLinkage(Token, HasToken);
			else
			{
				EParsedVariableFlags Flags = EParsedVariableFlags::IsExtern;
				ParseGeneral(Token, HasToken, {}, Flags);
			}
		}
		else ParseGeneral(Token, HasToken);
	}

	void CppParser::ParseNamespace(bool IsInline, TextToken& Token, bool& HasToken)
	{
		ParsedNamespace Namespace;
		Namespace.Attributes = std::move(m_PendingAttributes);
		ParseAttributes(Namespace.Attributes, Token, HasToken);
		bool First = true;
		while (HasToken && Token.Value_Text != TEXT("{") && Token.Value_Text != TEXT("="))
		{
			bool SegmentInline = false;
			if (Token.Value_Text == TEXT("inline"))
			{
				SegmentInline = true;
				Advance(Token, HasToken);
			}
			if (HasToken && Token.Type == ETextTokenType::Identifier)
			{
				ParsedNameSegment& Segment = Namespace.Name.Segments.EmplaceRef();
				Segment.Name = Token.Value_Text;
				Segment.IsInline = First ? IsInline : SegmentInline;
				First = false;
			}
			Advance(Token, HasToken);
		}
		if (HasToken && Token.Value_Text == TEXT("="))
		{
			Advance(Token, HasToken);
			ParsedNamespaceAlias Alias;
			Alias.Name = Namespace.Name;
			Alias.Attributes = Namespace.Attributes;
			while (HasToken && Token.Value_Text != TEXT(";"))
			{
				if (Token.Type == ETextTokenType::Identifier) AddNameSegment(Alias.Target, Token.Value_Text);
				else if (Token.Value_Text == TEXT("::") && Alias.Target.Segments.Size() == 0) Alias.Target.Segments.EmplaceRef();
				Advance(Token, HasToken);
			}
			Expect(TEXT(";"), Token, HasToken);
			OnParsed_NamespaceAlias(Alias);
			return;
		}
		Expect(TEXT("{"), Token, HasToken);
		OnParsed_Namespace(Namespace);
		m_Scopes.Add({ EScopeType::Namespace });
	}

	void CppParser::ParseClass(EClassType Type, bool IsFriend, TextToken& Token, bool& HasToken)
	{
		ParsedClass Class;
		Class.Type = Type;
		Class.IsFriend = IsFriend;
		Class.Attributes = std::move(m_PendingAttributes);
		ParseAttributes(Class.Attributes, Token, HasToken);
		if (HasToken && Token.Type == ETextTokenType::Identifier && Token.Value_Text != TEXT("final"))
		{
			AddNameSegment(Class.Name, Token.Value_Text);
			Advance(Token, HasToken);
			if (HasToken && Token.Value_Text == TEXT("<"))
			{
				Advance(Token, HasToken);
				bool Closed = false;
				while (HasToken && !Closed)
				{
					ParsedTemplateArgument& Argument = Class.Specialization.EmplaceRef();
					String Text;
					int32 Depth = 0;
					while (HasToken)
					{
						if (Token.Value_Text == TEXT(",") && Depth == 0) break;
						if (Token.Value_Text == TEXT(">") && Depth == 0)
						{
							Closed = true;
							break;
						}
						if (Token.Value_Text == TEXT(">>"))
						{
							if (Depth <= 1)
							{
								if (Depth == 1) Text += TEXT(">");
								Closed = true;
								break;
							}
							Depth -= 2;
						}
						else if (Token.Value_Text == TEXT("<")) ++Depth;
						else if (Token.Value_Text == TEXT(">") && Depth > 0) --Depth;
						AppendTokenText(Text, Token);
						Advance(Token, HasToken);
					}
					Argument.Kind = ParsedTemplateArgument::EKind::Type;
					AddNameSegment(Argument.Type.Name, Text);
					if (Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
					else if (Closed) Advance(Token, HasToken);
				}
			}
		}
		else Class.IsAnonymous = Type != EClassType::Union;
		ParseAttributes(Class.Attributes, Token, HasToken);
		if (HasToken && Token.Value_Text == TEXT("final"))
		{
			Class.IsFinal = true;
			Advance(Token, HasToken);
		}
		if (HasToken && Token.Value_Text == TEXT(":"))
		{
			Advance(Token, HasToken);
			while (HasToken && Token.Value_Text != TEXT("{") && Token.Value_Text != TEXT(";"))
			{
				ParsedBaseClass& Base = Class.BaseClasses.EmplaceRef();
				Base.AccessSpecifier = Type == EClassType::Class ? EAccessSpecifier::Private : EAccessSpecifier::Public;
				while (HasToken && Token.Value_Text != TEXT(",") && Token.Value_Text != TEXT("{"))
				{
					if (Token.Value_Text == TEXT("public")) Base.AccessSpecifier = EAccessSpecifier::Public;
					else if (Token.Value_Text == TEXT("protected")) Base.AccessSpecifier = EAccessSpecifier::Protected;
					else if (Token.Value_Text == TEXT("private")) Base.AccessSpecifier = EAccessSpecifier::Private;
					else if (Token.Value_Text == TEXT("virtual")) Base.IsVirtual = true;
					else if (Token.Type == ETextTokenType::Identifier) AddNameSegment(Base.Type.Name, Token.Value_Text);
					Advance(Token, HasToken);
				}
				if (HasToken && Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
			}
		}
		if (HasToken && Token.Value_Text == TEXT(";"))
		{
			Class.IsForward = true;
			Advance(Token, HasToken);
			OnParsed_Class(Class);
			return;
		}
		if (!HasToken || Token.Value_Text != TEXT("{"))
		{
			ParsedType Declared = std::move(m_PendingDeclaredType);
			Declared.ElaboratedType = Type == EClassType::Class ? EParsedElaboratedType::Class : Type == EClassType::Struct ? EParsedElaboratedType::Struct : EParsedElaboratedType::Union;
			Declared.Name = Class.Name;
			ParseGeneral(Token, HasToken, std::move(Declared), m_PendingVariableFlags);
			m_PendingVariableFlags = EParsedVariableFlags::None;
			return;
		}
		Advance(Token, HasToken);
		Class.HasBody = true;
		OnParsed_Class(Class);
		Scope ScopeData;
		ScopeData.Type = EScopeType::Class;
		ScopeData.ElaboratedType = Type == EClassType::Class ? EParsedElaboratedType::Class : Type == EClassType::Struct ? EParsedElaboratedType::Struct : EParsedElaboratedType::Union;
		ScopeData.DeclaredType = std::move(m_PendingDeclaredType);
		ScopeData.VariableFlags = m_PendingVariableFlags;
		m_PendingVariableFlags = EParsedVariableFlags::None;
		if (Class.Name.Segments.Size() > 0) ScopeData.Name = Class.Name.Segments[Class.Name.Segments.Size() - 1].Name;
		m_Scopes.Add(std::move(ScopeData));
	}

	void CppParser::ParseEnum(TextToken& Token, bool& HasToken)
	{
		ParsedEnum Enum;
		Enum.Attributes = std::move(m_PendingAttributes);
		if (HasToken && (Token.Value_Text == TEXT("class") || Token.Value_Text == TEXT("struct")))
		{
			Enum.IsScoped = true;
			Enum.IsStruct = Token.Value_Text == TEXT("struct");
			Advance(Token, HasToken);
		}
		ParseAttributes(Enum.Attributes, Token, HasToken);
		if (HasToken && Token.Type == ETextTokenType::Identifier)
		{
			AddNameSegment(Enum.Name, Token.Value_Text);
			Advance(Token, HasToken);
		}
		else Enum.IsAnonymous = true;
		ParseAttributes(Enum.Attributes, Token, HasToken);
		if (HasToken && Token.Value_Text == TEXT(":"))
		{
			Advance(Token, HasToken);
			while (HasToken && Token.Value_Text != TEXT("{") && Token.Value_Text != TEXT(";"))
			{
				if (Token.Type == ETextTokenType::Identifier) AddNameSegment(Enum.UnderlyingType.Name, Token.Value_Text);
				Advance(Token, HasToken);
			}
		}
		if (HasToken && Token.Value_Text == TEXT(";"))
		{
			Enum.IsForward = true;
			Advance(Token, HasToken);
			OnParsed_Enum(Enum);
			return;
		}
		if (!HasToken || Token.Value_Text != TEXT("{"))
		{
			ParsedType Type = std::move(m_PendingDeclaredType);
			Type.ElaboratedType = EParsedElaboratedType::Enum;
			Type.Name = Enum.Name;
			ParseGeneral(Token, HasToken, std::move(Type), m_PendingVariableFlags);
			m_PendingVariableFlags = EParsedVariableFlags::None;
			return;
		}
		Expect(TEXT("{"), Token, HasToken);
		OnParsed_Enum(Enum);
		while (HasToken && Token.Value_Text != TEXT("}"))
		{
			if (Token.Value_Text == TEXT(","))
			{
				Advance(Token, HasToken);
				continue;
			}
			if (Token.Type != ETextTokenType::Identifier) ThrowError(TEXT("Expected enum value"), Token, HasToken);
			ParsedEnumValue Value;
			Value.Name = Token.Value_Text;
			Advance(Token, HasToken);
			ParseAttributes(Value.Attributes, Token, HasToken);
			if (HasToken && Token.Value_Text == TEXT("="))
			{
				Value.HasValue = true;
				Advance(Token, HasToken);
				ReadExpressionUntil(Value.Value.Text, TEXT(","), TEXT("}"), Token, HasToken);
			}
			OnParsed_EnumValue(Value);
			if (HasToken && Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
		}
		Expect(TEXT("}"), Token, HasToken);
		OnParsed_ScopeEnd();
		if (HasToken && Token.Value_Text == TEXT(";"))
		{
			Advance(Token, HasToken);
			return;
		}
		ParsedType Type = std::move(m_PendingDeclaredType);
		Type.ElaboratedType = EParsedElaboratedType::Enum;
		Type.Name = Enum.Name;
		ParseGeneral(Token, HasToken, std::move(Type), m_PendingVariableFlags);
		m_PendingVariableFlags = EParsedVariableFlags::None;
	}

	void CppParser::ParseUsing(bool IsTypedef, TextToken& Token, bool& HasToken)
	{
		Advance(Token, HasToken);
		ParsedUsing Using;
		Using.Attributes = std::move(m_PendingAttributes);
		Using.Kind = IsTypedef ? ParsedUsing::EKind::Typedef : ParsedUsing::EKind::UsingDeclaration;
		if (IsTypedef && HasToken && (Token.Value_Text == TEXT("class") || Token.Value_Text == TEXT("struct") || Token.Value_Text == TEXT("union")))
		{
			String InlineDefinition = Token.Value_Text;
			Advance(Token, HasToken);
			ParseAttributes(Using.Attributes, Token, HasToken);
			if (HasToken && Token.Type == ETextTokenType::Identifier)
			{
				InlineDefinition += TEXT(" ") + Token.Value_Text;
				Advance(Token, HasToken);
			}
			ParseAttributes(Using.Attributes, Token, HasToken);
			if (HasToken && Token.Value_Text == TEXT("{"))
			{
				InlineDefinition += TEXT(" ");
				SkipBalanced(TEXT("{"), TEXT("}"), Token, HasToken, &InlineDefinition);
			}

			Array<ParsedUsing> Aliases;
			while (HasToken && Token.Value_Text != TEXT(";"))
			{
				ParsedUsing Alias;
				Alias.Kind = ParsedUsing::EKind::Typedef;
				String Declarator;
				String Name;
				while (HasToken && Token.Value_Text != TEXT(",") && Token.Value_Text != TEXT(";"))
				{
					if (Token.Type == ETextTokenType::Identifier) Name = Token.Value_Text;
					else AppendTokenText(Declarator, Token);
					Advance(Token, HasToken);
				}
				if (Name.Size() > 0)
				{
					AddNameSegment(Alias.Name, Name);
					Declarator.Trim();
					if (Declarator.Size() > 0) Alias.Type.Declarator = Declarator + TEXT("$");
					Aliases.Add(std::move(Alias));
				}
				if (HasToken && Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
			}
			Expect(TEXT(";"), Token, HasToken);

			size_t Canonical = 0;
			for (size_t Index = 0; Index < Aliases.Size(); ++Index)
			{
				if (Aliases[Index].Type.Declarator.Size() == 0)
				{
					Canonical = Index;
					break;
				}
			}
			if (Aliases.Size() > 0)
			{
				Aliases[Canonical].Attributes = std::move(Using.Attributes);
				String CanonicalName = Aliases[Canonical].Name.Segments[0].Name;
				String CanonicalDeclarator = Aliases[Canonical].Type.Declarator;
				Aliases[Canonical].Type = {};
				Aliases[Canonical].Type.Declarator = InlineDefinition + TEXT(" ") +
					(CanonicalDeclarator.Size() > 0 ? CanonicalDeclarator : TEXT("$"));
				OnParsed_Using(Aliases[Canonical]);
				for (size_t Index = 0; Index < Aliases.Size(); ++Index)
				{
					if (Index == Canonical) continue;
					AddNameSegment(Aliases[Index].Type.Name, CanonicalName);
					OnParsed_Using(Aliases[Index]);
				}
			}
			return;
		}
		if (!IsTypedef && HasToken && Token.Value_Text == TEXT("namespace"))
		{
			Using.Kind = ParsedUsing::EKind::UsingDirective;
			Advance(Token, HasToken);
			while (HasToken && Token.Value_Text != TEXT(";"))
			{
				if (Token.Type == ETextTokenType::Identifier) AddNameSegment(Using.Target, Token.Value_Text);
				Advance(Token, HasToken);
			}
			Expect(TEXT(";"), Token, HasToken);
			OnParsed_Using(Using);
			return;
		}

		if (!IsTypedef)
		{
			if (HasToken && Token.Value_Text == TEXT("::"))
			{
				Advance(Token, HasToken);
			}
			if (!HasToken || Token.Type != ETextTokenType::Identifier) ThrowError(TEXT("Expected using name"), Token, HasToken);
			String First = Token.Value_Text;
			Advance(Token, HasToken);
			ParseAttributes(Using.Attributes, Token, HasToken);
			if (HasToken && Token.Value_Text == TEXT("="))
			{
				Using.Kind = ParsedUsing::EKind::AliasDeclaration;
				AddNameSegment(Using.Name, First);
				Advance(Token, HasToken);
				bool InDeclarator = false;
				String PendingSegment;
				while (HasToken && Token.Value_Text != TEXT(";"))
				{
					if (InDeclarator)
					{
						AppendTokenText(Using.Type.Declarator, Token);
					}
					else if (Token.Type == ETextTokenType::Identifier)
					{
						if (PendingSegment.Size() > 0) AddNameSegment(Using.Type.Name, PendingSegment);
						PendingSegment = Token.Value_Text;
					}
					else if (Token.Value_Text == TEXT("::"))
					{
						if (PendingSegment.Size() > 0)
						{
							AddNameSegment(Using.Type.Name, PendingSegment);
							PendingSegment.Clear();
						}
					}
					else if (Token.Value_Text == TEXT("<"))
					{
						if (PendingSegment.Size() > 0)
						{
							AddNameSegment(Using.Type.Name, PendingSegment);
							PendingSegment.Clear();
						}
						String TemplateText;
						SkipBalanced(TEXT("<"), TEXT(">"), Token, HasToken, &TemplateText);
						if (Using.Type.Name.Segments.Size() > 0)
						{
							ParsedTemplateArgument& Argument = Using.Type.Name.Segments[Using.Type.Name.Segments.Size() - 1].TemplateArguments.EmplaceRef();
							AddNameSegment(Argument.Type.Name, TemplateText.SubString(1, TemplateText.Size() > 1 ? TemplateText.Size() - 2 : 0));
						}
						continue;
					}
					else
					{
						if (PendingSegment.Size() > 0)
						{
							AddNameSegment(Using.Type.Name, PendingSegment);
							PendingSegment.Clear();
						}
						InDeclarator = true;
						AppendTokenText(Using.Type.Declarator, Token);
					}
					Advance(Token, HasToken);
				}
				if (PendingSegment.Size() > 0) AddNameSegment(Using.Type.Name, PendingSegment);
				Expect(TEXT(";"), Token, HasToken);
				OnParsed_Using(Using);
				return;
			}
			AddNameSegment(Using.Target, First);
			while (HasToken && Token.Value_Text != TEXT(";"))
			{
				if (Token.Type == ETextTokenType::Identifier) AddNameSegment(Using.Target, Token.Value_Text);
				Advance(Token, HasToken);
			}
			Expect(TEXT(";"), Token, HasToken);
			OnParsed_Using(Using);
			return;
		}

		ParsedType Type;
		String Candidate;
		bool InDeclarator = false;
		bool HasDeclaratorName = false;
		while (HasToken && Token.Value_Text != TEXT(";"))
		{
			if (InDeclarator)
			{
				if (!HasDeclaratorName && Token.Type == ETextTokenType::Identifier)
				{
					Candidate = Token.Value_Text;
					Type.Declarator += TEXT("$");
					HasDeclaratorName = true;
				}
				else AppendTokenText(Type.Declarator, Token);
			}
			else if (Token.Type == ETextTokenType::Identifier)
			{
				if (Type.Name.Segments.Size() == 0) AddNameSegment(Type.Name, Token.Value_Text);
				else if (Candidate.Size() == 0) Candidate = Token.Value_Text;
				else
				{
					AddNameSegment(Type.Name, Candidate);
					Candidate = Token.Value_Text;
				}
			}
			else if (Token.Value_Text != TEXT("::"))
			{
				if (Candidate.Size() > 0)
				{
					AddNameSegment(Type.Name, Candidate);
					Candidate.Clear();
				}
				InDeclarator = true;
				AppendTokenText(Type.Declarator, Token);
			}
			Advance(Token, HasToken);
		}
		Expect(TEXT(";"), Token, HasToken);
		if (Candidate.Size() > 0) AddNameSegment(Using.Name, Candidate);
		Using.Type = std::move(Type);
		OnParsed_Using(Using);
	}

	void CppParser::ParseTemplate(TextToken& Token, bool& HasToken)
	{
		Advance(Token, HasToken);
		Expect(TEXT("<"), Token, HasToken);
		ParsedTemplate Template;
		while (HasToken && Token.Value_Text != TEXT(">") && Token.Value_Text != TEXT(">>"))
		{
			ParsedTemplateParameter& Parameter = Template.Parameters.EmplaceRef();
			if (Token.Value_Text == TEXT("template"))
			{
				Parameter.Kind = ParsedTemplateParameter::EKind::TemplateTemplate;
				Parameter.TemplatePrefix = TEXT("template");
				Advance(Token, HasToken);
				if (HasToken && Token.Value_Text == TEXT("<")) SkipBalanced(TEXT("<"), TEXT(">"), Token, HasToken, &Parameter.TemplatePrefix);
				if (HasToken && (Token.Value_Text == TEXT("class") || Token.Value_Text == TEXT("typename")))
				{
					Parameter.TemplatePrefix += TEXT(" ") + Token.Value_Text;
					Advance(Token, HasToken);
				}
				if (HasToken && Token.Type == ETextTokenType::Identifier)
				{
					Parameter.Name = Token.Value_Text;
					Advance(Token, HasToken);
				}
				if (HasToken && Token.Value_Text == TEXT("="))
				{
					Parameter.HasDefault = true;
					Advance(Token, HasToken);
					String Default;
					ReadExpressionUntil(Default, TEXT(","), TEXT(">"), Token, HasToken);
					AddNameSegment(Parameter.DefaultType.Name, Default);
				}
				if (HasToken && Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
				continue;
			}

			Parameter.Kind = Token.Value_Text == TEXT("typename") || Token.Value_Text == TEXT("class") ?
				ParsedTemplateParameter::EKind::Type : ParsedTemplateParameter::EKind::NonType;
			if (Parameter.Kind == ParsedTemplateParameter::EKind::Type) Advance(Token, HasToken);
			String LastIdentifier;
			while (HasToken && Token.Value_Text != TEXT(",") && Token.Value_Text != TEXT(">") && Token.Value_Text != TEXT(">>"))
			{
				if (Token.Value_Text == TEXT("="))
				{
					Parameter.HasDefault = true;
					Advance(Token, HasToken);
					String Default;
					ReadExpressionUntil(Default, TEXT(","), TEXT(">"), Token, HasToken);
					if (Parameter.Kind == ParsedTemplateParameter::EKind::NonType) Parameter.DefaultExpression.Text = std::move(Default);
					else AddNameSegment(Parameter.DefaultType.Name, Default);
					break;
				}
				if (Token.Value_Text == TEXT("...") || Token.Value_Text == TEXT("."))
				{
					Parameter.IsVariadic = ConsumeEllipsis(Token, HasToken);
					continue;
				}
				else if (Token.Type == ETextTokenType::Identifier)
				{
					if (LastIdentifier.Size() > 0 && Parameter.Kind == ParsedTemplateParameter::EKind::NonType) AddNameSegment(Parameter.Type.Name, LastIdentifier);
					LastIdentifier = Token.Value_Text;
				}
				Advance(Token, HasToken);
			}
			Parameter.Name = LastIdentifier;
			if (Parameter.Kind == ParsedTemplateParameter::EKind::NonType && Parameter.Type.Name.Segments.Size() == 1 &&
				Parameter.Type.Name.Segments[0].Name.Size() > 1 && !IsBuiltinType(Parameter.Type.Name.Segments[0].Name))
			{
				Parameter.Kind = ParsedTemplateParameter::EKind::Type;
				Parameter.Constraint = std::move(Parameter.Type.Name);
				Parameter.Type = {};
			}
			if (HasToken && Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
		}
		if (HasToken) Advance(Token, HasToken);
		if (HasToken && Token.Value_Text == TEXT("requires"))
		{
			Template.HasRequires = true;
			Advance(Token, HasToken);
			while (HasToken && Token.Value_Text != TEXT("class") && Token.Value_Text != TEXT("struct") && Token.Value_Text != TEXT("union") && Token.Value_Text != TEXT("concept"))
			{
				AppendTokenText(Template.RequiresClause.Text, Token);
				Advance(Token, HasToken);
			}
		}
		OnParsed_Template(Template);
		ParseDeclaration(Token, HasToken);
	}

	void CppParser::ParseConcept(TextToken& Token, bool& HasToken)
	{
		Advance(Token, HasToken);
		ParsedConcept Concept;
		Concept.Attributes = std::move(m_PendingAttributes);
		ParseAttributes(Concept.Attributes, Token, HasToken);
		if (!HasToken || Token.Type != ETextTokenType::Identifier) ThrowError(TEXT("Expected concept name"), Token, HasToken);
		AddNameSegment(Concept.Name, Token.Value_Text);
		Advance(Token, HasToken);
		Expect(TEXT("="), Token, HasToken);
		ReadExpressionUntil(Concept.Constraint.Text, TEXT(";"), {}, Token, HasToken);
		Expect(TEXT(";"), Token, HasToken);
		OnParsed_Concept(Concept);
	}

	void CppParser::ParseStaticAssert(TextToken& Token, bool& HasToken)
	{
		Advance(Token, HasToken);
		Expect(TEXT("("), Token, HasToken);
		ParsedStaticAssert Assert;
		ReadExpressionUntil(Assert.Condition.Text, TEXT(","), TEXT(")"), Token, HasToken);
		if (HasToken && Token.Value_Text == TEXT(","))
		{
			Assert.HasMessage = true;
			Advance(Token, HasToken);
			ReadExpressionUntil(Assert.Message.Text, TEXT(")"), {}, Token, HasToken);
		}
		Expect(TEXT(")"), Token, HasToken);
		Expect(TEXT(";"), Token, HasToken);
		OnParsed_StaticAssert(Assert);
	}

	void CppParser::ParseLinkage(TextToken& Token, bool& HasToken)
	{
		ParsedLinkage Linkage;
		Linkage.Language = Token.Value_Text;
		Advance(Token, HasToken);
		if (HasToken && Token.Value_Text == TEXT("{"))
		{
			Linkage.HasBody = true;
			Advance(Token, HasToken);
		}
		OnParsed_Linkage(Linkage);
		if (Linkage.HasBody) m_Scopes.Add({ EScopeType::Linkage });
		else ParseDeclaration(Token, HasToken);
	}

	void CppParser::ParseGeneral(TextToken& Token, bool& HasToken, ParsedType Type, EParsedVariableFlags VariableFlags)
	{
		Array<ParsedAttribute> Attributes = std::move(m_PendingAttributes);
		EParsedFunctionFlags FunctionFlags = EParsedFunctionFlags::None;
		if ((static_cast<uint16>(VariableFlags) & static_cast<uint16>(EParsedVariableFlags::IsInline)) != 0)
			AddFlag(FunctionFlags, EParsedFunctionFlags::IsInline);
		if ((static_cast<uint16>(VariableFlags) & static_cast<uint16>(EParsedVariableFlags::IsStatic)) != 0)
			AddFlag(FunctionFlags, EParsedFunctionFlags::IsStatic);
		String Candidate;
		String ExplicitExpression;
		bool IsDestructor = false;
		bool HasDeclaratorIndirection = false;
		bool IsComplexDeclarator = false;
		bool DeclaratorHasName = false;
		int32 ComplexDeclaratorDepth = 0;
		bool TypeIsComplete = Type.ElaboratedType != EParsedElaboratedType::None ||
			(static_cast<uint8>(Type.Flags) & static_cast<uint8>(EParsedTypeFlags::IsDecltype)) != 0;
		ParsedType BaseType = Type;
		auto AppendDeclaratorName = [&]()
			{
				if (IsComplexDeclarator && !DeclaratorHasName && Candidate.Size() > 0)
				{
					Type.Declarator += TEXT("$");
					DeclaratorHasName = true;
				}
			};

		auto EmitVariable = [&](const String& Name, ParsedType VariableType, String Initializer, bool IsBitfield)
			{
				if (Name.Size() == 0) return;
				ParsedVariable Variable;
				Variable.Type = std::move(VariableType);
				Variable.Flags = VariableFlags;
				Variable.Attributes = std::move(Attributes);
				AddNameSegment(Variable.Name, Name);
				if (Initializer.Size() > 0 || IsBitfield)
				{
					AddFlag(Variable.Flags, EParsedVariableFlags::HasInitializer);
					if (IsBitfield) AddFlag(Variable.Flags, EParsedVariableFlags::IsBitfield);
					Variable.Initializer.Text = std::move(Initializer);
				}
				OnParsed_Variable(Variable);
			};

		while (HasToken)
		{
			const String Value = Token.Value_Text;
			if (Value == TEXT("}"))
			{
				if (Candidate.Size() > 0) EmitVariable(Candidate, Type, {}, false);
				return;
			}
			if (IsDeclarationSpecifier(Value))
			{
				if (Value == TEXT("static"))
				{
					AddFlag(VariableFlags, EParsedVariableFlags::IsStatic);
					AddFlag(FunctionFlags, EParsedFunctionFlags::IsStatic);
				}
				else if (Value == TEXT("thread_local")) AddFlag(VariableFlags, EParsedVariableFlags::IsThreadLocal);
				else if (Value == TEXT("extern")) AddFlag(VariableFlags, EParsedVariableFlags::IsExtern);
				else if (Value == TEXT("mutable")) AddFlag(VariableFlags, EParsedVariableFlags::IsMutable);
				else if (Value == TEXT("constexpr"))
				{
					AddFlag(VariableFlags, EParsedVariableFlags::IsConstexpr);
					AddFlag(FunctionFlags, EParsedFunctionFlags::IsConstexpr);
				}
				else if (Value == TEXT("consteval"))
				{
					AddFlag(VariableFlags, EParsedVariableFlags::IsConsteval);
					AddFlag(FunctionFlags, EParsedFunctionFlags::IsConsteval);
				}
				else if (Value == TEXT("inline"))
				{
					AddFlag(VariableFlags, EParsedVariableFlags::IsInline);
					AddFlag(FunctionFlags, EParsedFunctionFlags::IsInline);
				}
				else if (Value == TEXT("virtual")) AddFlag(FunctionFlags, EParsedFunctionFlags::IsVirtual);
				else if (Value == TEXT("explicit")) AddFlag(FunctionFlags, EParsedFunctionFlags::IsExplicit);
				Advance(Token, HasToken);
				if (Value == TEXT("explicit") && HasToken && Token.Value_Text == TEXT("("))
				{
					Advance(Token, HasToken);
					ReadExpressionUntil(ExplicitExpression, TEXT(")"), {}, Token, HasToken);
					Expect(TEXT(")"), Token, HasToken);
				}
				continue;
			}
			if (IsTypeQualifier(Value))
			{
				if (Type.Indirections.Size() > 0 && (Value == TEXT("const") || Value == TEXT("volatile") || Value == TEXT("mutable")))
				{
					ParsedIndirection& Indirection = Type.Indirections[Type.Indirections.Size() - 1];
					if (Value == TEXT("const")) Indirection.IsConst = true;
					else if (Value == TEXT("volatile")) Indirection.IsVolatile = true;
					else Indirection.IsMutable = true;
				}
				else if (Value == TEXT("const")) AddFlag(Type.Flags, EParsedTypeFlags::IsConst);
				else if (Value == TEXT("volatile")) AddFlag(Type.Flags, EParsedTypeFlags::IsVolatile);
				else if (Value == TEXT("mutable")) AddFlag(Type.Flags, EParsedTypeFlags::IsMutable);
				else if (Value == TEXT("unsigned")) AddFlag(Type.Flags, EParsedTypeFlags::IsUnsigned);
				else if (Value == TEXT("signed")) AddFlag(Type.Flags, EParsedTypeFlags::IsSigned);
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT("typename"))
			{
				Type.IsTypename = true;
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT("class") || Value == TEXT("struct") || Value == TEXT("union") || Value == TEXT("enum"))
			{
				m_PendingAttributes = std::move(Attributes);
				m_PendingDeclaredType = Type;
				m_PendingVariableFlags = VariableFlags;
				Advance(Token, HasToken);
				if (Value == TEXT("enum")) ParseEnum(Token, HasToken);
				else ParseClass(Value == TEXT("class") ? EClassType::Class : Value == TEXT("struct") ? EClassType::Struct : EClassType::Union, false, Token, HasToken);
				return;
			}
			if (Value == TEXT("decltype"))
			{
				AddFlag(Type.Flags, EParsedTypeFlags::IsDecltype);
				TypeIsComplete = true;
				Advance(Token, HasToken);
				if (HasToken && Token.Value_Text == TEXT("("))
				{
					Advance(Token, HasToken);
					ReadExpressionUntil(Type.Decltype.Text, TEXT(")"), {}, Token, HasToken);
					Expect(TEXT(")"), Token, HasToken);
				}
				continue;
			}
			if (Value == TEXT("~") || Value == TEXT("compl"))
			{
				IsDestructor = true;
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT("operator"))
			{
				Advance(Token, HasToken);
				String Symbol;
				if (HasToken && Token.Value_Text == TEXT("("))
				{
					Symbol = TEXT("(");
					Advance(Token, HasToken);
					if (HasToken && Token.Value_Text == TEXT(")"))
					{
						Symbol += TEXT(")");
						Advance(Token, HasToken);
					}
				}
				while (HasToken && Token.Value_Text != TEXT("("))
				{
					AppendTokenText(Symbol, Token);
					Advance(Token, HasToken);
				}
				ParseFunction(std::move(Type), TEXT("operator"), false, true, Symbol, FunctionFlags, Attributes, ExplicitExpression, Token, HasToken);
				return;
			}
			if (IsComplexDeclarator && DeclaratorHasName)
			{
				if (Value == TEXT("(") && ComplexDeclaratorDepth == 0 && Type.Declarator == TEXT("($)"))
				{
					Type.Declarator.Clear();
					ParseFunction(std::move(Type), Candidate, IsDestructor, false, {}, FunctionFlags, Attributes, ExplicitExpression, Token, HasToken);
					return;
				}
				const bool IsTopLevelTerminator = ComplexDeclaratorDepth == 0 &&
					(Value == TEXT("=") || Value == TEXT(":") || Value == TEXT(",") || Value == TEXT(";"));
				if (!IsTopLevelTerminator)
				{
					if (Value == TEXT("(")) ++ComplexDeclaratorDepth;
					else if (Value == TEXT(")") && ComplexDeclaratorDepth > 0) --ComplexDeclaratorDepth;
					AppendTokenText(Type.Declarator, Token);
					Advance(Token, HasToken);
					continue;
				}
			}
			if (Token.Type == ETextTokenType::Identifier)
			{
				if (Type.Name.Segments.Size() == 0 && !TypeIsComplete)
				{
					AddNameSegment(Type.Name, Value);
				}
				else if (Candidate.Size() == 0)
				{
					if (IsBuiltinType(Value) && Type.Name.Segments.Size() == 1 && IsBuiltinType(Type.Name.Segments[0].Name))
						Type.Name.Segments[0].Name += TEXT(" ") + Value;
					else Candidate = Value;
				}
				else
				{
					AddNameSegment(Type.Name, Candidate);
					Candidate = Value;
				}
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT("::"))
			{
				if (Candidate.Size() > 0)
				{
					AddNameSegment(Type.Name, Candidate);
					Candidate.Clear();
				}
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT("<") && Type.Name.Segments.Size() > 0)
			{
				if (Candidate.Size() > 0)
				{
					AddNameSegment(Type.Name, Candidate);
					Candidate.Clear();
				}
				ParsedNameSegment& Segment = Type.Name.Segments[Type.Name.Segments.Size() - 1];
				Advance(Token, HasToken);
				bool Closed = false;
				while (HasToken && !Closed)
				{
					ParsedTemplateArgument& Argument = Segment.TemplateArguments.EmplaceRef();
					String ArgumentText;
					int32 Depth = 0;
					while (HasToken)
					{
						if (Token.Value_Text == TEXT(",") && Depth == 0) break;
						if (Token.Value_Text == TEXT(">") && Depth == 0)
						{
							Closed = true;
							break;
						}
						if (Token.Value_Text == TEXT(">>"))
						{
							if (Depth <= 1)
							{
								if (Depth == 1) ArgumentText += TEXT(">");
								Closed = true;
								break;
							}
							Depth -= 2;
						}
						else if (Token.Value_Text == TEXT("<")) ++Depth;
						else if (Token.Value_Text == TEXT(">") && Depth > 0) --Depth;
						AppendTokenText(ArgumentText, Token);
						Advance(Token, HasToken);
					}
					if (ArgumentText.Size() > 0 && (ArgumentText[0] >= '0' && ArgumentText[0] <= '9'))
					{
						Argument.Kind = ParsedTemplateArgument::EKind::Expression;
						Argument.Expression.Text = std::move(ArgumentText);
					}
					else AddNameSegment(Argument.Type.Name, ArgumentText);
					if (HasToken && Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
					else if (Closed) Advance(Token, HasToken);
				}
				continue;
			}
			if (Value == TEXT("*") || Value == TEXT("&") || Value == TEXT("&&"))
			{
				if (IsComplexDeclarator)
				{
					AppendTokenText(Type.Declarator, Token);
					Advance(Token, HasToken);
					continue;
				}
				if (!HasDeclaratorIndirection)
				{
					BaseType = Type;
					HasDeclaratorIndirection = true;
				}
				ParsedIndirection& Indirection = Type.Indirections.EmplaceRef();
				Indirection.Kind = Value == TEXT("*") ? ParsedIndirection::EKind::Pointer : Value == TEXT("&") ? ParsedIndirection::EKind::LReference : ParsedIndirection::EKind::RReference;
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT("(") && Candidate.Size() > 0 && !IsComplexDeclarator)
			{
				ParseFunction(std::move(Type), Candidate, IsDestructor, false, {}, FunctionFlags, Attributes, ExplicitExpression, Token, HasToken);
				return;
			}
			if (Value == TEXT("(") && Candidate.Size() == 0 && !IsComplexDeclarator && Type.Name.Segments.Size() == 1 &&
				(static_cast<uint8>(Type.Flags) & (static_cast<uint8>(EParsedTypeFlags::IsUnsigned) | static_cast<uint8>(EParsedTypeFlags::IsSigned))) != 0)
			{
				String FunctionName = Type.Name.Segments[0].Name;
				Type.Name = {};
				ParseFunction(std::move(Type), FunctionName, IsDestructor, false, {}, FunctionFlags, Attributes, ExplicitExpression, Token, HasToken);
				return;
			}
			if (Value == TEXT("(") && Candidate.Size() == 0 && !IsComplexDeclarator && Type.Name.Segments.Size() == 1 &&
				Type.Name.Segments[0].Name == CurrentClassName())
			{
				String ConstructorName = Type.Name.Segments[0].Name;
				Type = {};
				ParseFunction(std::move(Type), ConstructorName, IsDestructor, false, {}, FunctionFlags, Attributes, ExplicitExpression, Token, HasToken);
				return;
			}
			if (Value == TEXT("("))
			{
				AppendDeclaratorName();
				IsComplexDeclarator = true;
				++ComplexDeclaratorDepth;
				AppendTokenText(Type.Declarator, Token);
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT(")") && IsComplexDeclarator)
			{
				AppendDeclaratorName();
				if (ComplexDeclaratorDepth > 0) --ComplexDeclaratorDepth;
				AppendTokenText(Type.Declarator, Token);
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT("[") && Candidate.Size() > 0)
			{
				if (IsComplexDeclarator)
				{
					AppendDeclaratorName();
					AppendTokenText(Type.Declarator, Token);
					Advance(Token, HasToken);
					while (HasToken && Token.Value_Text != TEXT("]"))
					{
						AppendTokenText(Type.Declarator, Token);
						Advance(Token, HasToken);
					}
					if (HasToken)
					{
						AppendTokenText(Type.Declarator, Token);
						Advance(Token, HasToken);
					}
					continue;
				}
				Advance(Token, HasToken);
				ParsedExpression& Extent = Type.ArrayExtents.EmplaceRef();
				ReadExpressionUntil(Extent.Text, TEXT("]"), {}, Token, HasToken);
				Expect(TEXT("]"), Token, HasToken);
				continue;
			}
			if ((Value == TEXT("=") || Value == TEXT(":")) && Candidate.Size() > 0)
			{
				AppendDeclaratorName();
				const bool IsBitfield = Value == TEXT(":");
				Advance(Token, HasToken);
				String Initializer;
				ReadExpressionUntil(Initializer, TEXT(","), TEXT(";"), Token, HasToken);
				EmitVariable(Candidate, Type, std::move(Initializer), IsBitfield);
				Candidate.Clear();
				if (HasToken && Token.Value_Text == TEXT(","))
				{
					Advance(Token, HasToken);
					if (HasDeclaratorIndirection) Type = BaseType;
					HasDeclaratorIndirection = false;
					continue;
				}
				Expect(TEXT(";"), Token, HasToken);
				return;
			}
			if (Value == TEXT(",") && Candidate.Size() > 0)
			{
				AppendDeclaratorName();
				EmitVariable(Candidate, Type, {}, false);
				Candidate.Clear();
				if (HasDeclaratorIndirection) Type = BaseType;
				HasDeclaratorIndirection = false;
				IsComplexDeclarator = false;
				DeclaratorHasName = false;
				ComplexDeclaratorDepth = 0;
				Advance(Token, HasToken);
				continue;
			}
			if (Value == TEXT(";"))
			{
				AppendDeclaratorName();
				EmitVariable(Candidate, Type, {}, false);
				Advance(Token, HasToken);
				return;
			}
			if (Value == TEXT("{") && Candidate.Size() > 0)
			{
				String Initializer;
				SkipBalanced(TEXT("{"), TEXT("}"), Token, HasToken, &Initializer);
				EmitVariable(Candidate, Type, std::move(Initializer), false);
				if (HasToken && Token.Value_Text == TEXT(";")) Advance(Token, HasToken);
				return;
			}
			if (Value == TEXT("{"))
			{
				SkipBalanced(TEXT("{"), TEXT("}"), Token, HasToken);
				return;
			}
			Advance(Token, HasToken);
		}
	}

	void CppParser::ParseParameters(Array<ParsedFunctionParameter>& Parameters, TextToken& Token, bool& HasToken)
	{
		while (HasToken && Token.Value_Text != TEXT(")"))
		{
			ParsedFunctionParameter& Parameter = Parameters.EmplaceRef();
			if (Token.Value_Text == TEXT("...") || Token.Value_Text == TEXT("."))
			{
				Parameter.IsVariadic = ConsumeEllipsis(Token, HasToken);
			}
			else
			{
				String Candidate;
				while (HasToken && Token.Value_Text != TEXT(",") && Token.Value_Text != TEXT(")") && Token.Value_Text != TEXT("="))
				{
					const String Value = Token.Value_Text;
					if (Value == TEXT("const")) AddFlag(Parameter.Type.Flags, EParsedTypeFlags::IsConst);
					else if (Value == TEXT("volatile")) AddFlag(Parameter.Type.Flags, EParsedTypeFlags::IsVolatile);
					else if (Value == TEXT("typename")) Parameter.Type.IsTypename = true;
					else if (Value == TEXT("...") || Value == TEXT("."))
					{
						Parameter.IsTypePack = ConsumeEllipsis(Token, HasToken);
						continue;
					}
					else if (Token.Type == ETextTokenType::Identifier)
					{
						if (Parameter.Type.Name.Segments.Size() == 0) AddNameSegment(Parameter.Type.Name, Value);
						else if (Candidate.Size() == 0) Candidate = Value;
						else
						{
							AddNameSegment(Parameter.Type.Name, Candidate);
							Candidate = Value;
						}
					}
					else if (Value == TEXT("*") || Value == TEXT("&") || Value == TEXT("&&"))
					{
						ParsedIndirection& Indirection = Parameter.Type.Indirections.EmplaceRef();
						Indirection.Kind = Value == TEXT("*") ? ParsedIndirection::EKind::Pointer : Value == TEXT("&") ? ParsedIndirection::EKind::LReference : ParsedIndirection::EKind::RReference;
					}
					else if (Value == TEXT("<"))
					{
						if (Candidate.Size() > 0)
						{
							AddNameSegment(Parameter.Type.Name, Candidate);
							Candidate.Clear();
						}
						String TemplateText;
						SkipBalanced(TEXT("<"), TEXT(">"), Token, HasToken, &TemplateText);
						if (Parameter.Type.Name.Segments.Size() > 0)
						{
							ParsedTemplateArgument& Argument = Parameter.Type.Name.Segments[Parameter.Type.Name.Segments.Size() - 1].TemplateArguments.EmplaceRef();
							AddNameSegment(Argument.Type.Name, TemplateText.SubString(1, TemplateText.Size() > 1 ? TemplateText.Size() - 2 : 0));
						}
						continue;
					}
					else if (Value == TEXT("["))
					{
						Advance(Token, HasToken);
						ParsedExpression& Extent = Parameter.Type.ArrayExtents.EmplaceRef();
						ReadExpressionUntil(Extent.Text, TEXT("]"), {}, Token, HasToken);
						Expect(TEXT("]"), Token, HasToken);
						continue;
					}
					Advance(Token, HasToken);
				}
				if (Candidate.Size() > 0) AddNameSegment(Parameter.Name, Candidate);
				if (HasToken && Token.Value_Text == TEXT("="))
				{
					Parameter.HasDefaultValue = true;
					Advance(Token, HasToken);
					ReadExpressionUntil(Parameter.DefaultValue.Text, TEXT(","), TEXT(")"), Token, HasToken);
				}
			}
			if (HasToken && Token.Value_Text == TEXT(",")) Advance(Token, HasToken);
		}
	}

	void CppParser::ParseFunction(ParsedType ReturnType, const String& Name, bool IsDestructor, bool IsOperator, const String& OperatorSymbol,
		EParsedFunctionFlags Flags, Array<ParsedAttribute>& Attributes, const String& ExplicitExpression, TextToken& Token, bool& HasToken)
	{
		Expect(TEXT("("), Token, HasToken);
		ParsedFunction Function;
		Function.ReturnType = std::move(ReturnType);
		Function.Flags = Flags;
		Function.Attributes = std::move(Attributes);
		if (ExplicitExpression.Size() > 0)
		{
			Function.HasExplicitExpression = true;
			Function.ExplicitExpression.Text = ExplicitExpression;
		}
		AddNameSegment(Function.Name, Name);
		ParseParameters(Function.Parameters, Token, HasToken);
		Expect(TEXT(")"), Token, HasToken);

		while (HasToken && Token.Value_Text != TEXT(";") && Token.Value_Text != TEXT("{") && Token.Value_Text != TEXT("}"))
		{
			const String Value = Token.Value_Text;
			if (Value == TEXT("const") || Value == TEXT("volatile") || Value == TEXT("&") || Value == TEXT("&&") || Value == TEXT("override") || Value == TEXT("final"))
			{
				Function.Qualifiers.Add(Value);
				Advance(Token, HasToken);
			}
			else if (Value == TEXT("noexcept"))
			{
				AddFlag(Function.Flags, EParsedFunctionFlags::IsNoexcept);
				Advance(Token, HasToken);
				if (HasToken && Token.Value_Text == TEXT("("))
				{
					Advance(Token, HasToken);
					ReadExpressionUntil(Function.NoexceptExpression.Text, TEXT(")"), {}, Token, HasToken);
					Expect(TEXT(")"), Token, HasToken);
				}
			}
			else if (Value == TEXT("->"))
			{
				Function.IsTrailingType = true;
				Function.ReturnType = {};
				Advance(Token, HasToken);
				if (HasToken && Token.Value_Text == TEXT("decltype"))
				{
					AddFlag(Function.ReturnType.Flags, EParsedTypeFlags::IsDecltype);
					Advance(Token, HasToken);
					if (HasToken && Token.Value_Text == TEXT("("))
					{
						Advance(Token, HasToken);
						ReadExpressionUntil(Function.ReturnType.Decltype.Text, TEXT(")"), {}, Token, HasToken);
						Expect(TEXT(")"), Token, HasToken);
					}
				}
				while (HasToken && Token.Value_Text != TEXT("requires") && Token.Value_Text != TEXT("{") && Token.Value_Text != TEXT(";") && Token.Value_Text != TEXT("="))
				{
					if (Token.Type == ETextTokenType::Identifier) AddNameSegment(Function.ReturnType.Name, Token.Value_Text);
					else if (Token.Value_Text == TEXT("*") || Token.Value_Text == TEXT("&") || Token.Value_Text == TEXT("&&"))
					{
						ParsedIndirection& Indirection = Function.ReturnType.Indirections.EmplaceRef();
						Indirection.Kind = Token.Value_Text == TEXT("*") ? ParsedIndirection::EKind::Pointer : Token.Value_Text == TEXT("&") ? ParsedIndirection::EKind::LReference : ParsedIndirection::EKind::RReference;
					}
					Advance(Token, HasToken);
				}
			}
			else if (Value == TEXT("requires"))
			{
				AddFlag(Function.Flags, EParsedFunctionFlags::HasRequires);
				Advance(Token, HasToken);
				if (HasToken && Token.Value_Text == TEXT("requires"))
				{
					AppendTokenText(Function.RequiresClause.Text, Token);
					Advance(Token, HasToken);
					if (HasToken && Token.Value_Text == TEXT("(")) SkipBalanced(TEXT("("), TEXT(")"), Token, HasToken, &Function.RequiresClause.Text);
					if (HasToken && Token.Value_Text == TEXT("{")) SkipBalanced(TEXT("{"), TEXT("}"), Token, HasToken, &Function.RequiresClause.Text);
				}
				else ReadExpressionUntil(Function.RequiresClause.Text, TEXT("{"), TEXT(";"), Token, HasToken);
			}
			else if (Value == TEXT("try"))
			{
				Function.IsTryBlock = true;
				Advance(Token, HasToken);
			}
			else if (Value == TEXT(":"))
			{
				Function.HasInitializer = true;
				Advance(Token, HasToken);
				ReadExpressionUntil(Function.Initializer.Text, TEXT("{"), {}, Token, HasToken);
			}
			else if (Value == TEXT("="))
			{
				Advance(Token, HasToken);
				if (HasToken && Token.Value_Text == TEXT("0")) AddFlag(Function.Flags, EParsedFunctionFlags::IsPureVirtual);
				else if (HasToken && Token.Value_Text == TEXT("default")) AddFlag(Function.Flags, EParsedFunctionFlags::IsDefaulted);
				else if (HasToken && Token.Value_Text == TEXT("delete")) AddFlag(Function.Flags, EParsedFunctionFlags::IsDeleted);
				if (HasToken) Advance(Token, HasToken);
			}
			else Advance(Token, HasToken);
		}

		if (HasToken && Token.Value_Text == TEXT("{"))
		{
			AddFlag(Function.Flags, EParsedFunctionFlags::HasBody);
			SkipBalanced(TEXT("{"), TEXT("}"), Token, HasToken, &Function.Body.Text);
			while (HasToken && Token.Value_Text == TEXT("catch"))
			{
				AppendTokenText(Function.Body.Text, Token);
				Advance(Token, HasToken);
				if (HasToken && Token.Value_Text == TEXT("(")) SkipBalanced(TEXT("("), TEXT(")"), Token, HasToken, &Function.Body.Text);
				if (HasToken && Token.Value_Text == TEXT("{")) SkipBalanced(TEXT("{"), TEXT("}"), Token, HasToken, &Function.Body.Text);
			}
		}
		else if (HasToken && Token.Value_Text == TEXT(";")) Advance(Token, HasToken);

		if (IsOperator)
		{
			ParsedOperator Operator;
			static_cast<ParsedFunction&>(Operator) = std::move(Function);
			Operator.Symbol = OperatorSymbol;
			OnParsed_Operator(Operator);
		}
		else if (IsDestructor)
		{
			ParsedDestructor Destructor;
			static_cast<ParsedFunctionBase&>(Destructor) = std::move(static_cast<ParsedFunctionBase&>(Function));
			OnParsed_Destructor(Destructor);
		}
		else if (CurrentClassName().Size() > 0 && Name == CurrentClassName())
		{
			ParsedConstructor Constructor;
			static_cast<ParsedFunctionBase&>(Constructor) = std::move(static_cast<ParsedFunctionBase&>(Function));
			OnParsed_Constructor(Constructor);
		}
		else OnParsed_Function(Function);
	}

	void CppParser::ParseClosedClassDeclarators(const Scope& ClosedScope, TextToken& Token, bool& HasToken)
	{
		ParsedType Type = ClosedScope.DeclaredType;
		if (ClosedScope.Name.Size() > 0) AddNameSegment(Type.Name, ClosedScope.Name);
		else Type.ElaboratedType = ClosedScope.ElaboratedType;
		ParseGeneral(Token, HasToken, std::move(Type), ClosedScope.VariableFlags);
	}

	String CppParser::CurrentClassName() const
	{
		for (size_t At = m_Scopes.Size(); At > 0; --At)
		{
			if (m_Scopes[At - 1].Type == EScopeType::Class) return m_Scopes[At - 1].Name;
		}
		return {};
	}
}
