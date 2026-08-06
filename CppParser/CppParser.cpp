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

	bool CppParser::IsFunctionTailSpecifier(const String& Value)
	{
		return Value == TEXT("noexcept") || Value == TEXT("requires") || Value == TEXT("override") ||
			Value == TEXT("final") || Value == TEXT("try") || Value == TEXT("catch");
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


	CppParser::CppParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer) : Preprocessor(Path, Tokenizer)
	{
		Tokenizer.Config.SymbolPairs.AddUnique({ '[', '[' });
		Tokenizer.Config.SymbolPairs.AddUnique({ ']', ']' });
		Tokenizer.Config.SymbolPairs.AddUnique({ ':', ':' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '-', '>' });
	}

	void CppParser::Parse()
	{
		OnParseBegin();
		m_Tokens.Clear();
		m_TokenAt = 0;
		m_Scopes.Clear();
		m_PendingAttributes.Clear();
		m_PendingDeclaredType = {};
		m_PendingVariableFlags = EParsedVariableFlags::None;

		TextToken Token;
		while (GetToken(Token))
		{
			m_Tokens.Add(std::move(Token));
		}

		while (HasToken())
		{
			if (ConsumeToken(TEXT(";")))
			{
				continue;
			}
			if (ConsumeToken(TEXT("}")))
			{
				if (m_Scopes.Size() == 0)
				{
					ThrowError(TEXT("Unexpected scope end"));
				}
				Scope ClosedScope = std::move(m_Scopes[m_Scopes.Size() - 1]);
				m_Scopes.RemoveAt(m_Scopes.Size() - 1);
				OnParsed_ScopeEnd();
				if (ClosedScope.Type == EScopeType::Class && !IsToken(TEXT(";")) && !IsToken(TEXT("}")))
				{
					Parse_ClosedClassDeclarators(ClosedScope);
				}
				continue;
			}
			Parse_Declaration();
		}

		if (m_Scopes.Size() != 0)
		{
			ThrowError(TEXT("Expected scope end"));
		}
		OnParseEnd();
	}

	bool CppParser::HasToken(size_t Offset) const
	{
		return m_TokenAt + Offset < m_Tokens.Size();
	}

	const TextToken& CppParser::PeekToken(size_t Offset) const
	{
		if (!HasToken(Offset))
		{
			ThrowError(TEXT("Unexpected end of file"));
		}
		return m_Tokens[m_TokenAt + Offset];
	}

	TextToken CppParser::TakeToken()
	{
		TextToken Token = PeekToken();
		++m_TokenAt;
		return Token;
	}

	bool CppParser::IsToken(const String& Value, size_t Offset) const
	{
		return HasToken(Offset) && PeekToken(Offset).Value_Text == Value;
	}

	bool CppParser::ConsumeToken(const String& Value)
	{
		if (!IsToken(Value))
		{
			return false;
		}
		++m_TokenAt;
		return true;
	}

	void CppParser::ExpectToken(const String& Value)
	{
		if (!ConsumeToken(Value))
		{
			ThrowError(TEXT("Expected '") + Value + TEXT("'"));
		}
	}

	void CppParser::ThrowError(const String& Message) const
	{
		if (HasToken())
		{
			throw TextTokenizerError(Message, PeekToken(), CurrentFile());
		}
		TextToken Token;
		if (m_Tokens.Size() > 0)
		{
			Token = m_Tokens[m_Tokens.Size() - 1];
		}
		throw TextTokenizerError(Message, Token, CurrentFile());
	}

	bool CppParser::IsClassDeclaration() const
	{
		size_t At = m_TokenAt + 1;
		if (At >= m_Tokens.Size())
		{
			return false;
		}
		if (m_Tokens[At].Value_Text == TEXT("[[") || m_Tokens[At].Value_Text == TEXT("alignas") || m_Tokens[At].Value_Text == TEXT("__declspec"))
		{
			return true;
		}
		if (m_Tokens[At].Type != ETextTokenType::Identifier)
		{
			return m_Tokens[At].Value_Text == TEXT("{");
		}

		while (At < m_Tokens.Size())
		{
			++At;
			if (At < m_Tokens.Size() && m_Tokens[At].Value_Text == TEXT("<"))
			{
				int32 Depth = 1;
				while (++At < m_Tokens.Size() && Depth > 0)
				{
					if (m_Tokens[At].Value_Text == TEXT("<"))
					{
						++Depth;
					}
					else if (m_Tokens[At].Value_Text == TEXT(">"))
					{
						--Depth;
					}
					else if (m_Tokens[At].Value_Text == TEXT(">>"))
					{
						Depth -= 2;
					}
				}
			}
			if (At < m_Tokens.Size() && m_Tokens[At].Value_Text == TEXT("::"))
			{
				++At;
				if (At < m_Tokens.Size() && m_Tokens[At].Type == ETextTokenType::Identifier)
				{
					continue;
				}
			}
			break;
		}

		return At < m_Tokens.Size() && (m_Tokens[At].Value_Text == TEXT("{") || m_Tokens[At].Value_Text == TEXT(":") ||
			m_Tokens[At].Value_Text == TEXT("final") || m_Tokens[At].Value_Text == TEXT(";"));
	}

	size_t CppParser::FindDeclaratorName(size_t Begin, size_t End) const
	{
		size_t Equal = FindTopLevel(Begin, End, TEXT("="));
		if (Equal < End)
		{
			End = Equal;
		}
		for (size_t At = Begin; At < End; ++At)
		{
			if (m_Tokens[At].Value_Text != TEXT("(") || At + 1 >= End ||
				(m_Tokens[At + 1].Value_Text != TEXT("*") && m_Tokens[At + 1].Value_Text != TEXT("&") &&
				m_Tokens[At + 1].Value_Text != TEXT("&&")))
			{
				continue;
			}
			size_t Close = FindMatching(At, TEXT("("), TEXT(")"));
			if (Close >= End)
			{
				continue;
			}
			for (size_t Inner = At + 1; Inner < Close; ++Inner)
			{
				if (m_Tokens[Inner].Type == ETextTokenType::Identifier && !IsTypeQualifier(m_Tokens[Inner].Value_Text) &&
					!IsDeclarationSpecifier(m_Tokens[Inner].Value_Text) && !IsBuiltinType(m_Tokens[Inner].Value_Text))
				{
					return Inner;
				}
			}
		}

		int32 Parens = 0;
		int32 Brackets = 0;
		int32 Braces = 0;
		int32 Angles = 0;
		size_t NameAt = End;
		for (size_t At = Begin; At < End; ++At)
		{
			const String& Value = m_Tokens[At].Value_Text;
			if (Value == TEXT("(")) ++Parens;
			else if (Value == TEXT(")")) --Parens;
			else if (Value == TEXT("[") || Value == TEXT("[[")) ++Brackets;
			else if (Value == TEXT("]") || Value == TEXT("]]")) --Brackets;
			else if (Value == TEXT("{")) ++Braces;
			else if (Value == TEXT("}")) --Braces;
			else if (Value == TEXT("<")) ++Angles;
			else if (Value == TEXT(">") && Angles > 0) --Angles;
			else if (Value == TEXT(">>") && Angles > 0) Angles = Angles > 1 ? Angles - 2 : 0;
			else if (Parens == 0 && Brackets == 0 && Braces == 0 && Angles == 0 && m_Tokens[At].Type == ETextTokenType::Identifier &&
				!IsTypeQualifier(Value) && !IsDeclarationSpecifier(Value) && Value != TEXT("typename") && Value != TEXT("class") &&
				Value != TEXT("struct") && Value != TEXT("union") && Value != TEXT("enum") && Value != TEXT("decltype") &&
				!IsFunctionTailSpecifier(Value))
			{
				if (At + 1 < End && m_Tokens[At + 1].Value_Text == TEXT("("))
				{
					return At;
				}
				NameAt = At;
			}
		}
		return NameAt;
	}

	size_t CppParser::FindDeclaratorTypeEnd(size_t Begin, size_t NameAt) const
	{
		for (size_t At = Begin; At < NameAt; ++At)
		{
			if (m_Tokens[At].Value_Text == TEXT("(") && At + 1 < NameAt &&
				(m_Tokens[At + 1].Value_Text == TEXT("*") || m_Tokens[At + 1].Value_Text == TEXT("&") ||
				m_Tokens[At + 1].Value_Text == TEXT("&&") || m_Tokens[At + 1].Value_Text == TEXT("(")))
			{
				return At;
			}
		}

		int32 Parens = 0;
		int32 Angles = 0;
		for (size_t At = Begin; At < NameAt; ++At)
		{
			const String& Value = m_Tokens[At].Value_Text;
			if (Value == TEXT("(")) ++Parens;
			else if (Value == TEXT(")")) --Parens;
			else if (Value == TEXT("<")) ++Angles;
			else if (Value == TEXT(">") && Angles > 0) --Angles;
			else if (Value == TEXT(">>") && Angles > 0) Angles = Angles > 1 ? Angles - 2 : 0;
			else if (Parens == 0 && Angles == 0 && (Value == TEXT("*") || Value == TEXT("&") || Value == TEXT("&&")))
			{
				return At;
			}
		}
		return NameAt;
	}

	String CppParser::BuildDeclarator(size_t Begin, size_t End, size_t NameAt) const
	{
		size_t Equal = FindTopLevel(Begin, End, TEXT("="));
		if (Equal < End)
		{
			End = Equal;
		}
		String Declarator;
		for (size_t At = Begin; At < End; ++At)
		{
			if (Declarator.Size() > 0 && m_Tokens[At].Whitespaces.Size() > 0)
			{
				Declarator += TEXT(" ");
			}
			if (At == NameAt)
			{
				Declarator += TEXT("$");
			}
			else
			{
				Declarator += m_Tokens[At].RawText.Size() > 0 ? m_Tokens[At].RawText : m_Tokens[At].Value_Text;
			}
		}
		Declarator.Trim();
		return Declarator;
	}

	String CppParser::TokensToText(size_t Begin, size_t End) const
	{
		String Text;
		for (size_t At = Begin; At < End; ++At)
		{
			const TextToken& Token = m_Tokens[At];
			if (Text.Size() > 0 && Token.Whitespaces.Size() > 0)
			{
				Text += TEXT(" ");
			}
			Text += Token.RawText.Size() > 0 ? Token.RawText : Token.Value_Text;
		}
		Text.Trim();
		return Text;
	}

	size_t CppParser::FindMatching(size_t Open, const String& OpenValue, const String& CloseValue) const
	{
		size_t Depth = 0;
		for (size_t At = Open; At < m_Tokens.Size(); ++At)
		{
			if (m_Tokens[At].Value_Text == OpenValue)
			{
				++Depth;
			}
			else if (m_Tokens[At].Value_Text == CloseValue && --Depth == 0)
			{
				return At;
			}
		}
		ThrowError(TEXT("Unterminated balanced token sequence"));
		return m_Tokens.Size();
	}

	size_t CppParser::FindTopLevel(size_t Begin, size_t End, const String& Value) const
	{
		int32 Parens = 0;
		int32 Brackets = 0;
		int32 Braces = 0;
		int32 Angles = 0;
		for (size_t At = Begin; At < End; ++At)
		{
			const String& Token = m_Tokens[At].Value_Text;
			if (Token == Value && Parens == 0 && Brackets == 0 && Braces == 0 && Angles == 0)
			{
				return At;
			}
			if (Token == TEXT("("))
			{
				++Parens;
			}
			else if (Token == TEXT(")"))
			{
				--Parens;
			}
			else if (Token == TEXT("[") || Token == TEXT("[["))
			{
				++Brackets;
			}
			else if (Token == TEXT("]") || Token == TEXT("]]"))
			{
				--Brackets;
			}
			else if (Token == TEXT("{"))
			{
				++Braces;
			}
			else if (Token == TEXT("}"))
			{
				--Braces;
			}
			else if (Token == TEXT("<"))
			{
				++Angles;
			}
			else if (Token == TEXT(">") && Angles > 0)
			{
				--Angles;
			}
			else if (Token == TEXT(">>") && Angles > 0)
			{
				Angles = Angles > 1 ? Angles - 2 : 0;
			}
		}
		return End;
	}

	Array<std::pair<size_t, size_t>> CppParser::SplitTopLevel(size_t Begin, size_t End, const String& Value) const
	{
		Array<std::pair<size_t, size_t>> Parts;
		size_t PartBegin = Begin;
		while (PartBegin <= End)
		{
			size_t PartEnd = FindTopLevel(PartBegin, End, Value);
			Parts.Add({ PartBegin, PartEnd });
			if (PartEnd == End)
			{
				break;
			}
			PartBegin = PartEnd + 1;
		}
		return Parts;
	}

	void CppParser::Parse_Declaration()
	{
		Array<ParsedAttribute> Attributes;
		Parse_Attributes(Attributes);
		if (Attributes.Size() > 0)
		{
			m_PendingAttributes = std::move(Attributes);
		}
		size_t DeclarationBegin = m_TokenAt;
		size_t KeywordAt = DeclarationBegin;
		while (KeywordAt < m_Tokens.Size() && m_Tokens[KeywordAt].Type == ETextTokenType::Identifier &&
			(IsTypeQualifier(m_Tokens[KeywordAt].Value_Text) || IsDeclarationSpecifier(m_Tokens[KeywordAt].Value_Text)) &&
			m_Tokens[KeywordAt].Value_Text != TEXT("friend"))
		{
			++KeywordAt;
		}
		if (KeywordAt > DeclarationBegin && KeywordAt < m_Tokens.Size() &&
			(m_Tokens[KeywordAt].Value_Text == TEXT("class") || m_Tokens[KeywordAt].Value_Text == TEXT("struct") || m_Tokens[KeywordAt].Value_Text == TEXT("union") || m_Tokens[KeywordAt].Value_Text == TEXT("enum")))
		{
			m_TokenAt = KeywordAt;
			bool IsDefinition = m_Tokens[KeywordAt].Value_Text == TEXT("enum") ?
				FindTopLevel(KeywordAt, m_Tokens.Size(), TEXT("{")) < FindTopLevel(KeywordAt, m_Tokens.Size(), TEXT(";")) : IsClassDeclaration();
			if (IsDefinition)
			{
				Parse_Type(DeclarationBegin, KeywordAt, m_PendingDeclaredType);
				for (size_t At = DeclarationBegin; At < KeywordAt; ++At)
				{
					if (m_Tokens[At].Value_Text == TEXT("static")) AddFlag(m_PendingVariableFlags, EParsedVariableFlags::IsStatic);
					else if (m_Tokens[At].Value_Text == TEXT("thread_local")) AddFlag(m_PendingVariableFlags, EParsedVariableFlags::IsThreadLocal);
					else if (m_Tokens[At].Value_Text == TEXT("constexpr")) AddFlag(m_PendingVariableFlags, EParsedVariableFlags::IsConstexpr);
				}
			}
			else
			{
				m_TokenAt = DeclarationBegin;
				m_PendingDeclaredType = {};
				m_PendingVariableFlags = EParsedVariableFlags::None;
			}
		}

		if (IsToken(TEXT("inline")) && IsToken(TEXT("namespace"), 1))
		{
			TakeToken();
			TakeToken();
			Parse_Namespace(true);
		}
		else if (IsToken(TEXT("namespace")))
		{
			TakeToken();
			Parse_Namespace(false);
		}
		else if ((IsToken(TEXT("class")) || IsToken(TEXT("struct")) || IsToken(TEXT("union"))) && IsClassDeclaration())
		{
			TextToken Token = TakeToken();
			EClassType Type = EClassType::Union;
			if (Token.Value_Text == TEXT("class"))
			{
				Type = EClassType::Class;
			}
			else if (Token.Value_Text == TEXT("struct"))
			{
				Type = EClassType::Struct;
			}
			Parse_Class(Type);
		}
		else if (IsToken(TEXT("friend")) && (IsToken(TEXT("class"), 1) || IsToken(TEXT("struct"), 1) || IsToken(TEXT("union"), 1)))
		{
			TakeToken();
			TextToken Token = TakeToken();
			EClassType Type = EClassType::Union;
			if (Token.Value_Text == TEXT("class"))
			{
				Type = EClassType::Class;
			}
			else if (Token.Value_Text == TEXT("struct"))
			{
				Type = EClassType::Struct;
			}
			Parse_Class(Type, true);
		}
		else if (IsToken(TEXT("enum")))
		{
			TakeToken();
			Parse_Enum();
		}
		else if (IsToken(TEXT("public")) || IsToken(TEXT("protected")) || IsToken(TEXT("private")))
		{
			TextToken Token = TakeToken();
			EAccessSpecifier Access = EAccessSpecifier::Private;
			if (Token.Value_Text == TEXT("public"))
			{
				Access = EAccessSpecifier::Public;
			}
			else if (Token.Value_Text == TEXT("protected"))
			{
				Access = EAccessSpecifier::Protected;
			}
			Parse_Access(Access);
		}
		else if (IsToken(TEXT("using")))
		{
			Parse_Using(false);
		}
		else if (IsToken(TEXT("typedef")))
		{
			Parse_Using(true);
		}
		else if (IsToken(TEXT("template")))
		{
			Parse_Template();
		}
		else if (IsToken(TEXT("concept")))
		{
			Parse_Concept();
		}
		else if (IsToken(TEXT("static_assert")))
		{
			Parse_StaticAssert();
		}
		else if (IsToken(TEXT("extern")) && HasToken(1) && PeekToken(1).Type == ETextTokenType::Constant && PeekToken(1).ConstantType == ETextTokenConstantType::Text)
		{
			Parse_Linkage();
		}
		else
		{
			Parse_General();
		}
	}

	void CppParser::Parse_Name(size_t Begin, size_t End, ParsedName& Name, bool AllowInline)
	{
		Name.Segments.Clear();
		size_t At = Begin;
		if (At + 1 < End && m_Tokens[At].Value_Text == TEXT(":") && m_Tokens[At + 1].Value_Text == TEXT(":"))
		{
			Name.Segments.EmplaceRef();
			At += 2;
		}
		else if (At < End && m_Tokens[At].Value_Text == TEXT("::"))
		{
			Name.Segments.EmplaceRef();
			++At;
		}

		while (At < End)
		{
			bool IsInline = false;
			if (AllowInline && m_Tokens[At].Value_Text == TEXT("inline"))
			{
				IsInline = true;
				++At;
			}
			if (At >= End || m_Tokens[At].Type != ETextTokenType::Identifier)
			{
				break;
			}
			ParsedNameSegment& Segment = Name.Segments.EmplaceRef();
			Segment.Name = m_Tokens[At++].Value_Text;
			Segment.IsInline = IsInline;
			if (At < End && m_Tokens[At].Value_Text == TEXT("<"))
			{
				size_t Close = At + 1;
				int32 Depth = 1;
				for (; Close < End; ++Close)
				{
					if (m_Tokens[Close].Value_Text == TEXT("<")) ++Depth;
					else if (m_Tokens[Close].Value_Text == TEXT(">") && --Depth == 0)
					{
						break;
					}
					else if (m_Tokens[Close].Value_Text == TEXT(">>"))
					{
						Depth -= 2;
						if (Depth <= 0)
						{
							break;
						}
					}
				}
				for (const auto& Part : SplitTopLevel(At + 1, Close, TEXT(",")))
				{
					if (Part.first == Part.second)
					{
						continue;
					}
					ParsedTemplateArgument& Argument = Segment.TemplateArguments.EmplaceRef();
					const TextToken& First = m_Tokens[Part.first];
					const bool IsExpression = First.Type == ETextTokenType::Constant || First.Value_Text == TEXT("sizeof") || First.Value_Text == TEXT("&") || First.Value_Text == TEXT("-") || First.Value_Text == TEXT("+");
					if (IsExpression)
					{
						Argument.Kind = ParsedTemplateArgument::EKind::Expression;
						Argument.Expression.Text = TokensToText(Part.first, Part.second);
					}
					else
					{
						Parse_Type(Part.first, Part.second, Argument.Type);
					}
				}
				At = Close < End ? Close + 1 : End;
			}
			if (At < End && m_Tokens[At].Value_Text == TEXT("::"))
			{
				++At;
			}
			else if (At + 1 < End && m_Tokens[At].Value_Text == TEXT(":") && m_Tokens[At + 1].Value_Text == TEXT(":"))
			{
				At += 2;
			}
			else
			{
				break;
			}
		}
	}

	void CppParser::Parse_Type(size_t Begin, size_t End, ParsedType& Type)
	{
		Type = {};
		for (size_t At = Begin; At < End; ++At)
		{
			if (m_Tokens[At].Value_Text == TEXT("typename"))
			{
				Type.IsTypename = true;
				break;
			}
		}
		while (Begin < End && IsDeclarationSpecifier(m_Tokens[Begin].Value_Text)) ++Begin;
		for (size_t At = Begin; At < End; ++At)
		{
			const String& Value = m_Tokens[At].Value_Text;
			if (Value == TEXT("const"))
			{
				AddFlag(Type.Flags, EParsedTypeFlags::IsConst);
			}
			else if (Value == TEXT("volatile"))
			{
				AddFlag(Type.Flags, EParsedTypeFlags::IsVolatile);
			}
			else if (Value == TEXT("mutable"))
			{
				AddFlag(Type.Flags, EParsedTypeFlags::IsMutable);
			}
			else if (Value == TEXT("unsigned"))
			{
				AddFlag(Type.Flags, EParsedTypeFlags::IsUnsigned);
			}
			else if (Value == TEXT("signed"))
			{
				AddFlag(Type.Flags, EParsedTypeFlags::IsSigned);
			}
			else if (Value == TEXT("class"))
			{
				Type.ElaboratedType = EParsedElaboratedType::Class;
			}
			else if (Value == TEXT("struct"))
			{
				Type.ElaboratedType = EParsedElaboratedType::Struct;
			}
			else if (Value == TEXT("union"))
			{
				Type.ElaboratedType = EParsedElaboratedType::Union;
			}
			else if (Value == TEXT("enum"))
			{
				Type.ElaboratedType = EParsedElaboratedType::Enum;
			}
			else if (Value == TEXT("decltype") && At + 1 < End && m_Tokens[At + 1].Value_Text == TEXT("("))
			{
				size_t Close = FindMatching(At + 1, TEXT("("), TEXT(")"));
				AddFlag(Type.Flags, EParsedTypeFlags::IsDecltype);
				Type.Decltype.Text = TokensToText(At + 2, Close);
				At = Close;
			}
			else if (Value == TEXT("*") || Value == TEXT("&") || Value == TEXT("&&"))
			{
				ParsedIndirection& Indirection = Type.Indirections.EmplaceRef();
				if (Value == TEXT("*"))
				{
					Indirection.Kind = ParsedIndirection::EKind::Pointer;
				}
				else if (Value == TEXT("&"))
				{
					Indirection.Kind = ParsedIndirection::EKind::LReference;
				}
				else
				{
					Indirection.Kind = ParsedIndirection::EKind::RReference;
				}
				while (At + 1 < End && IsTypeQualifier(m_Tokens[At + 1].Value_Text))
				{
					++At;
					if (m_Tokens[At].Value_Text == TEXT("const"))
					{
						Indirection.IsConst = true;
					}
					else if (m_Tokens[At].Value_Text == TEXT("volatile"))
					{
						Indirection.IsVolatile = true;
					}
					else if (m_Tokens[At].Value_Text == TEXT("mutable"))
					{
						Indirection.IsMutable = true;
					}
				}
			}
		}

		size_t NameBegin = Begin;
		while (NameBegin < End && (IsTypeQualifier(m_Tokens[NameBegin].Value_Text) || IsDeclarationSpecifier(m_Tokens[NameBegin].Value_Text) ||
			m_Tokens[NameBegin].Value_Text == TEXT("class") || m_Tokens[NameBegin].Value_Text == TEXT("struct") ||
			m_Tokens[NameBegin].Value_Text == TEXT("union") || m_Tokens[NameBegin].Value_Text == TEXT("enum") || m_Tokens[NameBegin].Value_Text == TEXT("typename"))) ++NameBegin;
		if (NameBegin < End && m_Tokens[NameBegin].Value_Text != TEXT("decltype"))
		{
			size_t NameEnd = NameBegin;
			int32 Angles = 0;
			while (NameEnd < End)
			{
				const String& Value = m_Tokens[NameEnd].Value_Text;
				if (Value == TEXT("<"))
				{
					++Angles;
				}
				else if (Value == TEXT(">") && Angles > 0)
				{
					--Angles;
				}
				else if (Value == TEXT(">>") && Angles > 0)
				{
					Angles = Angles > 1 ? Angles - 2 : 0;
				}
				if (Angles == 0 && (Value == TEXT("*") || Value == TEXT("&") || Value == TEXT("&&") || Value == TEXT("[") || Value == TEXT("(")))
				{
					break;
				}
				++NameEnd;
			}
			Parse_Name(NameBegin, NameEnd, Type.Name);
			if (Type.Name.Segments.Size() == 1 && IsBuiltinType(Type.Name.Segments[0].Name))
			{
				String Builtin = Type.Name.Segments[0].Name;
				for (size_t At = NameBegin + 1; At < NameEnd; ++At)
				{
					if (IsBuiltinType(m_Tokens[At].Value_Text))
					{
						Builtin += TEXT(" ") + m_Tokens[At].Value_Text;
					}
				}
				Type.Name.Segments[0].Name = Builtin;
			}
		}
		size_t DeclaratorAt = FindTopLevel(Begin, End, TEXT("("));
		if (DeclaratorAt < End && DeclaratorAt + 1 < End && (m_Tokens[DeclaratorAt + 1].Value_Text == TEXT("*") ||
			m_Tokens[DeclaratorAt + 1].Value_Text == TEXT("&") || m_Tokens[DeclaratorAt + 1].Value_Text == TEXT("&&")))
		{
			Type.Declarator = TokensToText(DeclaratorAt, End);
		}
	}

	void CppParser::Parse_Attributes(Array<ParsedAttribute>& Attributes)
	{
		size_t At = m_TokenAt;
		Parse_Attributes(At, m_Tokens.Size(), Attributes);
		m_TokenAt = At;
	}

	void CppParser::Parse_Attributes(size_t& At, size_t End, Array<ParsedAttribute>& Attributes)
	{
		while (At < End)
		{
			if (m_Tokens[At].Value_Text == TEXT("[["))
			{
				size_t Close = At + 1;
				while (Close < End && m_Tokens[Close].Value_Text != TEXT("]]")) ++Close;
				if (Close == End)
				{
					ThrowError(TEXT("Unterminated attribute"));
				}
				for (const auto& Part : SplitTopLevel(At + 1, Close, TEXT(",")))
				{
					ParsedAttribute& Attribute = Attributes.EmplaceRef();
					Attribute.Kind = ParsedAttribute::EKind::Standard;
					Attribute.Text = TokensToText(Part.first, Part.second);
					size_t Open = FindTopLevel(Part.first, Part.second, TEXT("("));
					size_t NameEnd = Open == Part.second ? Part.second : Open;
					Parse_Name(Part.first, NameEnd, Attribute.Name);
					if (Open < Part.second)
					{
						Attribute.Arguments.EmplaceRef().Text = TokensToText(Open + 1, Part.second - 1);
					}
				}
				At = Close + 1;
			}
			else if (m_Tokens[At].Value_Text == TEXT("alignas") || m_Tokens[At].Value_Text == TEXT("__declspec") || m_Tokens[At].Value_Text == TEXT("__attribute__"))
			{
				const String Kind = m_Tokens[At].Value_Text;
				if (At + 1 >= End || m_Tokens[At + 1].Value_Text != TEXT("("))
				{
					break;
				}
				size_t Close = FindMatching(At + 1, TEXT("("), TEXT(")"));
				ParsedAttribute& Attribute = Attributes.EmplaceRef();
				if (Kind == TEXT("alignas"))
				{
					Attribute.Kind = ParsedAttribute::EKind::Alignas;
				}
				else if (Kind == TEXT("__declspec"))
				{
					Attribute.Kind = ParsedAttribute::EKind::Declspec;
				}
				else
				{
					Attribute.Kind = ParsedAttribute::EKind::Gnu;
				}
				if (Attribute.Kind == ParsedAttribute::EKind::Declspec)
				{
					Parse_Name(At + 2, Close, Attribute.Name);
				}
				else
				{
					size_t ArgBegin = At + 2;
					size_t ArgEnd = Close;
					if (Attribute.Kind == ParsedAttribute::EKind::Gnu && ArgBegin < ArgEnd && m_Tokens[ArgBegin].Value_Text == TEXT("("))
					{
						++ArgBegin;
						--ArgEnd;
					}
					Attribute.Arguments.EmplaceRef().Text = TokensToText(ArgBegin, ArgEnd);
				}
				At = Close + 1;
			}
			else
			{
				break;
			}
		}
	}

	void CppParser::Parse_Namespace(bool IsInline)
	{
		ParsedNamespace Namespace;
		Namespace.Attributes = std::move(m_PendingAttributes);
		Parse_Attributes(Namespace.Attributes);
		const size_t NameBegin = m_TokenAt;
		while (HasToken() && !IsToken(TEXT("{")) && !IsToken(TEXT("="))) ++m_TokenAt;
		Parse_Name(NameBegin, m_TokenAt, Namespace.Name, true);
		if (Namespace.Name.Segments.Size() > 0)
		{
			Namespace.Name.Segments[0].IsInline = IsInline;
		}
		if (ConsumeToken(TEXT("=")))
		{
			ParsedNamespaceAlias Alias;
			Alias.Name = Namespace.Name;
			Alias.Attributes = Namespace.Attributes;
			const size_t TargetBegin = m_TokenAt;
			m_TokenAt = FindTopLevel(TargetBegin, m_Tokens.Size(), TEXT(";"));
			Parse_Name(TargetBegin, m_TokenAt, Alias.Target);
			ExpectToken(TEXT(";"));
			OnParsed_NamespaceAlias(Alias);
			return;
		}
		ExpectToken(TEXT("{"));
		OnParsed_Namespace(Namespace);
		m_Scopes.Add({ EScopeType::Namespace, {}, EParsedElaboratedType::None });
	}

	void CppParser::Parse_Class(EClassType Type, bool IsFriend)
	{
		ParsedClass Class;
		Class.Type = Type;
		Class.IsFriend = IsFriend;
		Class.Attributes = std::move(m_PendingAttributes);
		Parse_Attributes(Class.Attributes);
		if (HasToken() && PeekToken().Type == ETextTokenType::Identifier && !IsToken(TEXT("final")))
		{
			size_t NameEnd = m_TokenAt + 1;
			if (NameEnd < m_Tokens.Size() && m_Tokens[NameEnd].Value_Text == TEXT("<"))
			{
				int32 Depth = 0;
				for (; NameEnd < m_Tokens.Size(); ++NameEnd)
				{
					if (m_Tokens[NameEnd].Value_Text == TEXT("<")) ++Depth;
					else if (m_Tokens[NameEnd].Value_Text == TEXT(">") && --Depth == 0)
					{
						++NameEnd;
						break;
					}
					else if (m_Tokens[NameEnd].Value_Text == TEXT(">>") && (Depth -= 2) <= 0)
					{
						++NameEnd;
						break;
					}
				}
			}
			Parse_Name(m_TokenAt, NameEnd, Class.Name);
			if (Class.Name.Segments.Size() > 0)
			{
				Class.Specialization = std::move(Class.Name.Segments[Class.Name.Segments.Size() - 1].TemplateArguments);
			}
			m_TokenAt = NameEnd;
		}
		else
		{
			Class.IsAnonymous = true;
		}
		Parse_Attributes(Class.Attributes);
		if (ConsumeToken(TEXT("final")))
		{
			Class.IsFinal = true;
		}
		if (ConsumeToken(TEXT(":")))
		{
			size_t BasesBegin = m_TokenAt;
			while (HasToken() && !IsToken(TEXT("{")) && !IsToken(TEXT(";"))) ++m_TokenAt;
			for (const auto& Part : SplitTopLevel(BasesBegin, m_TokenAt, TEXT(",")))
			{
				ParsedBaseClass& Base = Class.BaseClasses.EmplaceRef();
				Base.AccessSpecifier = EAccessSpecifier::Public;
				if (Type == EClassType::Class)
				{
					Base.AccessSpecifier = EAccessSpecifier::Private;
				}
				size_t At = Part.first;
				while (At < Part.second)
				{
					if (m_Tokens[At].Value_Text == TEXT("public"))
					{
						Base.AccessSpecifier = EAccessSpecifier::Public;
					}
					else if (m_Tokens[At].Value_Text == TEXT("protected"))
					{
						Base.AccessSpecifier = EAccessSpecifier::Protected;
					}
					else if (m_Tokens[At].Value_Text == TEXT("private"))
					{
						Base.AccessSpecifier = EAccessSpecifier::Private;
					}
					else if (m_Tokens[At].Value_Text == TEXT("virtual"))
					{
						Base.IsVirtual = true;
					}
					else
					{
						break;
					}
					++At;
				}
				Parse_Type(At, Part.second, Base.Type);
			}
		}
		if (ConsumeToken(TEXT(";")))
		{
			Class.IsForward = true;
			OnParsed_Class(Class);
			return;
		}
		ExpectToken(TEXT("{"));
		Class.HasBody = true;
		OnParsed_Class(Class);
		Scope ScopeData;
		ScopeData.Type = EScopeType::Class;
		if (Type == EClassType::Class)
		{
			ScopeData.ElaboratedType = EParsedElaboratedType::Class;
		}
		else if (Type == EClassType::Struct)
		{
			ScopeData.ElaboratedType = EParsedElaboratedType::Struct;
		}
		else
		{
			ScopeData.ElaboratedType = EParsedElaboratedType::Union;
		}
		ScopeData.DeclaredType = std::move(m_PendingDeclaredType);
		ScopeData.DeclaredType.ElaboratedType = ScopeData.ElaboratedType;
		ScopeData.VariableFlags = m_PendingVariableFlags;
		m_PendingVariableFlags = EParsedVariableFlags::None;
		if (Class.Name.Segments.Size() > 0)
		{
			ScopeData.Name = Class.Name.Segments[Class.Name.Segments.Size() - 1].Name;
		}
		m_Scopes.Add(std::move(ScopeData));
	}

	void CppParser::Parse_Enum()
	{
		ParsedEnum Enum;
		Enum.Attributes = std::move(m_PendingAttributes);
		if (ConsumeToken(TEXT("class")))
		{
			Enum.IsScoped = true;
		}
		else if (ConsumeToken(TEXT("struct")))
		{
			Enum.IsScoped = true;
			Enum.IsStruct = true;
		}
		Parse_Attributes(Enum.Attributes);
		if (HasToken() && PeekToken().Type == ETextTokenType::Identifier)
		{
			Parse_Name(m_TokenAt, m_TokenAt + 1, Enum.Name);
			++m_TokenAt;
		}
		else
		{
			Enum.IsAnonymous = true;
		}
		Parse_Attributes(Enum.Attributes);
		if (ConsumeToken(TEXT(":")))
		{
			size_t Begin = m_TokenAt;
			while (HasToken() && !IsToken(TEXT("{")) && !IsToken(TEXT(";"))) ++m_TokenAt;
			Parse_Type(Begin, m_TokenAt, Enum.UnderlyingType);
		}
		if (ConsumeToken(TEXT(";")))
		{
			Enum.IsForward = true;
			OnParsed_Enum(Enum);
			return;
		}
		ExpectToken(TEXT("{"));
		OnParsed_Enum(Enum);
		while (HasToken() && !IsToken(TEXT("}")))
		{
			if (ConsumeToken(TEXT(",")))
			{
				continue;
			}
			ParsedEnumValue Value;
			if (PeekToken().Type != ETextTokenType::Identifier)
			{
				ThrowError(TEXT("Expected enum value"));
			}
			Value.Name = TakeToken().Value_Text;
			Parse_Attributes(Value.Attributes);
			if (ConsumeToken(TEXT("=")))
			{
				Value.HasValue = true;
				size_t Begin = m_TokenAt;
				size_t End = FindTopLevel(Begin, m_Tokens.Size(), TEXT(","));
				size_t BraceEnd = FindTopLevel(Begin, m_Tokens.Size(), TEXT("}"));
				if (BraceEnd < End)
				{
					End = BraceEnd;
				}
				Value.Value.Text = TokensToText(Begin, End);
				m_TokenAt = End;
			}
			OnParsed_EnumValue(Value);
			ConsumeToken(TEXT(","));
		}
		ExpectToken(TEXT("}"));
		OnParsed_ScopeEnd();
		if (!ConsumeToken(TEXT(";")))
		{
			size_t Begin = m_TokenAt;
			m_TokenAt = FindTopLevel(Begin, m_Tokens.Size(), TEXT(";"));
			size_t End = m_TokenAt;
			ExpectToken(TEXT(";"));
			ParsedType Type = std::move(m_PendingDeclaredType);
			Type.ElaboratedType = EParsedElaboratedType::Enum;
			Type.Name = Enum.Name;
			for (const auto& Part : SplitTopLevel(Begin, End, TEXT(",")))
			{
				size_t NameAt = FindDeclaratorName(Part.first, Part.second);
				if (NameAt < Part.second)
				{
					Parse_Variable(Part.first, Part.second, NameAt, Type, m_PendingVariableFlags);
				}
			}
			m_PendingVariableFlags = EParsedVariableFlags::None;
		}
	}

	void CppParser::Parse_Access(EAccessSpecifier Access)
	{
		ExpectToken(TEXT(":"));
		OnParsed_Access(Access);
	}

	void CppParser::Parse_Using(bool IsTypedef)
	{
		TakeToken();
		size_t Begin = m_TokenAt;
		m_TokenAt = FindTopLevel(Begin, m_Tokens.Size(), TEXT(";"));
		size_t End = m_TokenAt;
		ExpectToken(TEXT(";"));
		if (!IsTypedef && Begin < End && m_Tokens[Begin].Value_Text == TEXT("namespace"))
		{
			ParsedUsing Using;
			Using.Attributes = std::move(m_PendingAttributes);
			Using.Kind = ParsedUsing::EKind::UsingDirective;
			Parse_Name(Begin + 1, End, Using.Target);
			OnParsed_Using(Using);
			return;
		}
		if (!IsTypedef)
		{
			size_t Equal = FindTopLevel(Begin, End, TEXT("="));
			ParsedUsing Using;
			Using.Attributes = std::move(m_PendingAttributes);
			if (Equal < End)
			{
				Using.Kind = ParsedUsing::EKind::AliasDeclaration;
				size_t AttributeAt = FindTopLevel(Begin, Equal, TEXT("[["));
				size_t NameEnd = AttributeAt < Equal ? AttributeAt : Equal;
				Parse_Name(Begin, NameEnd, Using.Name);
				if (AttributeAt < Equal)
				{
					Parse_Attributes(AttributeAt, Equal, Using.Attributes);
				}
				size_t DeclaratorAt = FindTopLevel(Equal + 1, End, TEXT("("));
				if (DeclaratorAt < End && DeclaratorAt + 1 < End && (m_Tokens[DeclaratorAt + 1].Value_Text == TEXT("*") || m_Tokens[DeclaratorAt + 1].Value_Text == TEXT("&") || m_Tokens[DeclaratorAt + 1].Value_Text == TEXT("&&")))
				{
					Parse_Type(Equal + 1, DeclaratorAt, Using.Type);
					Using.Type.Declarator = TokensToText(DeclaratorAt, End);
				}
				else
				{
					Parse_Type(Equal + 1, End, Using.Type);
				}
			}
			else
			{
				Using.Kind = ParsedUsing::EKind::UsingDeclaration;
				Parse_Name(Begin, End, Using.Target);
			}
			OnParsed_Using(Using);
			return;
		}
		auto Parts = SplitTopLevel(Begin, End, TEXT(","));
		size_t FirstNameAt = Parts.Size() > 0 ? FindDeclaratorName(Parts[0].first, Parts[0].second) : End;
		size_t BaseEnd = FirstNameAt < End ? FindDeclaratorTypeEnd(Begin, FirstNameAt) : End;
		size_t BodyAt = FindTopLevel(Begin, BaseEnd, TEXT("{"));
		size_t BodyEnd = BodyAt < BaseEnd ? FindMatching(BodyAt, TEXT("{"), TEXT("}")) + 1 : BodyAt;
		ParsedType BaseType;
		Parse_Type(Begin, BodyAt < BaseEnd ? BodyAt : BaseEnd, BaseType);
		String InlineDefinition;
		if (BodyAt < BaseEnd)
		{
			InlineDefinition = TokensToText(BodyAt, BodyEnd);
		}
		if (InlineDefinition.Size() > 0)
		{
			size_t OriginalIndex = Parts.Size();
			for (size_t Index = 0; Index < Parts.Size(); ++Index)
			{
				const auto& Part = Parts[Index];
				size_t NameAt = FindDeclaratorName(Part.first, Part.second);
				if (NameAt == Part.second)
				{
					continue;
				}
				size_t DeclaratorBegin = Part.first == Begin ? BaseEnd : Part.first;
				if (BuildDeclarator(DeclaratorBegin, Part.second, NameAt) == TEXT("$"))
				{
					OriginalIndex = Index;
					break;
				}
			}
			if (OriginalIndex < Parts.Size())
			{
				const auto& OriginalPart = Parts[OriginalIndex];
				size_t OriginalNameAt = FindDeclaratorName(OriginalPart.first, OriginalPart.second);
				ParsedUsing Original;
				Original.Kind = ParsedUsing::EKind::Typedef;
				Original.Attributes = std::move(m_PendingAttributes);
				Original.Type = BaseType;
				Original.Type.Declarator = InlineDefinition + TEXT(" $");
				Parse_Name(OriginalNameAt, OriginalNameAt + 1, Original.Name);
				OnParsed_Using(Original);

				ParsedName OriginalName = Original.Name;
				for (size_t Index = 0; Index < Parts.Size(); ++Index)
				{
					if (Index == OriginalIndex)
					{
						continue;
					}
					const auto& Part = Parts[Index];
					size_t NameAt = FindDeclaratorName(Part.first, Part.second);
					if (NameAt == Part.second)
					{
						continue;
					}
					ParsedUsing Using;
					Using.Kind = ParsedUsing::EKind::Typedef;
					Using.Type.Name = OriginalName;
					Parse_Name(NameAt, NameAt + 1, Using.Name);
					size_t DeclaratorBegin = Part.first == Begin ? BaseEnd : Part.first;
					String Declarator = BuildDeclarator(DeclaratorBegin, Part.second, NameAt);
					if (Declarator != TEXT("$"))
					{
						Using.Type.Declarator = std::move(Declarator);
					}
					OnParsed_Using(Using);
				}
				return;
			}
		}
		for (const auto& Part : Parts)
		{
			size_t NameAt = FindDeclaratorName(Part.first, Part.second);
			if (NameAt == Part.second)
			{
				continue;
			}
			ParsedUsing Using;
			if (Part.first == Begin)
			{
				Using.Attributes = std::move(m_PendingAttributes);
			}
			Using.Kind = ParsedUsing::EKind::Typedef;
			Parse_Name(NameAt, NameAt + 1, Using.Name);
			Using.Type = BaseType;
			size_t DeclaratorBegin = Part.first == Begin ? BaseEnd : Part.first;
			String Declarator = BuildDeclarator(DeclaratorBegin, Part.second, NameAt);
			if (InlineDefinition.Size() > 0)
			{
				Declarator = InlineDefinition + TEXT(" ") + Declarator;
			}
			if (Declarator != TEXT("$"))
			{
				Using.Type.Declarator = std::move(Declarator);
			}
			OnParsed_Using(Using);
		}
	}

	void CppParser::Parse_Template()
	{
		TakeToken();
		ExpectToken(TEXT("<"));
		size_t Begin = m_TokenAt;
		int32 Depth = 1;
		while (HasToken() && Depth > 0)
		{
			if (IsToken(TEXT("<"))) ++Depth;
			else if (IsToken(TEXT(">")) && --Depth == 0)
			{
				break;
			}
			else if (IsToken(TEXT(">>")) && (Depth -= 2) <= 0)
			{
				break;
			}
			++m_TokenAt;
		}
		size_t End = m_TokenAt;
		if (!HasToken())
		{
			ThrowError(TEXT("Unterminated template parameter list"));
		}
		++m_TokenAt;
		ParsedTemplate Template;
		for (const auto& Part : SplitTopLevel(Begin, End, TEXT(",")))
		{
			if (Part.first == Part.second)
			{
				continue;
			}
			ParsedTemplateParameter& Parameter = Template.Parameters.EmplaceRef();
			size_t At = Part.first;
			if (m_Tokens[At].Value_Text == TEXT("template")) Parameter.Kind = ParsedTemplateParameter::EKind::TemplateTemplate;
			else if (m_Tokens[At].Value_Text == TEXT("typename") || m_Tokens[At].Value_Text == TEXT("class")) Parameter.Kind = ParsedTemplateParameter::EKind::Type;
			else if (At + 1 < Part.second && (m_Tokens[At + 1].Value_Text == TEXT("typename") || m_Tokens[At + 1].Value_Text == TEXT("class")))
			{
				Parameter.Kind = ParsedTemplateParameter::EKind::Type;
				Parse_Name(At, At + 1, Parameter.Constraint);
			}
			else if (At + 1 < Part.second && m_Tokens[At].Type == ETextTokenType::Identifier &&
				m_Tokens[At + 1].Type == ETextTokenType::Identifier && m_Tokens[At].Value_Text.Size() > 1 && !IsBuiltinType(m_Tokens[At].Value_Text))
			{
				Parameter.Kind = ParsedTemplateParameter::EKind::Type;
				Parse_Name(At, At + 1, Parameter.Constraint);
			}
			else Parameter.Kind = ParsedTemplateParameter::EKind::NonType;
			size_t Equal = FindTopLevel(At, Part.second, TEXT("="));
			size_t NameAt = Equal < Part.second ? Equal : Part.second;
			while (NameAt > At && m_Tokens[NameAt - 1].Type != ETextTokenType::Identifier) --NameAt;
			if (NameAt > At)
			{
				--NameAt;
				Parameter.Name = m_Tokens[NameAt].Value_Text;
				if (Parameter.Kind == ParsedTemplateParameter::EKind::TemplateTemplate)
				{
					Parameter.TemplatePrefix = TokensToText(At, NameAt);
				}
				if (NameAt > At && m_Tokens[NameAt - 1].Value_Text == TEXT("...")) Parameter.IsVariadic = true;
				else if (NameAt >= At + 3 && m_Tokens[NameAt - 1].Value_Text == TEXT(".") && m_Tokens[NameAt - 2].Value_Text == TEXT(".") && m_Tokens[NameAt - 3].Value_Text == TEXT(".")) Parameter.IsVariadic = true;
				if (Parameter.Kind == ParsedTemplateParameter::EKind::NonType)
				{
					Parse_Type(At, NameAt, Parameter.Type);
				}
			}
			if (Equal < Part.second)
			{
				Parameter.HasDefault = true;
				if (Parameter.Kind == ParsedTemplateParameter::EKind::NonType) Parameter.DefaultExpression.Text = TokensToText(Equal + 1, Part.second);
				else
				{
					Parse_Type(Equal + 1, Part.second, Parameter.DefaultType);
				}
			}
		}
		if (ConsumeToken(TEXT("requires")))
		{
			Template.HasRequires = true;
			size_t RequiresBegin = m_TokenAt;
			while (HasToken() && !IsToken(TEXT("class")) && !IsToken(TEXT("struct")) && !IsToken(TEXT("union")) && !IsToken(TEXT("concept"))) ++m_TokenAt;
			Template.RequiresClause.Text = TokensToText(RequiresBegin, m_TokenAt);
		}
		OnParsed_Template(Template);
		Parse_Declaration();
	}

	void CppParser::Parse_Concept()
	{
		TakeToken();
		ParsedConcept Concept;
		Concept.Attributes = std::move(m_PendingAttributes);
		Parse_Attributes(Concept.Attributes);
		if (!HasToken() || PeekToken().Type != ETextTokenType::Identifier)
		{
			ThrowError(TEXT("Expected concept name"));
		}
		Parse_Name(m_TokenAt, m_TokenAt + 1, Concept.Name);
		++m_TokenAt;
		ExpectToken(TEXT("="));
		size_t Begin = m_TokenAt;
		m_TokenAt = FindTopLevel(Begin, m_Tokens.Size(), TEXT(";"));
		Concept.Constraint.Text = TokensToText(Begin, m_TokenAt);
		ExpectToken(TEXT(";"));
		OnParsed_Concept(Concept);
	}

	void CppParser::Parse_StaticAssert()
	{
		TakeToken();
		ExpectToken(TEXT("("));
		size_t Close = FindMatching(m_TokenAt - 1, TEXT("("), TEXT(")"));
		size_t Comma = FindTopLevel(m_TokenAt, Close, TEXT(","));
		ParsedStaticAssert Assert;
		Assert.Condition.Text = TokensToText(m_TokenAt, Comma);
		if (Comma < Close)
		{
			Assert.HasMessage = true;
			Assert.Message.Text = TokensToText(Comma + 1, Close);
		}
		m_TokenAt = Close + 1;
		ExpectToken(TEXT(";"));
		OnParsed_StaticAssert(Assert);
	}

	void CppParser::Parse_Linkage()
	{
		TakeToken();
		TextToken Language = TakeToken();
		ParsedLinkage Linkage;
		Linkage.Language = Language.Value_Text;
		Linkage.HasBody = ConsumeToken(TEXT("{"));
		OnParsed_Linkage(Linkage);
		if (Linkage.HasBody)
		{
			m_Scopes.Add({ EScopeType::Linkage, {}, EParsedElaboratedType::None });
		}
		else
		{
			Parse_Declaration();
		}
	}

	void CppParser::Parse_General()
	{
		const size_t Begin = m_TokenAt;
		int32 Parens = 0;
		int32 Brackets = 0;
		int32 Braces = 0;
		bool FunctionBody = false;
		while (HasToken())
		{
			const String& Value = PeekToken().Value_Text;
			if (Value == TEXT(";") && Parens == 0 && Brackets == 0 && Braces == 0)
			{
				break;
			}
			if (Value == TEXT("}") && Parens == 0 && Brackets == 0 && Braces == 0)
			{
				break;
			}
			if (Value == TEXT("(")) ++Parens;
			else if (Value == TEXT(")")) --Parens;
			else if (Value == TEXT("[") || Value == TEXT("[[")) ++Brackets;
			else if (Value == TEXT("]") || Value == TEXT("]]")) --Brackets;
			else if (Value == TEXT("{") && Parens == 0 && Brackets == 0 && Braces == 0)
			{
				bool HasFunctionParen = FindTopLevel(Begin, m_TokenAt, TEXT("(")) < m_TokenAt;
				bool IsRequiresExpression = false;
				for (size_t At = Begin; At < m_TokenAt; ++At)
				{
					if (m_Tokens[At].Value_Text == TEXT("requires") && At + 1 < m_TokenAt && m_Tokens[At + 1].Value_Text == TEXT("("))
					{
						IsRequiresExpression = true;
					}
				}
				if (HasFunctionParen && !IsRequiresExpression)
				{
					FunctionBody = true;
					size_t Close = FindMatching(m_TokenAt, TEXT("{"), TEXT("}"));
					m_TokenAt = Close + 1;
					while (IsToken(TEXT("catch")))
					{
						++m_TokenAt;
						if (IsToken(TEXT("(")))
						{
							m_TokenAt = FindMatching(m_TokenAt, TEXT("("), TEXT(")")) + 1;
						}
						if (IsToken(TEXT("{")))
						{
							m_TokenAt = FindMatching(m_TokenAt, TEXT("{"), TEXT("}")) + 1;
						}
					}
					break;
				}
				++Braces;
			}
			else if (Value == TEXT("{") && Braces > 0)
			{
				++Braces;
			}
			else if (Value == TEXT("}") && Braces > 0) --Braces;
			++m_TokenAt;
		}
		const size_t End = m_TokenAt;
		if (End == Begin)
		{
			ThrowError(TEXT("Unexpected token '") + PeekToken().Value_Text + TEXT("'"));
		}
		ConsumeToken(TEXT(";"));

		EParsedVariableFlags VariableFlags = EParsedVariableFlags::None;
		EParsedFunctionFlags FunctionFlags = EParsedFunctionFlags::None;
		for (size_t At = Begin; At < End; ++At)
		{
			const String& Value = m_Tokens[At].Value_Text;
			if (Value == TEXT("static"))
			{
				AddFlag(VariableFlags, EParsedVariableFlags::IsStatic);
				AddFlag(FunctionFlags, EParsedFunctionFlags::IsStatic);
			}
			else if (Value == TEXT("thread_local"))
			{
				AddFlag(VariableFlags, EParsedVariableFlags::IsThreadLocal);
			}
			else if (Value == TEXT("extern"))
			{
				AddFlag(VariableFlags, EParsedVariableFlags::IsExtern);
			}
			else if (Value == TEXT("mutable"))
			{
				AddFlag(VariableFlags, EParsedVariableFlags::IsMutable);
			}
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
			else if (Value == TEXT("virtual"))
			{
				AddFlag(FunctionFlags, EParsedFunctionFlags::IsVirtual);
			}
			else if (Value == TEXT("explicit"))
			{
				AddFlag(FunctionFlags, EParsedFunctionFlags::IsExplicit);
			}
		}

		size_t NameAt = FindDeclaratorName(Begin, End);
		size_t OperatorAt = End;
		for (size_t At = Begin; At < End; ++At)
		{
			if (m_Tokens[At].Value_Text == TEXT("operator"))
			{
				OperatorAt = At;
				break;
			}
		}
		bool IsOperator = OperatorAt < End;
		size_t Open = End;
		if (IsOperator)
		{
			for (size_t At = OperatorAt + 1; At < End; ++At)
			{
				if (m_Tokens[At].Value_Text == TEXT("("))
				{
					if (At + 1 < End && m_Tokens[At + 1].Value_Text == TEXT(")") && At + 2 < End && m_Tokens[At + 2].Value_Text == TEXT("("))
					{
						Open = At + 2;
					}
					else
					{
						Open = At;
					}
					break;
				}
			}
		}
		else if (NameAt < End && NameAt + 1 < End && m_Tokens[NameAt + 1].Value_Text == TEXT("("))
		{
			Open = NameAt + 1;
		}
		if (Open < End)
		{
			size_t Close = FindMatching(Open, TEXT("("), TEXT(")"));
			if (NameAt == End && !IsOperator)
			{
				ThrowError(TEXT("Expected function name"));
			}
			bool IsDestructor = !IsOperator && NameAt > Begin && (m_Tokens[NameAt - 1].Value_Text == TEXT("~") || m_Tokens[NameAt - 1].Value_Text == TEXT("compl"));
			const String ClassName = CurrentClassName();
			bool IsConstructor = !IsOperator && !IsDestructor && ClassName.Size() > 0 && m_Tokens[NameAt].Value_Text == ClassName;
			ParsedFunction Function;
			Function.Attributes = std::move(m_PendingAttributes);
			Function.Flags = FunctionFlags;
			for (size_t At = Begin; At < NameAt; ++At)
			{
				if (m_Tokens[At].Value_Text == TEXT("explicit") && At + 1 < NameAt && m_Tokens[At + 1].Value_Text == TEXT("("))
				{
					size_t ExplicitEnd = FindMatching(At + 1, TEXT("("), TEXT(")"));
					Function.HasExplicitExpression = true;
					Function.ExplicitExpression.Text = TokensToText(At + 2, ExplicitEnd);
				}
			}
			if (FunctionBody)
			{
				AddFlag(Function.Flags, EParsedFunctionFlags::HasBody);
			}
			Parse_Name(NameAt, NameAt + 1, Function.Name);
			Parse_Parameters(Open + 1, Close, Function.Parameters);
			if (!IsConstructor && !IsDestructor)
			{
				Parse_Type(Begin, IsOperator ? OperatorAt : NameAt, Function.ReturnType);
			}
			bool IsTrailing = false;
			Parse_FunctionTail(Close + 1, End, Function, &Function.ReturnType, IsTrailing);
			Function.IsTrailingType = IsTrailing;
			if (FunctionBody)
			{
				size_t BodyOpen = FindTopLevel(Close + 1, End, TEXT("{"));
				if (BodyOpen < End)
				{
					Function.Body.Text = TokensToText(BodyOpen, End);
				}
			}
			if (IsOperator)
			{
				ParsedOperator Operator;
				static_cast<ParsedFunction&>(Operator) = std::move(Function);
				Operator.Symbol = TokensToText(OperatorAt + 1, Open);
				OnParsed_Operator(Operator);
			}
			else if (IsConstructor)
			{
				ParsedConstructor Constructor;
				static_cast<ParsedFunctionBase&>(Constructor) = std::move(static_cast<ParsedFunctionBase&>(Function));
				OnParsed_Constructor(Constructor);
			}
			else if (IsDestructor)
			{
				ParsedDestructor Destructor;
				static_cast<ParsedFunctionBase&>(Destructor) = std::move(static_cast<ParsedFunctionBase&>(Function));
				OnParsed_Destructor(Destructor);
			}
			else OnParsed_Function(Function);
			return;
		}

		auto Parts = SplitTopLevel(Begin, End, TEXT(","));
		size_t BaseEnd = End;
		if (Parts.Size() > 0)
		{
			size_t NameAt = FindDeclaratorName(Parts[0].first, Parts[0].second);
			if (NameAt < Parts[0].second)
			{
				BaseEnd = FindDeclaratorTypeEnd(Begin, NameAt);
			}
		}
		ParsedType BaseType;
		Parse_Type(Begin, BaseEnd, BaseType);
		for (size_t Index = 0; Index < Parts.Size(); ++Index)
		{
			size_t PartBegin = Parts[Index].first;
			size_t PartEnd = Parts[Index].second;
			size_t NameAt = FindDeclaratorName(PartBegin, PartEnd);
			if (NameAt == PartEnd)
			{
				continue;
			}
			Parse_Variable(PartBegin, PartEnd, NameAt, BaseType, VariableFlags);
		}
	}

	void CppParser::Parse_ClosedClassDeclarators(const Scope& ClosedScope)
	{
		size_t Begin = m_TokenAt;
		m_TokenAt = FindTopLevel(Begin, m_Tokens.Size(), TEXT(";"));
		size_t End = m_TokenAt;
		ExpectToken(TEXT(";"));
		ParsedType Type = ClosedScope.DeclaredType;
		if (ClosedScope.Name.Size() > 0)
		{
			Type.ElaboratedType = EParsedElaboratedType::None;
			Type.Name.Segments.EmplaceRef().Name = ClosedScope.Name;
		}
		else
		{
			Type.ElaboratedType = ClosedScope.ElaboratedType;
		}
		for (const auto& Part : SplitTopLevel(Begin, End, TEXT(",")))
		{
			size_t NameAt = FindDeclaratorName(Part.first, Part.second);
			if (NameAt < Part.second)
			{
				Parse_Variable(Part.first, Part.second, NameAt, Type, ClosedScope.VariableFlags);
			}
		}
	}

	void CppParser::Parse_Parameters(size_t Begin, size_t End, Array<ParsedFunctionParameter>& Parameters)
	{
		if (Begin == End || (End == Begin + 1 && m_Tokens[Begin].Value_Text == TEXT("void"))) return;
		for (const auto& Part : SplitTopLevel(Begin, End, TEXT(",")))
		{
			ParsedFunctionParameter& Parameter = Parameters.EmplaceRef();
			if (TokensToText(Part.first, Part.second) == TEXT("..."))
			{
				Parameter.IsVariadic = true;
				continue;
			}
			size_t Equal = FindTopLevel(Part.first, Part.second, TEXT("="));
			if (Equal < Part.second)
			{
				Parameter.HasDefaultValue = true;
				Parameter.DefaultValue.Text = TokensToText(Equal + 1, Part.second);
			}
			size_t NameAt = FindDeclaratorName(Part.first, Equal);
			if (NameAt < Equal)
			{
				bool IsName = false;
				for (size_t At = Part.first; At < NameAt; ++At)
				{
					if (m_Tokens[At].Type == ETextTokenType::Identifier && !IsTypeQualifier(m_Tokens[At].Value_Text) &&
						!IsDeclarationSpecifier(m_Tokens[At].Value_Text) && m_Tokens[At].Value_Text != TEXT("typename"))
					{
						IsName = true;
						break;
					}
				}
				if (NameAt + 1 < Equal && m_Tokens[NameAt + 1].Value_Text == TEXT("<"))
				{
					IsName = false;
				}
				if (NameAt > Part.first && m_Tokens[NameAt - 1].Value_Text == TEXT("::") && NameAt + 1 == Equal)
				{
					IsName = false;
				}
				for (size_t At = NameAt + 1; At < Equal; ++At)
				{
					if (m_Tokens[At].Value_Text == TEXT("*") || m_Tokens[At].Value_Text == TEXT("&") || m_Tokens[At].Value_Text == TEXT("&&"))
					{
						IsName = false;
						break;
					}
				}
				if (IsName)
				{
					Parse_Name(NameAt, NameAt + 1, Parameter.Name);
					Parse_Type(Part.first, NameAt, Parameter.Type);
					for (size_t At = Part.first; At < NameAt; ++At)
					{
						if (m_Tokens[At].Value_Text == TEXT("...") || (At + 2 < NameAt && m_Tokens[At].Value_Text == TEXT(".") && m_Tokens[At + 1].Value_Text == TEXT(".") && m_Tokens[At + 2].Value_Text == TEXT(".")))
						{
							Parameter.IsTypePack = true;
							break;
						}
					}
				}
				else
				{
					Parse_Type(Part.first, Equal, Parameter.Type);
				}
			}
			size_t Bracket = FindTopLevel(Part.first, Equal, TEXT("["));
			while (Bracket < Equal)
			{
				size_t Close = FindMatching(Bracket, TEXT("["), TEXT("]"));
				Parameter.Type.ArrayExtents.EmplaceRef().Text = TokensToText(Bracket + 1, Close);
				Bracket = FindTopLevel(Close + 1, Equal, TEXT("["));
			}
		}
	}

	void CppParser::Parse_FunctionTail(size_t Begin, size_t End, ParsedFunctionBase& Function, ParsedType* TrailingType, bool& IsTrailingType)
	{
		for (size_t At = Begin; At < End; ++At)
		{
			const String& Value = m_Tokens[At].Value_Text;
			if (Value == TEXT("try"))
			{
				Function.IsTryBlock = true;
			}
			else if (Value == TEXT(":") && !Function.HasInitializer)
			{
				size_t InitializerEnd = FindTopLevel(At + 1, End, TEXT("{"));
				Function.HasInitializer = true;
				Function.Initializer.Text = TokensToText(At + 1, InitializerEnd);
				At = InitializerEnd > 0 ? InitializerEnd - 1 : InitializerEnd;
			}
			else if (Value == TEXT("const") || Value == TEXT("volatile") || Value == TEXT("&") || Value == TEXT("&&") || Value == TEXT("override") || Value == TEXT("final"))
			{
				Function.Qualifiers.Add(Value);
			}
			else if (Value == TEXT("noexcept"))
			{
				AddFlag(Function.Flags, EParsedFunctionFlags::IsNoexcept);
				if (At + 1 < End && m_Tokens[At + 1].Value_Text == TEXT("("))
				{
					size_t Close = FindMatching(At + 1, TEXT("("), TEXT(")"));
					Function.NoexceptExpression.Text = TokensToText(At + 2, Close);
					At = Close;
				}
			}
			else if (Value == TEXT("->") && TrailingType != nullptr)
			{
				size_t TypeEnd = End;
				for (size_t Scan = At + 1; Scan < End; ++Scan)
				{
					if (m_Tokens[Scan].Value_Text == TEXT("requires") || m_Tokens[Scan].Value_Text == TEXT("{") || m_Tokens[Scan].Value_Text == TEXT("="))
					{
						TypeEnd = Scan;
						break;
					}
				}
				Parse_Type(At + 1, TypeEnd, *TrailingType);
				IsTrailingType = true;
				At = TypeEnd - 1;
			}
			else if (Value == TEXT("requires"))
			{
				AddFlag(Function.Flags, EParsedFunctionFlags::HasRequires);
				size_t RequiresEnd = End;
				if ((static_cast<uint16>(Function.Flags) & static_cast<uint16>(EParsedFunctionFlags::HasBody)) != 0)
				{
					RequiresEnd = FindTopLevel(At + 1, End, TEXT("{"));
				}
				Function.RequiresClause.Text = TokensToText(At + 1, RequiresEnd);
				At = RequiresEnd - 1;
			}
			else if (Value == TEXT("=") && At + 1 < End)
			{
				if (m_Tokens[At + 1].Value_Text == TEXT("0"))
				{
					AddFlag(Function.Flags, EParsedFunctionFlags::IsPureVirtual);
				}
				else if (m_Tokens[At + 1].Value_Text == TEXT("default"))
				{
					AddFlag(Function.Flags, EParsedFunctionFlags::IsDefaulted);
				}
				else if (m_Tokens[At + 1].Value_Text == TEXT("delete"))
				{
					AddFlag(Function.Flags, EParsedFunctionFlags::IsDeleted);
				}
			}
		}
	}

	void CppParser::Parse_Variable(size_t Begin, size_t End, size_t NameAt, const ParsedType& BaseType, EParsedVariableFlags Flags)
	{
		ParsedVariable Variable;
		Variable.Type = BaseType;
		Variable.Flags = Flags;
		Variable.Attributes = std::move(m_PendingAttributes);
		Parse_Name(NameAt, NameAt + 1, Variable.Name);
		size_t DeclaratorBegin = FindDeclaratorTypeEnd(Begin, NameAt);
		for (size_t At = DeclaratorBegin; At < NameAt; ++At)
		{
			const String& Value = m_Tokens[At].Value_Text;
			if (Value == TEXT("*") || Value == TEXT("&") || Value == TEXT("&&"))
			{
				ParsedIndirection& Indirection = Variable.Type.Indirections.EmplaceRef();
				Indirection.Kind = Value == TEXT("*") ? ParsedIndirection::EKind::Pointer : Value == TEXT("&") ? ParsedIndirection::EKind::LReference : ParsedIndirection::EKind::RReference;
				while (At + 1 < NameAt && IsTypeQualifier(m_Tokens[At + 1].Value_Text))
				{
					++At;
					if (m_Tokens[At].Value_Text == TEXT("const"))
					{
						Indirection.IsConst = true;
					}
					else if (m_Tokens[At].Value_Text == TEXT("volatile"))
					{
						Indirection.IsVolatile = true;
					}
				}
			}
		}
		String Declarator = BuildDeclarator(DeclaratorBegin, End, NameAt);
		bool IsComplexDeclarator = Declarator != TEXT("$") &&
			((DeclaratorBegin < NameAt && m_Tokens[DeclaratorBegin].Value_Text == TEXT("(")) || FindTopLevel(DeclaratorBegin, End, TEXT("(")) < End);
		if (IsComplexDeclarator)
		{
			Variable.Type.Declarator = std::move(Declarator);
			Variable.Type.Indirections.Clear();
			Variable.Type.ArrayExtents.Clear();
		}
		if (!IsComplexDeclarator)
		{
			size_t Bracket = FindTopLevel(NameAt + 1, End, TEXT("["));
			while (Bracket < End)
			{
				size_t Close = FindMatching(Bracket, TEXT("["), TEXT("]"));
				Variable.Type.ArrayExtents.EmplaceRef().Text = TokensToText(Bracket + 1, Close);
				Bracket = FindTopLevel(Close + 1, End, TEXT("["));
			}
		}
		size_t Equal = FindTopLevel(Begin, End, TEXT("="));
		size_t Colon = FindTopLevel(Begin, End, TEXT(":"));
		if (Colon < Equal)
		{
			Equal = Colon;
			AddFlag(Variable.Flags, EParsedVariableFlags::IsBitfield);
		}
		if (Equal < End)
		{
			AddFlag(Variable.Flags, EParsedVariableFlags::HasInitializer);
			Variable.Initializer.Text = TokensToText(Equal + 1, End);
		}
		OnParsed_Variable(Variable);
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
