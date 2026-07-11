#include "CppParser.h"


namespace CE
{
	CppParser::CppParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer) : Preprocessor(Path, Tokenizer)
	{
		Tokenizer.Config.SymbolPairs.AddUnique({ '[', '[' });
		Tokenizer.Config.SymbolPairs.AddUnique({ ']', ']' });
	}

	void CppParser::Parse()
	{
		OnParseBegin();

		while (true)
		{
			TextToken Token;
			if (!GetParserToken(Token))
			{
				break;
			}

			if (IsIdentifier(Token, L"inline"))
			{
				TextToken NextToken;
				if (GetParserToken(NextToken))
				{
					if (IsIdentifier(NextToken, L"namespace"))
					{
						ParseNamespace(true);
						continue;
					}

					PushParserToken(NextToken);
				}
			}

			if (IsIdentifier(Token, L"using"))
			{
				ParseUsing();
				continue;
			}

			if (IsIdentifier(Token, L"typedef"))
			{
				ParseTypedef();
				continue;
			}

			if (IsIdentifier(Token, L"template"))
			{
				if (!ParseTemplateDeclaration())
				{
					throw TextTokenizerError(TEXT("Could not parse template declaration"), Token, CurrentFile());
				}
				continue;
			}

			if (IsIdentifier(Token, L"namespace"))
			{
				ParseNamespace(false);
				continue;
			}

			if (IsIdentifier(Token, L"concept"))
			{
				ParseConceptDeclaration();
				continue;
			}

			if (IsAttributeStart(Token))
			{
				PushParserToken(Token);
				Array<ParsedAttribute> Attributes = ParseAttributes();
				const bool HasToken = GetParserToken(Token);
				if (HasToken && IsIdentifier(Token, L"class"))
				{
					if (!ParseVariableDeclaration(Token, Attributes))
					{
						ParseClass(EClassType::Class, Attributes);
					}
				}
				else if (HasToken && IsIdentifier(Token, L"struct"))
				{
					if (!ParseVariableDeclaration(Token, Attributes))
					{
						ParseClass(EClassType::Struct, Attributes);
					}
				}
				else if (HasToken && IsIdentifier(Token, L"union"))
				{
					if (!ParseVariableDeclaration(Token, Attributes))
					{
						ParseClass(EClassType::Union, Attributes);
					}
				}
				else if (HasToken && IsIdentifier(Token, L"enum"))
				{
					ParseEnum(Attributes);
				}
				else if (HasToken && IsIdentifier(Token, L"using"))
				{
					ParseUsing(Attributes);
				}
				else if (HasToken && IsIdentifier(Token, L"typedef"))
				{
					ParseTypedef(Attributes);
				}
				else if (HasToken && IsIdentifier(Token, L"concept"))
				{
					ParseConceptDeclaration(Attributes);
				}
				else if (HasToken && IsVariableStartToken(Token) && IsCurrentScopeTokenDepth(m_BraceDepth, m_NamespaceDepths, m_ParsedScopeDepths) && ParseVariableDeclaration(Token, Attributes))
				{
				}
				else if (HasToken)
				{
					PushParserToken(Token);
				}
				continue;
			}

			if (IsIdentifier(Token, L"class"))
			{
				if (!ParseVariableDeclaration(Token))
				{
					ParseClass(EClassType::Class);
				}
				continue;
			}

			if (IsIdentifier(Token, L"struct"))
			{
				if (!ParseVariableDeclaration(Token))
				{
					ParseClass(EClassType::Struct);
				}
				continue;
			}

			if (IsIdentifier(Token, L"union"))
			{
				if (!ParseVariableDeclaration(Token))
				{
					ParseClass(EClassType::Union);
				}
				continue;
			}

			if (IsIdentifier(Token, L"public") || IsIdentifier(Token, L"protected") || IsIdentifier(Token, L"private"))
			{
				TextToken NextToken;
				const bool HasNextToken = GetParserToken(NextToken);
				if (HasNextToken && IsSymbol(NextToken, L":"))
				{
					EAccessSpecifier Access = EAccessSpecifier::Private;
					if (IsIdentifier(Token, L"public"))
					{
						Access = EAccessSpecifier::Public;
					}
					else if (IsIdentifier(Token, L"protected"))
					{
						Access = EAccessSpecifier::Protected;
					}

					if (!OnParsed_AccessSpecifier(Access))
					{
						throw TextTokenizerError(TEXT("OnParsed_AccessSpecifier returned false"), Token, CurrentFile());
					}
				}
				else if (HasNextToken)
				{
					PushParserToken(NextToken);
				}
				continue;
			}

			if (IsIdentifier(Token, L"enum"))
			{
				ParseEnum();
				continue;
			}

			if (IsSymbol(Token, L"{"))
			{
				++m_BraceDepth;
			}
			else if (IsSymbol(Token, L"}"))
			{
				if (m_NamespaceDepths.Size() > 0 && m_NamespaceDepths[m_NamespaceDepths.Size() - 1] == m_BraceDepth)
				{
					if (!OnParsed_ScopeEnd())
					{
						throw TextTokenizerError(TEXT("OnParsed_ScopeEnd returned false"), Token, CurrentFile());
					}
					m_NamespaceDepths.RemoveAt(m_NamespaceDepths.Size() - 1);
				}
				else if (m_ParsedScopeDepths.Size() > 0 && m_ParsedScopeDepths[m_ParsedScopeDepths.Size() - 1] == m_BraceDepth)
				{
					ParsedClass ClosedClass = m_ParsedScopeClasses[m_ParsedScopeClasses.Size() - 1];
					Array<TextToken> TypePrefixTokens = m_ParsedScopeTypePrefixTokens[m_ParsedScopeTypePrefixTokens.Size() - 1];
					if (!OnParsed_ScopeEnd())
					{
						throw TextTokenizerError(TEXT("OnParsed_ScopeEnd returned false"), Token, CurrentFile());
					}
					m_ParsedScopeDepths.RemoveAt(m_ParsedScopeDepths.Size() - 1);
					m_ParsedScopeClasses.RemoveAt(m_ParsedScopeClasses.Size() - 1);
					m_ParsedScopeTypePrefixTokens.RemoveAt(m_ParsedScopeTypePrefixTokens.Size() - 1);

					if (m_BraceDepth > 0)
					{
						--m_BraceDepth;
					}

					ParseTrailingClassDeclarator(ClosedClass, TypePrefixTokens);
					continue;
				}

				if (m_BraceDepth > 0)
				{
					--m_BraceDepth;
				}
			}
			else if (IsVariableStartToken(Token) && IsCurrentScopeTokenDepth(m_BraceDepth, m_NamespaceDepths, m_ParsedScopeDepths) && ParseVariableDeclaration(Token))
			{
				continue;
			}
		}

		OnParseEnd();
	}

	bool CppParser::GetParserToken(TextToken& Token)
	{
		if (m_TokenBuffer.Size() > 0)
		{
			Token = m_TokenBuffer[m_TokenBuffer.Size() - 1];
			m_TokenBuffer.RemoveAt(m_TokenBuffer.Size() - 1);
			return true;
		}

		return GetToken(Token);
	}

	void CppParser::PushParserToken(const TextToken& Token)
	{
		m_TokenBuffer.Add(Token);
	}

	bool CppParser::ParseNamespace(bool IsInline)
	{
		Array<ParsedAttribute> Attributes = ParseAttributes();

		TextToken Token;
		if (!GetParserToken(Token))
		{
			throw TextTokenizerError(TEXT("Expected namespace name or body"), CurrentTokenizer(), 0, CurrentFile());
		}

		Array<ParsedNamespace> Namespaces;
		if (!IsSymbol(Token, L"{"))
		{
			if (Token.Type != ETextTokenType::Identifier)
			{
				throw TextTokenizerError(TEXT("Expected namespace name or body"), Token, CurrentFile());
			}

			bool CurrentIsInline = IsInline;
			while (true)
			{
				ParsedNamespace& Namespace = Namespaces.EmplaceRef();
				Namespace.Name = MakeName(Token.Value_Text);
				Namespace.IsInline = CurrentIsInline;
				if (Namespaces.Size() == 1)
				{
					Namespace.Attributes = Attributes;
				}

				if (!GetParserToken(Token))
				{
					throw TextTokenizerError(TEXT("Expected namespace body"), CurrentTokenizer(), 0, CurrentFile());
				}

				if (Namespaces.Size() == 1 && IsSymbol(Token, L"="))
				{
					ParsedNamespaceAlias Alias;
					Alias.Attributes = Namespace.Attributes;
					Alias.Name = Namespace.Name;

					const Array<TextToken> TargetTokens = ReadDeclarationTokensUntilSemicolon();
					if (TargetTokens.Size() == 0)
					{
						throw TextTokenizerError(TEXT("Expected namespace alias target"), Token, CurrentFile());
					}
					Alias.Target = MakeTextName(TokensToText(TargetTokens, 0, TargetTokens.Size()));

					if (!OnParsed_NamespaceAlias(Alias))
					{
						throw TextTokenizerError(TEXT("OnParsed_NamespaceAlias returned false"), Token, CurrentFile());
					}
					return false;
				}

				PushParserToken(Token);
				if (!ReadScopeSeparator())
				{
					break;
				}

				if (!GetParserToken(Token))
				{
					throw TextTokenizerError(TEXT("Expected nested namespace name"), CurrentTokenizer(), 0, CurrentFile());
				}

				CurrentIsInline = false;
				if (IsIdentifier(Token, L"inline"))
				{
					CurrentIsInline = true;
					if (!GetParserToken(Token))
					{
						throw TextTokenizerError(TEXT("Expected nested inline namespace name"), CurrentTokenizer(), 0, CurrentFile());
					}
				}

				if (Token.Type != ETextTokenType::Identifier)
				{
					throw TextTokenizerError(TEXT("Expected nested namespace name"), Token, CurrentFile());
				}
			}

			if (!GetParserToken(Token))
			{
				throw TextTokenizerError(TEXT("Expected namespace body"), CurrentTokenizer(), 0, CurrentFile());
			}

			if (!IsSymbol(Token, L"{"))
			{
				throw TextTokenizerError(TEXT("Expected namespace body"), Token, CurrentFile());
			}
		}

		if (Namespaces.Size() == 0 && (Attributes.Size() > 0 || IsInline))
		{
			ParsedNamespace& Namespace = Namespaces.EmplaceRef();
			Namespace.Attributes = Attributes;
			Namespace.IsInline = IsInline;
		}

		if (!OnParsed_Namespace(Namespaces))
		{
			throw TextTokenizerError(TEXT("OnParsed_Namespace returned false"), Token, CurrentFile());
		}

		++m_BraceDepth;
		m_NamespaceDepths.Add(m_BraceDepth);
		return true;
	}

	bool CppParser::ParseClass(EClassType Type, Array<ParsedAttribute> LeadingAttributes)
	{
		ParsedClass Class;
		Class.Type = Type;
		Class.Attributes = LeadingAttributes;

		TextToken Token;
		while (GetParserToken(Token))
		{
			if (IsAttributeStart(Token))
			{
				PushParserToken(Token);
				Class.Attributes.Add(ParseAttributes());
				continue;
			}

			break;
		}

		if (Token.Type == ETextTokenType::Identifier)
		{
			Class.Name = ParseQualifiedName(Token, Token);
		}

		while (true)
		{
			if (IsIdentifier(Token, L"final"))
			{
				Class.IsFinal = true;
			}
			else if (IsSymbol(Token, L":"))
			{
				while (GetParserToken(Token))
				{
					ParsedBaseClass BaseClass;
					String TypeText;
					size_t ParenDepth = 0;
					size_t BracketDepth = 0;
					size_t AngleDepth = 0;
					while (true)
					{
						if (IsIdentifier(Token, L"public") && TypeText.Size() == 0)
						{
							BaseClass.Access = EAccessSpecifier::Public;
						}
						else if (IsIdentifier(Token, L"protected") && TypeText.Size() == 0)
						{
							BaseClass.Access = EAccessSpecifier::Protected;
						}
						else if (IsIdentifier(Token, L"private") && TypeText.Size() == 0)
						{
							BaseClass.Access = EAccessSpecifier::Private;
						}
						else if (IsIdentifier(Token, L"virtual") && TypeText.Size() == 0)
						{
							BaseClass.IsVirtual = true;
						}
						else if ((IsSymbol(Token, L",") || IsSymbol(Token, L"{") || IsSymbol(Token, L";")) && ParenDepth == 0 && BracketDepth == 0 && AngleDepth == 0)
						{
							break;
						}
						else
						{
							if (IsSymbol(Token, L"(")) { ++ParenDepth; }
							else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
							else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
							else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
							else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
							else if (IsSymbol(Token, L">>") && AngleDepth > 1) { AngleDepth -= 2; }
							else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
							AppendTokenText(TypeText, Token);
						}

						if (!GetParserToken(Token))
						{
							break;
						}
					}

					TypeText.Trim();
					if (TypeText.Size() > 0)
					{
						BaseClass.Type.Name.Segments.EmplaceRef().Name = TypeText;
						Class.BaseClasses.Add(BaseClass);
					}

					if (!IsSymbol(Token, L","))
					{
						break;
					}
				}

				if (IsSymbol(Token, L"{"))
				{
					Class.HasDefinition = true;
				}
			}
			else if (IsSymbol(Token, L"{"))
			{
				Class.HasDefinition = true;
				break;
			}
			else if (IsSymbol(Token, L";"))
			{
				Class.IsForward = Class.Name.Segments.Size() > 0;
				break;
			}
			else
			{
				return false;
			}

			if (Class.HasDefinition || Class.IsForward || !GetParserToken(Token))
			{
				break;
			}
		}

		if (!Class.HasDefinition && !Class.IsForward)
		{
			return false;
		}

		if (!OnParsed_Class(Class))
		{
			throw TextTokenizerError(TEXT("OnParsed_Class returned false"), Token, CurrentFile());
		}

		if (Class.HasDefinition)
		{
			++m_BraceDepth;
			m_ParsedScopeDepths.Add(m_BraceDepth);
			m_ParsedScopeClasses.Add(Class);
			m_ParsedScopeTypePrefixTokens.Add(m_PendingClassTypePrefixTokens);
			m_PendingClassTypePrefixTokens.Clear();
		}
		return true;
	}

	bool CppParser::ParseEnum(Array<ParsedAttribute> LeadingAttributes)
	{
		ParsedEnum Enum;
		Enum.Attributes = LeadingAttributes;

		TextToken Token;
		if (!GetParserToken(Token))
		{
			throw TextTokenizerError(TEXT("Expected enum name or body"), CurrentTokenizer(), 0, CurrentFile());
		}

		if (IsIdentifier(Token, L"class") || IsIdentifier(Token, L"struct"))
		{
			Enum.IsScoped = true;
		}
		else
		{
			PushParserToken(Token);
		}

		Enum.Attributes.Add(ParseAttributes());
		if (!GetParserToken(Token))
		{
			throw TextTokenizerError(TEXT("Expected enum name or body"), CurrentTokenizer(), 0, CurrentFile());
		}

		if (Token.Type == ETextTokenType::Identifier)
		{
			Enum.Name = ParseQualifiedName(Token, Token);
		}

		if (IsSymbol(Token, L":"))
		{
			Enum.UnderlyingType = ParseTypeUntil(Token);
		}

		if (IsSymbol(Token, L";"))
		{
			Enum.IsOpaque = true;
			if (!OnParsed_Enum(Enum))
			{
				throw TextTokenizerError(TEXT("OnParsed_Enum returned false"), Token, CurrentFile());
			}
			return true;
		}

		if (!IsSymbol(Token, L"{"))
		{
			throw TextTokenizerError(TEXT("Expected enum body or opaque declaration"), Token, CurrentFile());
		}

		if (!OnParsed_Enum(Enum))
		{
			throw TextTokenizerError(TEXT("OnParsed_Enum returned false"), Token, CurrentFile());
		}

		while (GetParserToken(Token))
		{
			if (IsSymbol(Token, L"}"))
			{
				break;
			}

			if (IsSymbol(Token, L","))
			{
				continue;
			}

			if (!ParseEnumValue(Token, Token))
			{
				break;
			}
		}

		if (!OnParsed_ScopeEnd())
		{
			throw TextTokenizerError(TEXT("OnParsed_ScopeEnd returned false"), Token, CurrentFile());
		}

		if (GetParserToken(Token) && !IsSymbol(Token, L";"))
		{
			PushParserToken(Token);
		}
		return true;
	}

	bool CppParser::ParseUsing(Array<ParsedAttribute> LeadingAttributes)
	{
		const Array<TextToken> Tokens = ReadDeclarationTokensUntilSemicolon();
		if (Tokens.Size() == 0)
		{
			return false;
		}

		if (IsIdentifier(Tokens[0], L"namespace"))
		{
			ParsedUsing Using;
			Using.Kind = ParsedUsing::EKind::UsingDirective;
			Using.Attributes = LeadingAttributes;
			Using.Target = MakeTextName(TokensToText(Tokens, 1, Tokens.Size()));
			if (!OnParsed_Using(Using))
			{
				throw TextTokenizerError(TEXT("OnParsed_Using returned false"), Tokens[0], CurrentFile());
			}
			return true;
		}

		if (IsIdentifier(Tokens[0], L"enum"))
		{
			ParsedUsing Using;
			Using.Kind = ParsedUsing::EKind::UsingEnum;
			Using.Attributes = LeadingAttributes;
			Using.Target = MakeTextName(TokensToText(Tokens, 1, Tokens.Size()));
			if (!OnParsed_Using(Using))
			{
				throw TextTokenizerError(TEXT("OnParsed_Using returned false"), Tokens[0], CurrentFile());
			}
			return true;
		}

		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		size_t EqualIndex = Tokens.Size();
		for (size_t Index = 0; Index < Tokens.Size(); ++Index)
		{
			const TextToken& Token = Tokens[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Tokens, Index);
			if (DecltypeEnd < Tokens.Size())
			{
				Index = DecltypeEnd;
				continue;
			}

			if (IsSymbol(Token, L"(") || IsSymbol(Token, L"[")) { ++ParenDepth; }
			else if ((IsSymbol(Token, L")") || IsSymbol(Token, L"]")) && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if (IsSymbol(Token, L"]]") && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
			else if (IsSymbol(Token, L"=") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				EqualIndex = Index;
				break;
			}
		}

		if (EqualIndex < Tokens.Size())
		{
			ParsedUsing Using;
			Using.Kind = ParsedUsing::EKind::AliasDeclaration;
			Using.Attributes = LeadingAttributes;
			Using.Name = MakeTextName(TokensToText(Tokens, 0, EqualIndex));
			Using.Type = MakeTextType(TokensToText(Tokens, EqualIndex + 1, Tokens.Size()));
			if (!OnParsed_Using(Using))
			{
				throw TextTokenizerError(TEXT("OnParsed_Using returned false"), Tokens[0], CurrentFile());
			}
			return true;
		}

		for (const Array<TextToken>& Declarator : SplitTopLevelCommas(Tokens))
		{
			ParsedUsing Using;
			Using.Kind = ParsedUsing::EKind::UsingDeclaration;
			Using.Attributes = LeadingAttributes;
			Using.Target = MakeTextName(TokensToText(Declarator, 0, Declarator.Size()));
			if (!OnParsed_Using(Using))
			{
				throw TextTokenizerError(TEXT("OnParsed_Using returned false"), Tokens[0], CurrentFile());
			}
		}
		return true;
	}

	bool CppParser::ParseTypedef(Array<ParsedAttribute> LeadingAttributes)
	{
		const Array<TextToken> Tokens = ReadDeclarationTokensUntilSemicolon();
		if (Tokens.Size() == 0)
		{
			return false;
		}

		const Array<Array<TextToken>> Declarators = SplitTopLevelCommas(Tokens);
		if (Declarators.Size() == 0)
		{
			return false;
		}

		const size_t FirstDeclaratorStart = FindTypedefDeclaratorStart(Declarators[0]);
		const String TypeText = TokensToText(Declarators[0], 0, FirstDeclaratorStart);
		const bool UseFirstAliasTypeForLaterDeclarators = HasTopLevelClassBody(Declarators[0]) && FirstDeclaratorStart + 1 == Declarators[0].Size() && Declarators[0][FirstDeclaratorStart].Type == ETextTokenType::Identifier;
		const String FirstAliasTypeText = UseFirstAliasTypeForLaterDeclarators ? Declarators[0][FirstDeclaratorStart].Value_Text : String();
		for (size_t Index = 0; Index < Declarators.Size(); ++Index)
		{
			const Array<TextToken>& Declarator = Declarators[Index];
			const size_t DeclaratorStart = Index == 0 ? FirstDeclaratorStart : 0;
			ParsedUsing Using;
			Using.Kind = ParsedUsing::EKind::Typedef;
			Using.Attributes = LeadingAttributes;
			if (Index > 0 && UseFirstAliasTypeForLaterDeclarators)
			{
				const size_t NameIndex = FindVariableNameIndex(Declarator, 0, FindVariableDeclaratorEnd(Declarator));
				if (NameIndex < Declarator.Size())
				{
					Array<TextToken> TypeTokens;
					TypeTokens.Add(MakeIdentifierToken(FirstAliasTypeText));
					for (size_t TypeIndex = 0; TypeIndex < NameIndex; ++TypeIndex)
					{
						TypeTokens.Add(Declarator[TypeIndex]);
					}
					Using.Type = MakeTextType(TokensToText(TypeTokens, 0, TypeTokens.Size()));
					Using.Name = MakeTextName(TokensToText(Declarator, NameIndex, Declarator.Size()));
				}
				else
				{
					Using.Type = MakeTextType(FirstAliasTypeText);
					Using.Name = MakeTextName(TokensToText(Declarator, 0, Declarator.Size()));
				}
			}
			else
			{
				Using.Type = MakeTextType(TypeText);
				Using.Name = MakeTextName(TokensToText(Declarator, DeclaratorStart, Declarator.Size()));
			}
			if (!OnParsed_Using(Using))
			{
				throw TextTokenizerError(TEXT("OnParsed_Using returned false"), Tokens[0], CurrentFile());
			}
		}
		return true;
	}

	bool CppParser::ParseVariableDeclaration(const TextToken& FirstToken, Array<ParsedAttribute> LeadingAttributes)
	{
		bool HasSemicolon = false;
		const Array<TextToken> Tokens = ReadVariableDeclarationTokens(FirstToken, HasSemicolon);
		if (!HasSemicolon || Tokens.Size() == 0)
		{
			const size_t NameIndex = FindVariableNameIndex(Tokens, 0, FindVariableDeclaratorEnd(Tokens));
			if (Tokens.Size() > 0 && NameIndex < Tokens.Size() && IsFunctionDeclarator(Tokens, NameIndex))
			{
				Array<TextToken> EmptyBaseTypeTokens;
				if (ParseFunctionDeclaration(Tokens, EmptyBaseTypeTokens, LeadingAttributes, true))
				{
					SkipFunctionDefinitionBody();
					return true;
				}
			}

			PushParserTokensAfterFirst(Tokens);
			return false;
		}

		if (HasTopLevelClassBody(Tokens))
		{
			Array<TextToken> TypePrefixTokens;
			for (const TextToken& Token : Tokens)
			{
				if (IsIdentifier(Token, L"class") || IsIdentifier(Token, L"struct") || IsIdentifier(Token, L"union"))
				{
					break;
				}

				TypePrefixTokens.Add(Token);
			}

			if (TypePrefixTokens.Size() > 0 || m_PendingClassTypePrefixTokens.Size() == 0)
			{
				m_PendingClassTypePrefixTokens = TypePrefixTokens;
			}

			PushParserTokensAfterFirst(Tokens);
			return false;
		}

		if (IsVariableDeclarationBlocked(Tokens))
		{
			PushParserTokensAfterFirst(Tokens);
			return false;
		}

		if (IsIdentifier(Tokens[0], L"friend"))
		{
			bool HasFunction = false;
			for (const Array<TextToken>& Declarator : SplitTopLevelCommas(Tokens))
			{
				const size_t NameIndex = FindVariableNameIndex(Declarator, 0, FindVariableDeclaratorEnd(Declarator));
				HasFunction = NameIndex < Declarator.Size() && IsFunctionDeclarator(Declarator, NameIndex);
				if (HasFunction)
				{
					break;
				}
			}
			if (!HasFunction)
			{
				if (ParseFriendClassDeclaration(Tokens, LeadingAttributes))
				{
					return true;
				}
				return true;
			}
		}

		Array<TextToken> DeclarationTokens;
		const size_t TokenEnd = IsSymbol(Tokens[Tokens.Size() - 1], L";") ? Tokens.Size() - 1 : Tokens.Size();
		for (size_t Index = 0; Index < TokenEnd; ++Index)
		{
			DeclarationTokens.Add(Tokens[Index]);
		}

		const Array<Array<TextToken>> Declarators = SplitTopLevelCommas(DeclarationTokens);
		if (Declarators.Size() == 0 || Declarators[0].Size() == 0)
		{
			PushParserTokensAfterFirst(Tokens);
			return false;
		}

		const size_t FirstNameIndex = FindVariableNameIndex(Declarators[0], 0, FindVariableDeclaratorEnd(Declarators[0]));
		if (FirstNameIndex >= Declarators[0].Size())
		{
			PushParserTokensAfterFirst(Tokens);
			return false;
		}
		if (IsFunctionDeclarator(Declarators[0], FirstNameIndex))
		{
			const size_t FirstDeclaratorTypeBegin = FindVariableDeclaratorTypeBegin(Declarators[0], FirstNameIndex);
			Array<TextToken> BaseTypeTokens;
			for (size_t Index = 0; Index < FirstDeclaratorTypeBegin; ++Index)
			{
				BaseTypeTokens.Add(Declarators[0][Index]);
			}

			for (size_t DeclaratorIndex = 0; DeclaratorIndex < Declarators.Size(); ++DeclaratorIndex)
			{
				const Array<TextToken>& Declarator = Declarators[DeclaratorIndex];
				const size_t NameIndex = DeclaratorIndex == 0 ? FirstNameIndex : FindVariableNameIndex(Declarator, 0, FindVariableDeclaratorEnd(Declarator));
				if (NameIndex < Declarator.Size() && IsFunctionDeclarator(Declarator, NameIndex))
				{
					ParseFunctionDeclaration(Declarator, DeclaratorIndex == 0 ? Array<TextToken>() : BaseTypeTokens, LeadingAttributes, false);
				}
			}
			return true;
		}

		const size_t FirstDeclaratorTypeBegin = FindVariableDeclaratorTypeBegin(Declarators[0], FirstNameIndex);
		Array<TextToken> BaseTypeTokens;
		for (size_t Index = 0; Index < FirstDeclaratorTypeBegin; ++Index)
		{
			BaseTypeTokens.Add(Declarators[0][Index]);
		}

		bool ParsedAny = false;
		bool SkippedFunction = false;
		for (size_t DeclaratorIndex = 0; DeclaratorIndex < Declarators.Size(); ++DeclaratorIndex)
		{
			const Array<TextToken>& Declarator = Declarators[DeclaratorIndex];
			const size_t NameIndex = DeclaratorIndex == 0 ? FirstNameIndex : FindVariableNameIndex(Declarator, 0, FindVariableDeclaratorEnd(Declarator));
			if (NameIndex >= Declarator.Size())
			{
				continue;
			}
			if (IsFunctionDeclarator(Declarator, NameIndex))
			{
				ParseFunctionDeclaration(Declarator, BaseTypeTokens, LeadingAttributes, false);
				SkippedFunction = true;
				continue;
			}

			size_t EndIndex = Declarator.Size();
			size_t InitializerIndex = Declarator.Size();
			size_t BitfieldIndex = Declarator.Size();
			size_t ParenDepth = 0;
			size_t BracketDepth = 0;
			size_t BraceDepth = 0;
			for (size_t Index = NameIndex + 1; Index < Declarator.Size(); ++Index)
			{
				const TextToken& Token = Declarator[Index];
				if (IsSymbol(Token, L"=") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0)
				{
					InitializerIndex = Index;
					EndIndex = Index;
					break;
				}
				if (IsSymbol(Token, L":") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0)
				{
					BitfieldIndex = Index;
					EndIndex = Index;
					break;
				}
				if (IsSymbol(Token, L"{") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0)
				{
					InitializerIndex = Index;
					EndIndex = Index;
					break;
				}

				if (IsSymbol(Token, L"(")) { ++ParenDepth; }
				else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
				else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
				else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
				else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
				else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			}

			Array<TextToken> TypeTokens = BaseTypeTokens;
			const size_t DeclaratorTypeBegin = DeclaratorIndex == 0 ? FirstDeclaratorTypeBegin : FindVariableDeclaratorTypeBegin(Declarator, NameIndex);
			for (size_t Index = DeclaratorTypeBegin; Index < NameIndex; ++Index)
			{
				TypeTokens.Add(Declarator[Index]);
			}

			ParsedVariable Variable;
			Variable.Attributes = LeadingAttributes;
			Variable.Type = ParseVariableType(TypeTokens);
			Variable.Name = Declarator[NameIndex].Value_Text;
			for (const TextToken& Token : TypeTokens)
			{
				Variable.IsConstexpr = Variable.IsConstexpr || IsIdentifier(Token, L"constexpr");
				Variable.IsConsteval = Variable.IsConsteval || IsIdentifier(Token, L"consteval");
				Variable.IsStatic = Variable.IsStatic || IsIdentifier(Token, L"static");
				Variable.IsThreadLocal = Variable.IsThreadLocal || IsIdentifier(Token, L"thread_local");
				Variable.IsExtern = Variable.IsExtern || IsIdentifier(Token, L"extern");
				Variable.IsMutable = Variable.IsMutable || IsIdentifier(Token, L"mutable");
			}

			if (InitializerIndex < Declarator.Size())
			{
				Variable.HasInitializer = true;
				Variable.Initializer = MakeTextExpression(Declarator, IsSymbol(Declarator[InitializerIndex], L"=") ? InitializerIndex + 1 : InitializerIndex, Declarator.Size());
			}
			if (BitfieldIndex < Declarator.Size())
			{
				Variable.IsBitfield = true;
				Variable.BitfieldSize = MakeTextExpression(Declarator, BitfieldIndex + 1, Declarator.Size());
			}

			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(TypeTokens, 0);
			if (DecltypeEnd < TypeTokens.Size())
			{
				ParsedDecltype Decltype;
				Decltype.Expression = MakeTextExpression(TypeTokens, 2, DecltypeEnd);
				Decltype.Name = Variable.Name;
				Decltype.Initializer = Variable.Initializer;
				Decltype.Attributes = Variable.Attributes;
				Decltype.HasInitializer = Variable.HasInitializer;
				Decltype.IsConstexpr = Variable.IsConstexpr;
				Decltype.IsConsteval = Variable.IsConsteval;
				Decltype.IsStatic = Variable.IsStatic;
				Decltype.IsThreadLocal = Variable.IsThreadLocal;
				Decltype.IsMutable = Variable.IsMutable;
				Decltype.IsExtern = Variable.IsExtern;
				if (!OnParsed_Decltype(Decltype))
				{
					throw TextTokenizerError(TEXT("OnParsed_Decltype returned false"), Declarator[NameIndex], CurrentFile());
				}
			}
			else if (!OnParsed_Variable(Variable))
			{
				throw TextTokenizerError(TEXT("OnParsed_Variable returned false"), Declarator[NameIndex], CurrentFile());
			}
			ParsedAny = true;
		}

		if (!ParsedAny && !SkippedFunction)
		{
			PushParserTokensAfterFirst(Tokens);
		}
		return ParsedAny || SkippedFunction;
	}

	bool CppParser::ParseFriendClassDeclaration(const Array<TextToken>& Tokens, Array<ParsedAttribute> LeadingAttributes)
	{
		if (Tokens.Size() < 4 || !IsIdentifier(Tokens[0], L"friend"))
		{
			return false;
		}

		ParsedClass Class;
		if (IsIdentifier(Tokens[1], L"class"))
		{
			Class.Type = EClassType::Class;
		}
		else if (IsIdentifier(Tokens[1], L"struct"))
		{
			Class.Type = EClassType::Struct;
		}
		else if (IsIdentifier(Tokens[1], L"union"))
		{
			Class.Type = EClassType::Union;
		}
		else
		{
			return false;
		}

		const size_t NameEnd = IsSymbol(Tokens[Tokens.Size() - 1], L";") ? Tokens.Size() - 1 : Tokens.Size();
		if (NameEnd <= 2)
		{
			return false;
		}

		Class.Attributes = LeadingAttributes;
		Class.Name = MakeTextName(TokensToText(Tokens, 2, NameEnd));
		Class.IsFriend = true;
		if (!OnParsed_Class(Class))
		{
			throw TextTokenizerError(TEXT("OnParsed_Class returned false"), Tokens[0], CurrentFile());
		}
		return true;
	}

	bool CppParser::ParseFunctionDeclaration(const Array<TextToken>& Declarator, const Array<TextToken>& BaseTypeTokens, Array<ParsedAttribute> LeadingAttributes, bool HasDefinition)
	{
		size_t NameIndex = FindVariableNameIndex(Declarator, 0, FindVariableDeclaratorEnd(Declarator));
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = 0; Index < Declarator.Size(); ++Index)
		{
			const TextToken& Token = Declarator[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Declarator, Index);
			if (DecltypeEnd < Declarator.Size())
			{
				Index = DecltypeEnd;
				continue;
			}

			if (IsSymbol(Token, L"(") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0 && Index > 0 && Declarator[Index - 1].Type == ETextTokenType::Identifier)
			{
				NameIndex = Index - 1;
				break;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
		}
		if (NameIndex >= Declarator.Size())
		{
			return false;
		}

		const size_t ParameterOpen = FindFunctionParameterListOpen(Declarator, NameIndex);
		const size_t ParameterClose = FindMatchingSymbol(Declarator, ParameterOpen, L"(", L")");
		if (ParameterOpen >= Declarator.Size() || ParameterClose >= Declarator.Size())
		{
			return false;
		}

		size_t NameBegin = NameIndex;
		while (NameBegin >= 2 && IsSymbol(Declarator[NameBegin - 1], L":") && IsSymbol(Declarator[NameBegin - 2], L":"))
		{
			NameBegin -= 2;
			if (NameBegin == 0 || Declarator[NameBegin - 1].Type != ETextTokenType::Identifier)
			{
				break;
			}
			--NameBegin;
		}
		if (NameBegin >= 1 && IsDestructorToken(Declarator[NameBegin - 1]))
		{
			--NameBegin;
		}

		Array<TextToken> PrefixTokens = BaseTypeTokens;
		for (size_t Index = 0; Index < NameBegin; ++Index)
		{
			PrefixTokens.Add(Declarator[Index]);
		}

		ParsedFunction Function;
		Function.Attributes = LeadingAttributes;
		Function.HasDefinition = HasDefinition;
		Function.Name = MakeTextName(TokensToText(Declarator, IsDestructorToken(Declarator[NameBegin]) ? NameBegin + 1 : NameBegin, NameIndex + 1));
		Function.Type = IsDestructorToken(Declarator[NameBegin]) ? ParsedFunction::EType::Destructor : ParsedFunction::EType::Function;

		Array<TextToken> ReturnTypeTokens;
		bool IsExplicit = false;
		for (size_t Index = 0; Index < PrefixTokens.Size(); ++Index)
		{
			const TextToken& Token = PrefixTokens[Index];
			if (IsIdentifier(Token, L"friend")) { Function.IsFriend = true; continue; }
			if (IsIdentifier(Token, L"virtual")) { Function.IsVirtual = true; continue; }
			if (IsIdentifier(Token, L"static")) { Function.IsStatic = true; continue; }
			if (IsIdentifier(Token, L"inline")) { Function.IsInline = true; continue; }
			if (IsIdentifier(Token, L"constexpr")) { Function.IsConstexpr = true; continue; }
			if (IsIdentifier(Token, L"consteval")) { Function.IsConsteval = true; continue; }
			if (IsIdentifier(Token, L"explicit"))
			{
				IsExplicit = true;
				if (Index + 1 < PrefixTokens.Size() && IsSymbol(PrefixTokens[Index + 1], L"("))
				{
					Index = FindMatchingSymbol(PrefixTokens, Index + 1, L"(", L")");
				}
				continue;
			}
			if (!IsFunctionSpecifier(Token))
			{
				ReturnTypeTokens.Add(Token);
			}
		}

		const String FunctionNameText = TokensToText(Declarator, NameBegin, NameIndex + 1);
		String CurrentClassName;
		if (m_ParsedScopeClasses.Size() > 0)
		{
			CurrentClassName = NameToText(m_ParsedScopeClasses[m_ParsedScopeClasses.Size() - 1].Name);
		}
		if (Function.Type != ParsedFunction::EType::Destructor && ReturnTypeTokens.Size() == 0 && (FunctionNameText == CurrentClassName || (NameIndex >= 2 && Declarator[NameIndex - 1].Value_Text == L":" && Declarator[NameIndex - 2].Value_Text == L":")))
		{
			Function.Type = ParsedFunction::EType::Constructor;
		}
		else if (Function.Type == ParsedFunction::EType::Function)
		{
			Function.ReturnType = ParseVariableType(ReturnTypeTokens);
		}

		const Array<TextToken> ParameterTokens = [&]()
		{
			Array<TextToken> Result;
			for (size_t Index = ParameterOpen + 1; Index < ParameterClose; ++Index)
			{
				Result.Add(Declarator[Index]);
			}
			return Result;
		}();

		for (const Array<TextToken>& Parameter : SplitTopLevelCommas(ParameterTokens))
		{
			if (Parameter.Size() == 0 || (Parameter.Size() == 1 && IsIdentifier(Parameter[0], L"void")))
			{
				continue;
			}
			if ((Parameter.Size() == 1 && IsSymbol(Parameter[0], L"...")) || (Parameter.Size() == 3 && IsSymbol(Parameter[0], L".") && IsSymbol(Parameter[1], L".") && IsSymbol(Parameter[2], L".")))
			{
				Function.IsVariadic = true;
				continue;
			}
			Function.Parameters.Add(ParseFunctionParameter(Parameter));
		}

		for (size_t Index = ParameterClose + 1; Index < Declarator.Size(); ++Index)
		{
			const TextToken& Token = Declarator[Index];
			if (IsIdentifier(Token, L"const")) { Function.IsConst = true; continue; }
			if (IsIdentifier(Token, L"volatile")) { Function.IsVolatile = true; continue; }
			if (IsSymbol(Token, L"&")) { Function.RefQualifier = ParsedFunction::ERefQual::LValue; continue; }
			if (IsSymbol(Token, L"&&")) { Function.RefQualifier = ParsedFunction::ERefQual::RValue; continue; }
			if (IsIdentifier(Token, L"override")) { Function.IsOverride = true; continue; }
			if (IsIdentifier(Token, L"final")) { Function.IsFinal = true; continue; }
			if (IsIdentifier(Token, L"noexcept"))
			{
				Function.IsNoExcept = true;
				if (Index + 1 < Declarator.Size() && IsSymbol(Declarator[Index + 1], L"("))
				{
					const size_t Close = FindMatchingSymbol(Declarator, Index + 1, L"(", L")");
					Function.NoExceptExpression = MakeTextExpression(Declarator, Index + 2, Close);
					Index = Close;
				}
				continue;
			}
			if (IsSymbol(Token, L"->") || (IsSymbol(Token, L"-") && Index + 1 < Declarator.Size() && IsSymbol(Declarator[Index + 1], L">")))
			{
				const size_t TypeBegin = IsSymbol(Token, L"->") ? Index + 1 : Index + 2;
				const size_t End = FindTopLevelToken(Declarator, TypeBegin, L"requires");
				Function.TrailingReturnType = MakeTextType(TokensToText(Declarator, TypeBegin, End));
				Index = End - 1;
				continue;
			}
			if (IsIdentifier(Token, L"requires"))
			{
				const size_t End = FindTopLevelRequiresClauseEnd(Declarator, Index + 1);
				Function.RequiresClause = MakeTextExpression(Declarator, Index + 1, End);
				Index = End - 1;
				continue;
			}
			if (IsSymbol(Token, L"=") && Index + 1 < Declarator.Size())
			{
				Function.IsPure = Declarator[Index + 1].RawText == L"0";
				Function.IsDefaulted = IsIdentifier(Declarator[Index + 1], L"default");
				Function.IsDeleted = IsIdentifier(Declarator[Index + 1], L"delete");
				if (Function.IsDeleted && Index + 2 < Declarator.Size() && IsSymbol(Declarator[Index + 2], L"("))
				{
					const size_t Close = FindMatchingSymbol(Declarator, Index + 2, L"(", L")");
					Function.DeletedMessage = MakeTextExpression(Declarator, Index + 3, Close);
				}
				break;
			}
			if (IsSymbol(Token, L":") && Function.Type == ParsedFunction::EType::Constructor)
			{
				break;
			}
		}

		if (Function.Type == ParsedFunction::EType::Constructor)
		{
			ParsedConstructor Constructor;
			Constructor.Name = Function.Name;
			Constructor.Parameters = Function.Parameters;
			Constructor.Attributes = Function.Attributes;
			Constructor.NoExceptExpression = Function.NoExceptExpression;
			Constructor.DeletedMessage = Function.DeletedMessage;
			Constructor.RequiresClause = Function.RequiresClause;
			Constructor.RefQualifier = Function.RefQualifier;
			Constructor.IsExplicit = IsExplicit;
			Constructor.IsVirtual = Function.IsVirtual;
			Constructor.IsOverride = Function.IsOverride;
			Constructor.IsFinal = Function.IsFinal;
			Constructor.IsPure = Function.IsPure;
			Constructor.IsInline = Function.IsInline;
			Constructor.IsConstexpr = Function.IsConstexpr;
			Constructor.IsConsteval = Function.IsConsteval;
			Constructor.IsNoExcept = Function.IsNoExcept;
			Constructor.IsDeleted = Function.IsDeleted;
			Constructor.IsDefaulted = Function.IsDefaulted;
			Constructor.HasDefinition = Function.HasDefinition;
			Constructor.IsFriend = Function.IsFriend;
			Constructor.IsVariadic = Function.IsVariadic;
			if (!OnParsed_Constructor(Constructor))
			{
				throw TextTokenizerError(TEXT("OnParsed_Constructor returned false"), Declarator[NameIndex], CurrentFile());
			}
		}
		else if (Function.Type == ParsedFunction::EType::Destructor)
		{
			ParsedDestructor Destructor;
			Destructor.Name = Function.Name;
			Destructor.Parameters = Function.Parameters;
			Destructor.Attributes = Function.Attributes;
			Destructor.NoExceptExpression = Function.NoExceptExpression;
			Destructor.DeletedMessage = Function.DeletedMessage;
			Destructor.RequiresClause = Function.RequiresClause;
			Destructor.RefQualifier = Function.RefQualifier;
			Destructor.IsVirtual = Function.IsVirtual;
			Destructor.IsOverride = Function.IsOverride;
			Destructor.IsFinal = Function.IsFinal;
			Destructor.IsPure = Function.IsPure;
			Destructor.IsInline = Function.IsInline;
			Destructor.IsConstexpr = Function.IsConstexpr;
			Destructor.IsConsteval = Function.IsConsteval;
			Destructor.IsNoExcept = Function.IsNoExcept;
			Destructor.IsDeleted = Function.IsDeleted;
			Destructor.IsDefaulted = Function.IsDefaulted;
			Destructor.HasDefinition = Function.HasDefinition;
			Destructor.IsFriend = Function.IsFriend;
			if (!OnParsed_Destructor(Destructor))
			{
				throw TextTokenizerError(TEXT("OnParsed_Destructor returned false"), Declarator[NameIndex], CurrentFile());
			}
		}
		else if (!OnParsed_Function(Function))
		{
			throw TextTokenizerError(TEXT("OnParsed_Function returned false"), Declarator[NameIndex], CurrentFile());
		}
		return true;
	}

	ParsedParameter CppParser::ParseFunctionParameter(const Array<TextToken>& Tokens)
	{
		ParsedParameter Parameter;
		size_t Begin = 0;
		if (Tokens.Size() > 0 && IsIdentifier(Tokens[0], L"this"))
		{
			Parameter.IsExplicitObject = true;
			Begin = 1;
		}

		const size_t EqualIndex = FindTopLevelToken(Tokens, Begin, L"=");
		const size_t NameIndex = FindVariableNameIndex(Tokens, Begin, EqualIndex);
		if (NameIndex < EqualIndex && NameIndex > Begin)
		{
			Parameter.Name = Tokens[NameIndex].Value_Text;
			Parameter.Type = ParseVariableType([&]()
			{
				Array<TextToken> TypeTokens;
				for (size_t Index = Begin; Index < NameIndex; ++Index)
				{
					TypeTokens.Add(Tokens[Index]);
				}
				for (size_t Index = NameIndex + 1; Index < EqualIndex; ++Index)
				{
					TypeTokens.Add(Tokens[Index]);
				}
				return TypeTokens;
			}());
		}
		else
		{
			Parameter.Type = MakeTextType(TokensToText(Tokens, Begin, EqualIndex));
		}

		if (EqualIndex < Tokens.Size())
		{
			Parameter.HasDefaultValue = true;
			Parameter.DefaultValue = MakeTextExpression(Tokens, EqualIndex + 1, Tokens.Size());
		}
		return Parameter;
	}

	bool CppParser::ParseTemplateDeclaration()
	{
		TextToken Token;
		if (!GetParserToken(Token))
		{
			throw TextTokenizerError(TEXT("Expected template parameter list"), CurrentTokenizer(), 0, CurrentFile());
		}

		if (!IsSymbol(Token, L"<"))
		{
			throw TextTokenizerError(TEXT("Expected template parameter list"), Token, CurrentFile());
		}

		ParsedTemplateDeclaration Template;
		Array<TextToken> Tokens;
		size_t AngleDepth = 1;
		while (GetParserToken(Token))
		{
			if (IsSymbol(Token, L"<"))
			{
				++AngleDepth;
			}
			else if (IsSymbol(Token, L">") && AngleDepth > 0)
			{
				--AngleDepth;
				if (AngleDepth == 0)
				{
					break;
				}
			}

			Tokens.Add(Token);
		}

		if (AngleDepth > 0)
		{
			throw TextTokenizerError(TEXT("Expected template parameter list end"), CurrentTokenizer(), 0, CurrentFile());
		}

		for (const Array<TextToken>& ParameterTokens : SplitTopLevelCommas(Tokens))
		{
			if (ParameterTokens.Size() > 0)
			{
				Template.Parameters.Add(ParseTemplateParameter(ParameterTokens));
			}
		}

		if (GetParserToken(Token))
		{
			if (IsIdentifier(Token, L"requires"))
			{
				Array<TextToken> RequiresTokens;
				size_t ParenDepth = 0;
				size_t BracketDepth = 0;
				size_t BraceDepth = 0;
				size_t AngleDepth = 0;
				while (GetParserToken(Token))
				{
					const TextToken* LastRequiresToken = RequiresTokens.Size() > 0 ? &RequiresTokens[RequiresTokens.Size() - 1] : nullptr;
					const bool LastTokenEndsClause = LastRequiresToken != nullptr && !IsSymbol(*LastRequiresToken, L"&&") && !IsSymbol(*LastRequiresToken, L"||") && !IsSymbol(*LastRequiresToken, L"!") && !IsSymbol(*LastRequiresToken, L"(") && !IsSymbol(*LastRequiresToken, L",") && !IsIdentifier(*LastRequiresToken, L"and") && !IsIdentifier(*LastRequiresToken, L"or") && !IsIdentifier(*LastRequiresToken, L"not");
					const bool CanEndClause = LastTokenEndsClause && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0;
					const bool IsDeclarationStart = IsVariableStartToken(Token) || IsIdentifier(Token, L"class") || IsIdentifier(Token, L"struct") || IsIdentifier(Token, L"union") || IsIdentifier(Token, L"enum") || IsIdentifier(Token, L"using") || IsIdentifier(Token, L"typedef") || IsIdentifier(Token, L"template") || IsIdentifier(Token, L"concept");
					if (CanEndClause && IsDeclarationStart && !IsIdentifier(Token, L"and") && !IsIdentifier(Token, L"or") && !IsIdentifier(Token, L"not"))
					{
						PushParserToken(Token);
						break;
					}

					if (IsSymbol(Token, L"(") ) { ++ParenDepth; }
					else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
					else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
					else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
					else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
					else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
					else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
					else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }

					RequiresTokens.Add(Token);
				}
				Template.RequiresClause = MakeTextExpression(RequiresTokens, 0, RequiresTokens.Size());
			}
			else
			{
				PushParserToken(Token);
			}
		}

		if (!OnParsed_TemplateDeclaration(Template))
		{
			throw TextTokenizerError(TEXT("OnParsed_TemplateDeclaration returned false"), Token, CurrentFile());
		}
		return true;
	}

	ParsedTemplateParameter CppParser::ParseTemplateParameter(const Array<TextToken>& Tokens)
	{
		ParsedTemplateParameter Parameter;
		size_t EqualIndex = Tokens.Size();
		size_t RequiresIndex = Tokens.Size();
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = 0; Index < Tokens.Size(); ++Index)
		{
			const TextToken& Token = Tokens[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Tokens, Index);
			if (DecltypeEnd < Tokens.Size())
			{
				Index = DecltypeEnd;
				continue;
			}

			if (IsSymbol(Token, L"=") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				EqualIndex = Index;
				break;
			}
			if (IsIdentifier(Token, L"requires") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				RequiresIndex = Index;
				break;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
		}
		const size_t DeclarationEnd = EqualIndex < RequiresIndex ? EqualIndex : RequiresIndex;

		if (IsIdentifier(Tokens[0], L"typename") || IsIdentifier(Tokens[0], L"class"))
		{
			Parameter.Kind = ParsedTemplateParameter::EKind::Type;
			size_t NameIndex = 1;
			if (NameIndex + 2 < DeclarationEnd && IsSymbol(Tokens[NameIndex], L".") && IsSymbol(Tokens[NameIndex + 1], L".") && IsSymbol(Tokens[NameIndex + 2], L"."))
			{
				Parameter.IsVariadic = true;
				NameIndex += 3;
			}

			if (NameIndex < DeclarationEnd)
			{
				if (Tokens[NameIndex].Type != ETextTokenType::Identifier)
				{
					throw TextTokenizerError(TEXT("Expected template type parameter name"), Tokens[NameIndex], CurrentFile());
				}
				Parameter.Name = Tokens[NameIndex].Value_Text;
			}

			if (EqualIndex < Tokens.Size())
			{
				Parameter.HasDefault = true;
				Parameter.DefaultType = MakeTextType(TokensToText(Tokens, EqualIndex + 1, Tokens.Size()));
			}
			if (RequiresIndex < Tokens.Size())
			{
				Parameter.RequiresClause = MakeTextExpression(Tokens, RequiresIndex + 1, Tokens.Size());
			}
			return Parameter;
		}

		const size_t NameIndex = FindVariableNameIndex(Tokens, 0, DeclarationEnd);
		if (NameIndex < DeclarationEnd)
		{
			Parameter.Name = Tokens[NameIndex].Value_Text;
			Parameter.IsVariadic = NameIndex >= 3 && IsSymbol(Tokens[NameIndex - 3], L".") && IsSymbol(Tokens[NameIndex - 2], L".") && IsSymbol(Tokens[NameIndex - 1], L".");

			if (NameIndex + 1 < DeclarationEnd && IsIdentifier(Tokens[NameIndex + 1], L"auto"))
			{
				Parameter.Kind = ParsedTemplateParameter::EKind::NonType;
				Parameter.Constraint = MakeTextName(TokensToText(Tokens, 0, NameIndex));
				Parameter.Type = MakeTextType(TokensToText(Tokens, NameIndex + 1, DeclarationEnd));
			}
			else if (NameIndex > 0 && IsIdentifier(Tokens[NameIndex - 1], L"auto"))
			{
				Parameter.Kind = ParsedTemplateParameter::EKind::NonType;
				Parameter.Constraint = MakeTextName(TokensToText(Tokens, 0, NameIndex - 1));
				Parameter.Type = MakeTextType(TokensToText(Tokens, NameIndex - 1, DeclarationEnd));
			}
			else if (NameIndex > 0)
			{
				Parameter.Kind = ParsedTemplateParameter::EKind::Type;
				Parameter.Constraint = MakeTextName(TokensToText(Tokens, 0, Parameter.IsVariadic ? NameIndex - 3 : NameIndex));
			}
			else
			{
				Parameter.Kind = ParsedTemplateParameter::EKind::NonType;
				Parameter.Type = MakeTextType(TokensToText(Tokens, 0, DeclarationEnd));
			}

			if (EqualIndex < Tokens.Size())
			{
				Parameter.HasDefault = true;
				if (Parameter.Kind == ParsedTemplateParameter::EKind::Type)
				{
					Parameter.DefaultType = MakeTextType(TokensToText(Tokens, EqualIndex + 1, RequiresIndex < Tokens.Size() ? RequiresIndex : Tokens.Size()));
				}
				else
				{
					Parameter.DefaultExpression = MakeTextExpression(Tokens, EqualIndex + 1, RequiresIndex < Tokens.Size() ? RequiresIndex : Tokens.Size());
				}
			}
			if (RequiresIndex < Tokens.Size())
			{
				Parameter.RequiresClause = MakeTextExpression(Tokens, RequiresIndex + 1, Tokens.Size());
			}
			return Parameter;
		}

		throw TextTokenizerError(TEXT("Unsupported template parameter"), Tokens[0], CurrentFile());
	}

	bool CppParser::ParseConceptDeclaration(Array<ParsedAttribute> LeadingAttributes)
	{
		const Array<TextToken> Tokens = ReadDeclarationTokensUntilSemicolon();
		if (Tokens.Size() == 0 || Tokens[0].Type != ETextTokenType::Identifier)
		{
			return false;
		}

		const size_t EqualIndex = FindTopLevelToken(Tokens, 0, L"=");
		if (EqualIndex >= Tokens.Size())
		{
			throw TextTokenizerError(TEXT("Expected concept constraint expression"), Tokens[0], CurrentFile());
		}

		ParsedConcept Concept;
		Concept.Name = Tokens[0].Value_Text;
		Concept.Attributes = LeadingAttributes;
		Concept.Constraint = MakeTextExpression(Tokens, EqualIndex + 1, Tokens.Size());

		if (!OnParsed_Concept(Concept))
		{
			throw TextTokenizerError(TEXT("OnParsed_Concept returned false"), Tokens[0], CurrentFile());
		}
		return true;
	}

	Array<ParsedTemplateArgument> CppParser::ParseTemplateArguments()
	{
		Array<TextToken> Tokens;
		TextToken Token;
		size_t AngleDepth = 1;
		while (GetParserToken(Token))
		{
			if (IsSymbol(Token, L"<"))
			{
				++AngleDepth;
			}
			else if (IsSymbol(Token, L">") && AngleDepth > 0)
			{
				--AngleDepth;
				if (AngleDepth == 0)
				{
					break;
				}
			}

			Tokens.Add(Token);
		}

		if (AngleDepth > 0)
		{
			throw TextTokenizerError(TEXT("Expected template argument list end"), CurrentTokenizer(), 0, CurrentFile());
		}

		Array<ParsedTemplateArgument> Arguments;
		for (const Array<TextToken>& ArgumentTokens : SplitTopLevelCommas(Tokens))
		{
			if (ArgumentTokens.Size() == 0)
			{
				continue;
			}

			ParsedTemplateArgument& Argument = Arguments.EmplaceRef();
			Argument.Kind = ParsedTemplateArgument::EKind::Type;
			Argument.Type = std::make_shared<ParsedType>(MakeTextType(TokensToText(ArgumentTokens, 0, ArgumentTokens.Size())));
		}
		return Arguments;
	}

	Array<ParsedAttribute> CppParser::ParseAttributes()
	{
		Array<ParsedAttribute> Attributes;
		TextToken Token;
		while (GetParserToken(Token))
		{
			if (!IsAttributeStart(Token))
			{
				PushParserToken(Token);
				break;
			}

			std::wstring Text;
			ParsedAttribute::EKind Kind = ParsedAttribute::EKind::Standard;
			const bool IsStandardAttribute = IsSymbol(Token, L"[[");
			if (IsIdentifier(Token, L"alignas"))
			{
				Kind = ParsedAttribute::EKind::Alignas;
			}
			else if (IsIdentifier(Token, L"__declspec"))
			{
				Kind = ParsedAttribute::EKind::Declspec;
			}
			else if (IsIdentifier(Token, L"__attribute__"))
			{
				Kind = ParsedAttribute::EKind::Gnu;
			}

			if (IsStandardAttribute)
			{
				size_t Depth = 1;
				while (GetParserToken(Token))
				{
					if (IsSymbol(Token, L"[["))
					{
						++Depth;
					}
					else if (IsSymbol(Token, L"]]"))
					{
						--Depth;
						if (Depth == 0)
						{
							break;
						}
					}

					if (Token.Whitespaces.Size() > 0 && !Text.empty())
					{
						Text += L' ';
					}
					Text += static_cast<std::wstring>(Token.RawText);
				}
			}
			else
			{
				if (!GetParserToken(Token))
				{
					break;
				}
				if (!IsSymbol(Token, L"("))
				{
					PushParserToken(Token);
					break;
				}

				size_t Depth = 1;
				while (GetParserToken(Token))
				{
					if (IsSymbol(Token, L"("))
					{
						++Depth;
					}
					else if (IsSymbol(Token, L")"))
					{
						--Depth;
						if (Depth == 0)
						{
							break;
						}
					}

					if (Token.Whitespaces.Size() > 0 && !Text.empty())
					{
						Text += L' ';
					}
					Text += static_cast<std::wstring>(Token.RawText);
				}
			}

			if (Kind == ParsedAttribute::EKind::Gnu && Text.size() >= 2 && Text.front() == L'(' && Text.back() == L')')
			{
				Text = Text.substr(1, Text.size() - 2);
			}

			ParsedAttribute Attribute = MakeStandardAttribute(Text);
			Attribute.Kind = Kind;
			Attributes.Add(Attribute);
		}

		return Attributes;
	}

	ParsedName CppParser::ParseQualifiedName(const TextToken& FirstToken, TextToken& NextToken)
	{
		ParsedName Name;
		ParsedNameSegment* Segment = &Name.Segments.EmplaceRef();
		Segment->Name = FirstToken.Value_Text;

		while (GetParserToken(NextToken))
		{
			if (IsSymbol(NextToken, L"<"))
			{
				Segment->TemplateArguments = ParseTemplateArguments();
				continue;
			}

			if (!IsSymbol(NextToken, L":"))
			{
				return Name;
			}

			TextToken ColonOrNext;
			if (!GetParserToken(ColonOrNext))
			{
				return Name;
			}

			if (!IsSymbol(ColonOrNext, L":"))
			{
				PushParserToken(ColonOrNext);
				return Name;
			}

			if (!GetParserToken(NextToken))
			{
				throw TextTokenizerError(TEXT("Expected qualified enum name segment"), CurrentTokenizer(), 0, CurrentFile());
			}

			if (NextToken.Type != ETextTokenType::Identifier)
			{
				throw TextTokenizerError(TEXT("Expected qualified enum name segment"), NextToken, CurrentFile());
			}
			Segment = &Name.Segments.EmplaceRef();
			Segment->Name = NextToken.Value_Text;
		}

		return Name;
	}

	ParsedType CppParser::ParseTypeUntil(TextToken& NextToken)
	{
		ParsedType Type;
		String Text;
		while (GetParserToken(NextToken))
		{
			if (IsSymbol(NextToken, L"{") || IsSymbol(NextToken, L";"))
			{
				break;
			}

			AppendTokenText(Text, NextToken);
		}

		Text.Trim();
		if (Text.Size() > 0)
		{
			Type.Name.Segments.EmplaceRef().Name = Text;
		}
		return Type;
	}

	ParsedExpression CppParser::ParseExpressionUntilEnumValueEnd(TextToken& NextToken)
	{
		ParsedExpression Expression;
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;

		while (GetParserToken(NextToken))
		{
			if (IsSymbol(NextToken, L"("))
			{
				++ParenDepth;
			}
			else if (IsSymbol(NextToken, L")") && ParenDepth > 0)
			{
				--ParenDepth;
			}
			else if (IsSymbol(NextToken, L"[") || IsSymbol(NextToken, L"[["))
			{
				++BracketDepth;
			}
			else if ((IsSymbol(NextToken, L"]") || IsSymbol(NextToken, L"]]")) && BracketDepth > 0)
			{
				--BracketDepth;
			}
			else if (IsSymbol(NextToken, L"{"))
			{
				++BraceDepth;
			}
			else if (IsSymbol(NextToken, L"}"))
			{
				if (BraceDepth == 0 && ParenDepth == 0 && BracketDepth == 0)
				{
					break;
				}
				--BraceDepth;
			}
			else if (IsSymbol(NextToken, L",") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0)
			{
				break;
			}

			AppendTokenText(Expression.Text, NextToken);
		}

		Expression.Text.Trim();
		return Expression;
	}

	bool CppParser::ParseEnumValue(const TextToken& FirstToken, TextToken& NextToken)
	{
		if (FirstToken.Type != ETextTokenType::Identifier)
		{
			throw TextTokenizerError(TEXT("Expected enum value name"), FirstToken, CurrentFile());
		}

		ParsedEnumValue Value;
		Value.Name = FirstToken.Value_Text;
		Value.Attributes = ParseAttributes();

		if (!GetParserToken(NextToken))
		{
			throw TextTokenizerError(TEXT("Expected enum value separator"), CurrentTokenizer(), 0, CurrentFile());
		}

		if (IsSymbol(NextToken, L"="))
		{
			Value.HasValue = true;
			Value.Value = ParseExpressionUntilEnumValueEnd(NextToken);
		}

		if (!OnParsed_EnumValue(Value))
		{
			throw TextTokenizerError(TEXT("OnParsed_EnumValue returned false"), FirstToken, CurrentFile());
		}

		if (IsSymbol(NextToken, L"}"))
		{
			return false;
		}

		if (!IsSymbol(NextToken, L","))
		{
			throw TextTokenizerError(TEXT("Expected enum value separator"), NextToken, CurrentFile());
		}
		return true;
	}

	void CppParser::SkipBalancedBlock(const WChar* Open, const WChar* Close)
	{
		TextToken Token;
		size_t Depth = 1;
		while (Depth > 0 && GetParserToken(Token))
		{
			if (IsSymbol(Token, Open))
			{
				++Depth;
			}
			else if (IsSymbol(Token, Close))
			{
				--Depth;
			}
		}
	}

	void CppParser::SkipFunctionDefinitionBody()
	{
		TextToken Token;
		while (GetParserToken(Token))
		{
			if (!IsSymbol(Token, L"{"))
			{
				continue;
			}

			SkipBalancedBlock(L"{", L"}");

			TextToken NextToken;
			if (!GetParserToken(NextToken))
			{
				return;
			}
			if (IsSymbol(NextToken, L",") || IsSymbol(NextToken, L"{"))
			{
				PushParserToken(NextToken);
				continue;
			}
			while (IsIdentifier(NextToken, L"catch"))
			{
				while (GetParserToken(NextToken) && !IsSymbol(NextToken, L"{")) {}
				if (IsSymbol(NextToken, L"{"))
				{
					SkipBalancedBlock(L"{", L"}");
				}
				if (!GetParserToken(NextToken))
				{
					return;
				}
			}

			PushParserToken(NextToken);
			return;
		}
	}

	void CppParser::AppendTokenText(String& Text, const TextToken& Token)
	{
		const bool IsScopeColon = IsSymbol(Token, L":") && Text.EndsWith(L':');
		const bool AfterScopeSeparator = Text.EndsWith(L"::");
		if (Token.Whitespaces.Size() > 0 && Text.Size() > 0 && !IsScopeColon && !AfterScopeSeparator)
		{
			Text += L' ';
		}
		Text += Token.RawText;
	}

	bool CppParser::ReadScopeSeparator()
	{
		TextToken Token;
		if (!GetParserToken(Token))
		{
			return false;
		}

		if (IsSymbol(Token, L"::"))
		{
			return true;
		}

		if (IsSymbol(Token, L":"))
		{
			TextToken NextToken;
			if (GetParserToken(NextToken))
			{
				if (IsSymbol(NextToken, L":"))
				{
					return true;
				}
				PushParserToken(NextToken);
			}
		}

		PushParserToken(Token);
		return false;
	}

	Array<TextToken> CppParser::ReadDeclarationTokensUntilSemicolon()
	{
		Array<TextToken> Tokens;
		TextToken Token;
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		while (GetParserToken(Token))
		{
			if (IsSymbol(Token, L";") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0)
			{
				break;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }

			Tokens.Add(Token);
		}
		return Tokens;
	}

	Array<TextToken> CppParser::ReadVariableDeclarationTokens(const TextToken& FirstToken, bool& HasSemicolon)
	{
		Array<TextToken> Tokens;
		Tokens.Add(FirstToken);
		HasSemicolon = false;

		TextToken Token;
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		while (GetParserToken(Token))
		{
			if (IsSymbol(Token, L";") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0)
			{
				Tokens.Add(Token);
				HasSemicolon = true;
				break;
			}

			if (IsSymbol(Token, L"{") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && !IsTopLevelClassBody(Tokens))
			{
				if (!IsRequiresExpressionBodyStart(Tokens))
				{
					PushParserToken(Token);
					break;
				}
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }

			Tokens.Add(Token);
		}
		return Tokens;
	}

	void CppParser::PushParserTokensAfterFirst(const Array<TextToken>& Tokens)
	{
		for (size_t Index = Tokens.Size(); Index > 1; --Index)
		{
			PushParserToken(Tokens[Index - 1]);
		}
	}

	Array<Array<TextToken>> CppParser::SplitTopLevelCommas(const Array<TextToken>& Tokens)
	{
		Array<Array<TextToken>> Result;
		Array<TextToken>* Current = &Result.EmplaceRef();
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = 0; Index < Tokens.Size(); ++Index)
		{
			const TextToken& Token = Tokens[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Tokens, Index);
			if (DecltypeEnd < Tokens.Size())
			{
				for (size_t TokenIndex = Index; TokenIndex <= DecltypeEnd; ++TokenIndex)
				{
					Current->Add(Tokens[TokenIndex]);
				}
				Index = DecltypeEnd;
				continue;
			}

			if (IsSymbol(Token, L",") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				Current = &Result.EmplaceRef();
				continue;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }

			Current->Add(Token);
		}
		return Result;
	}

	String CppParser::TokensToText(const Array<TextToken>& Tokens, size_t Begin, size_t End)
	{
		String Text;
		if (End > Tokens.Size())
		{
			End = Tokens.Size();
		}

		for (size_t Index = Begin; Index < End; ++Index)
		{
			AppendTokenText(Text, Tokens[Index]);
		}
		Text.Trim();
		return Text;
	}

	ParsedName CppParser::MakeTextName(const String& Text)
	{
		ParsedName Name;
		if (Text.Size() > 0)
		{
			Name.Segments.EmplaceRef().Name = Text;
		}
		return Name;
	}

	ParsedType CppParser::MakeTextType(const String& Text)
	{
		ParsedType Type;
		if (Text.Size() > 0)
		{
			Type.Name = MakeTextName(Text);
		}
		return Type;
	}

	size_t CppParser::FindTypedefDeclaratorStart(const Array<TextToken>& Tokens)
	{
		for (size_t Index = 0; Index + 3 < Tokens.Size(); ++Index)
		{
			if (!IsSymbol(Tokens[Index], L"("))
			{
				continue;
			}

			size_t Cursor = Index + 1;
			while (Cursor < Tokens.Size() && (IsSymbol(Tokens[Cursor], L"*") || IsSymbol(Tokens[Cursor], L"&") || IsSymbol(Tokens[Cursor], L"&&")))
			{
				++Cursor;
			}

			if (Cursor < Tokens.Size() && Tokens[Cursor].Type == ETextTokenType::Identifier)
			{
				return Index;
			}
		}

		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = 0; Index < Tokens.Size(); ++Index)
		{
			const TextToken& Token = Tokens[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Tokens, Index);
			if (DecltypeEnd < Tokens.Size())
			{
				Index = DecltypeEnd;
				continue;
			}

			if (IsSymbol(Token, L"(") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0 && Index > 0 && Tokens[Index - 1].Type == ETextTokenType::Identifier && !IsIdentifier(Tokens[Index - 1], L"decltype"))
			{
				return Index - 1;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
		}

		for (size_t Index = Tokens.Size(); Index > 0; --Index)
		{
			const TextToken& Token = Tokens[Index - 1];
			if (Token.Type == ETextTokenType::Identifier)
			{
				return Index - 1;
			}
		}
		return Tokens.Size();
	}

	size_t CppParser::FindVariableNameIndex(const Array<TextToken>& Tokens, size_t Begin, size_t End)
	{
		if (End > Tokens.Size())
		{
			End = Tokens.Size();
		}

		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = End; Index > Begin; --Index)
		{
			const TextToken& Token = Tokens[Index - 1];
			if (IsSymbol(Token, L")")) { ++ParenDepth; }
			else if (IsSymbol(Token, L"(") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"}")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"{") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L">")) { ++AngleDepth; }
			else if (IsSymbol(Token, L"<") && AngleDepth > 0) { --AngleDepth; }
			else if (Token.Type == ETextTokenType::Identifier && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				const size_t TokenIndex = Index - 1;
				const bool IsTemplateTypeName = TokenIndex + 1 < End && IsSymbol(Tokens[TokenIndex + 1], L"<") && FindMatchingSymbol(Tokens, TokenIndex + 1, L"<", L">") + 1 == End;
				if (!IsTemplateTypeName && !IsIdentifier(Token, L"const") && !IsIdentifier(Token, L"volatile") && !IsIdentifier(Token, L"static") && !IsIdentifier(Token, L"extern") && !IsIdentifier(Token, L"mutable") && !IsIdentifier(Token, L"thread_local") && !IsIdentifier(Token, L"constexpr") && !IsIdentifier(Token, L"consteval") && !IsIdentifier(Token, L"constinit") && !IsIdentifier(Token, L"noexcept") && !IsIdentifier(Token, L"override") && !IsIdentifier(Token, L"final"))
				{
					return TokenIndex;
				}
			}
		}
		return End;
	}

	size_t CppParser::FindVariableDeclaratorTypeBegin(const Array<TextToken>& Tokens, size_t NameIndex)
	{
		if (NameIndex > Tokens.Size())
		{
			NameIndex = Tokens.Size();
		}

		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = 0; Index < NameIndex; ++Index)
		{
			const TextToken& Token = Tokens[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Tokens, Index);
			if (DecltypeEnd < Tokens.Size())
			{
				Index = DecltypeEnd;
				continue;
			}

			if ((IsSymbol(Token, L"*") || IsSymbol(Token, L"&") || IsSymbol(Token, L"&&")) && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				return Index;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
		}
		return NameIndex;
	}

	size_t CppParser::FindVariableDeclaratorEnd(const Array<TextToken>& Tokens)
	{
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = 0; Index < Tokens.Size(); ++Index)
		{
			const TextToken& Token = Tokens[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Tokens, Index);
			if (DecltypeEnd < Tokens.Size())
			{
				Index = DecltypeEnd;
				continue;
			}

			if (IsSymbol(Token, L"=") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				return Index;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
		}
		return Tokens.Size();
	}

	ParsedType CppParser::ParseVariableType(const Array<TextToken>& Tokens)
	{
		ParsedType Type;
		Array<TextToken> NameTokens;
		ParsedIndirection* CurrentIndirection = nullptr;
		for (size_t Index = 0; Index < Tokens.Size(); ++Index)
		{
			const TextToken& Token = Tokens[Index];
			if (IsIdentifier(Token, L"static") || IsIdentifier(Token, L"extern") || IsIdentifier(Token, L"thread_local") || IsIdentifier(Token, L"constexpr") || IsIdentifier(Token, L"consteval") || IsIdentifier(Token, L"constinit"))
			{
				continue;
			}

			if (IsIdentifier(Token, L"const"))
			{
				if (CurrentIndirection != nullptr) { CurrentIndirection->IsConst = true; }
				else { Type.IsConst = true; }
				continue;
			}
			if (IsIdentifier(Token, L"volatile"))
			{
				if (CurrentIndirection != nullptr) { CurrentIndirection->IsVolatile = true; }
				else { Type.IsVolatile = true; }
				continue;
			}
			if (IsIdentifier(Token, L"mutable"))
			{
				if (CurrentIndirection != nullptr) { CurrentIndirection->IsMutable = true; }
				continue;
			}
			if (IsIdentifier(Token, L"signed"))
			{
				Type.IsSigned = true;
				continue;
			}
			if (IsIdentifier(Token, L"unsigned"))
			{
				Type.IsUnsigned = true;
				continue;
			}
			if (IsIdentifier(Token, L"class") || IsIdentifier(Token, L"struct") || IsIdentifier(Token, L"union") || IsIdentifier(Token, L"enum") || IsIdentifier(Token, L"typename"))
			{
				Type.IsElaboratedType = true;
				Type.ElaboratedTypeKeyword = Token.Value_Text;
				continue;
			}

			if (IsSymbol(Token, L"*") || IsSymbol(Token, L"&") || IsSymbol(Token, L"&&"))
			{
				ParsedIndirection& Indirection = Type.Indirections.EmplaceRef();
				if (IsSymbol(Token, L"&"))
				{
					Indirection.Kind = ParsedIndirection::EKind::LValueReference;
				}
				else if (IsSymbol(Token, L"&&"))
				{
					Indirection.Kind = ParsedIndirection::EKind::RValueReference;
				}
				CurrentIndirection = &Indirection;
				continue;
			}

			NameTokens.Add(Token);
		}

		Type.Name = MakeTextName(TokensToText(NameTokens, 0, NameTokens.Size()));
		return Type;
	}

	ParsedExpression CppParser::MakeTextExpression(const Array<TextToken>& Tokens, size_t Begin, size_t End)
	{
		ParsedExpression Expression;
		Expression.Text = TokensToText(Tokens, Begin, End);
		return Expression;
	}

	size_t CppParser::FindFunctionParameterListOpen(const Array<TextToken>& Tokens, size_t NameIndex)
	{
		for (size_t Index = NameIndex + 1; Index < Tokens.Size(); ++Index)
		{
			if (IsSymbol(Tokens[Index], L"("))
			{
				return Index;
			}
			if (IsSymbol(Tokens[Index], L"[["))
			{
				Index = FindMatchingSymbol(Tokens, Index, L"[[", L"]]");
				continue;
			}
			if (IsSymbol(Tokens[Index], L"=") || IsSymbol(Tokens[Index], L"{") || IsSymbol(Tokens[Index], L"["))
			{
				break;
			}
		}
		return Tokens.Size();
	}

	size_t CppParser::FindMatchingSymbol(const Array<TextToken>& Tokens, size_t OpenIndex, const WChar* Open, const WChar* Close)
	{
		if (OpenIndex >= Tokens.Size() || !IsSymbol(Tokens[OpenIndex], Open))
		{
			return Tokens.Size();
		}

		size_t Depth = 1;
		for (size_t Index = OpenIndex + 1; Index < Tokens.Size(); ++Index)
		{
			if (IsSymbol(Tokens[Index], Open))
			{
				++Depth;
			}
			else if (IsSymbol(Tokens[Index], Close))
			{
				--Depth;
				if (Depth == 0)
				{
					return Index;
				}
			}
		}
		return Tokens.Size();
	}

	size_t CppParser::FindTopLevelToken(const Array<TextToken>& Tokens, size_t Begin, const WChar* Text)
	{
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = Begin; Index < Tokens.Size(); ++Index)
		{
			const TextToken& Token = Tokens[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Tokens, Index);
			if (DecltypeEnd < Tokens.Size())
			{
				Index = DecltypeEnd;
				continue;
			}

			if ((IsSymbol(Token, Text) || IsIdentifier(Token, Text)) && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				return Index;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
		}
		return Tokens.Size();
	}

	size_t CppParser::FindTopLevelRequiresClauseEnd(const Array<TextToken>& Tokens, size_t Begin)
	{
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		size_t AngleDepth = 0;
		for (size_t Index = Begin; Index < Tokens.Size(); ++Index)
		{
			const TextToken& Token = Tokens[Index];
			const size_t DecltypeEnd = FindDecltypeSpecifierEnd(Tokens, Index);
			if (DecltypeEnd < Tokens.Size())
			{
				Index = DecltypeEnd;
				continue;
			}

			if ((IsSymbol(Token, L"=") || IsSymbol(Token, L":") || IsSymbol(Token, L";")) && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && AngleDepth == 0)
			{
				return Index;
			}

			if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }
			else if (IsSymbol(Token, L"(") ) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"<")) { ++AngleDepth; }
			else if (IsSymbol(Token, L">") && AngleDepth > 0) { --AngleDepth; }
		}
		return Tokens.Size();
	}

	size_t CppParser::FindDecltypeSpecifierEnd(const Array<TextToken>& Tokens, size_t Index)
	{
		if (Index + 1 >= Tokens.Size() || !IsIdentifier(Tokens[Index], L"decltype") || !IsSymbol(Tokens[Index + 1], L"("))
		{
			return Tokens.Size();
		}

		return FindMatchingSymbol(Tokens, Index + 1, L"(", L")");
	}

	bool CppParser::IsRequiresExpressionBodyStart(const Array<TextToken>& Tokens)
	{
		if (Tokens.Size() == 0)
		{
			return false;
		}

		const size_t LastIndex = Tokens.Size() - 1;
		if (IsIdentifier(Tokens[LastIndex], L"requires"))
		{
			return true;
		}
		if (!IsSymbol(Tokens[LastIndex], L")"))
		{
			return false;
		}

		size_t Depth = 1;
		for (size_t Index = LastIndex; Index > 0; --Index)
		{
			const TextToken& Token = Tokens[Index - 1];
			if (IsSymbol(Token, L")"))
			{
				++Depth;
			}
			else if (IsSymbol(Token, L"(") && Depth > 0)
			{
				--Depth;
				if (Depth == 0)
				{
					return Index >= 2 && IsIdentifier(Tokens[Index - 2], L"requires");
				}
			}
		}
		return false;
	}

	bool CppParser::IsFunctionSpecifier(const TextToken& Token)
	{
		return IsIdentifier(Token, L"friend") || IsIdentifier(Token, L"virtual") || IsIdentifier(Token, L"static") || IsIdentifier(Token, L"inline") || IsIdentifier(Token, L"constexpr") || IsIdentifier(Token, L"consteval") || IsIdentifier(Token, L"explicit");
	}

	bool CppParser::IsDestructorToken(const TextToken& Token)
	{
		return IsSymbol(Token, L"~") || IsIdentifier(Token, L"compl");
	}

	bool CppParser::IsVariableDeclarationBlocked(const Array<TextToken>& Tokens)
	{
		if (Tokens.Size() == 0)
		{
			return true;
		}

		if (IsIdentifier(Tokens[0], L"class") || IsIdentifier(Tokens[0], L"struct") || IsIdentifier(Tokens[0], L"union"))
		{
			const size_t TokenEnd = IsSymbol(Tokens[Tokens.Size() - 1], L";") ? Tokens.Size() - 1 : Tokens.Size();
			return TokenEnd == 2 && Tokens[1].Type == ETextTokenType::Identifier;
		}

		return !IsVariableStartToken(Tokens[0]);
	}

	bool CppParser::IsVariableStartToken(const TextToken& Token)
	{
		if (Token.Type != ETextTokenType::Identifier)
		{
			return false;
		}

		return !IsIdentifier(Token, L"using") && !IsIdentifier(Token, L"typedef") && !IsIdentifier(Token, L"namespace") && !IsIdentifier(Token, L"class") && !IsIdentifier(Token, L"struct") && !IsIdentifier(Token, L"union") && !IsIdentifier(Token, L"enum") && !IsIdentifier(Token, L"template") && !IsIdentifier(Token, L"concept") && !IsIdentifier(Token, L"return") && !IsIdentifier(Token, L"static_assert") && !IsIdentifier(Token, L"asm") && !IsIdentifier(Token, L"public") && !IsIdentifier(Token, L"protected") && !IsIdentifier(Token, L"private");
	}

	bool CppParser::IsFunctionDeclarator(const Array<TextToken>& Tokens, size_t NameIndex)
	{
		return FindFunctionParameterListOpen(Tokens, NameIndex) < Tokens.Size();
	}

	bool CppParser::ParseTrailingClassDeclarator(const ParsedClass& Class, const Array<TextToken>& TypePrefixTokens)
	{
		TextToken Token;
		if (!GetParserToken(Token))
		{
			return false;
		}

		if (IsSymbol(Token, L";"))
		{
			return false;
		}

		if (Class.Name.Segments.Size() == 0 || Token.Type != ETextTokenType::Identifier)
		{
			PushParserToken(Token);
			return false;
		}

		Array<TextToken> TypeTokens = TypePrefixTokens;
		TypeTokens.Add(MakeIdentifierToken(Class.Type == EClassType::Class ? L"class" : Class.Type == EClassType::Struct ? L"struct" : L"union"));
		TypeTokens.Add(MakeIdentifierToken(NameToText(Class.Name)));

		PushParserToken(Token);
		for (size_t Index = TypeTokens.Size(); Index > 1; --Index)
		{
			PushParserToken(TypeTokens[Index - 1]);
		}
		return ParseVariableDeclaration(TypeTokens[0]);
	}

	TextToken CppParser::MakeIdentifierToken(const String& Text)
	{
		TextToken Token;
		Token.Type = ETextTokenType::Identifier;
		Token.RawText = Text;
		Token.Value_Text = Text;
		return Token;
	}

	String CppParser::NameToText(const ParsedName& Name)
	{
		String Text;
		for (size_t Index = 0; Index < Name.Segments.Size(); ++Index)
		{
			if (Index > 0)
			{
				Text += L"::";
			}
			Text += Name.Segments[Index].Name;
		}
		return Text;
	}

	bool CppParser::IsTopLevelClassBody(const Array<TextToken>& Tokens)
	{
		for (size_t Index = Tokens.Size(); Index > 0; --Index)
		{
			const TextToken& Token = Tokens[Index - 1];
			if (IsIdentifier(Token, L"class") || IsIdentifier(Token, L"struct") || IsIdentifier(Token, L"union"))
			{
				return true;
			}
		}
		return false;
	}

	bool CppParser::HasTopLevelClassBody(const Array<TextToken>& Tokens)
	{
		size_t ParenDepth = 0;
		size_t BracketDepth = 0;
		size_t BraceDepth = 0;
		Array<TextToken> Prefix;
		for (const TextToken& Token : Tokens)
		{
			if (IsSymbol(Token, L"{") && ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0 && IsTopLevelClassBody(Prefix))
			{
				return true;
			}

			if (IsSymbol(Token, L"(")) { ++ParenDepth; }
			else if (IsSymbol(Token, L")") && ParenDepth > 0) { --ParenDepth; }
			else if (IsSymbol(Token, L"[") || IsSymbol(Token, L"[[")) { ++BracketDepth; }
			else if ((IsSymbol(Token, L"]") || IsSymbol(Token, L"]]")) && BracketDepth > 0) { --BracketDepth; }
			else if (IsSymbol(Token, L"{")) { ++BraceDepth; }
			else if (IsSymbol(Token, L"}") && BraceDepth > 0) { --BraceDepth; }

			Prefix.Add(Token);
		}
		return false;
	}

	bool CppParser::IsCurrentScopeTokenDepth(size_t BraceDepth, const Array<size_t>& NamespaceDepths, const Array<size_t>& ParsedScopeDepths)
	{
		if (BraceDepth == 0)
		{
			return true;
		}
		if (NamespaceDepths.Size() > 0 && NamespaceDepths[NamespaceDepths.Size() - 1] == BraceDepth)
		{
			return true;
		}
		return ParsedScopeDepths.Size() > 0 && ParsedScopeDepths[ParsedScopeDepths.Size() - 1] == BraceDepth;
	}

	bool CppParser::IsAttributeStart(const TextToken& Token)
	{
		return IsSymbol(Token, L"[[") || IsIdentifier(Token, L"alignas") || IsIdentifier(Token, L"__declspec") || IsIdentifier(Token, L"__attribute__");
	}

	bool CppParser::IsIdentifier(const TextToken& Token, const WChar* Text)
	{
		return Token.Type == ETextTokenType::Identifier && Token.Value_Text == Text;
	}

	bool CppParser::IsSymbol(const TextToken& Token, const WChar* Text)
	{
		return Token.Type == ETextTokenType::Symbol && Token.Value_Text == Text;
	}

	ParsedName CppParser::MakeName(const String& Name)
	{
		ParsedName Result;
		Result.Segments.EmplaceRef().Name = Name;
		return Result;
	}

	ParsedAttribute CppParser::MakeStandardAttribute(const std::wstring& Text)
	{
		ParsedAttribute Result;
		std::wstring Name = Text;
		std::wstring Arguments;
		bool HasTopLevelComma = false;
		size_t ParenDepth = 0;
		for (const WChar Char : Text)
		{
			if (Char == L'(')
			{
				++ParenDepth;
			}
			else if (Char == L')' && ParenDepth > 0)
			{
				--ParenDepth;
			}
			else if (Char == L',' && ParenDepth == 0)
			{
				HasTopLevelComma = true;
				break;
			}
		}

		const size_t OpenParen = HasTopLevelComma ? std::wstring::npos : Text.find(L'(');
		if (OpenParen != std::wstring::npos)
		{
			Name = Text.substr(0, OpenParen);
			const size_t CloseParen = Text.rfind(L')');
			if (CloseParen != std::wstring::npos && CloseParen > OpenParen)
			{
				Arguments = Text.substr(OpenParen + 1, CloseParen - OpenParen - 1);
			}
		}

		String NameText(Name);
		NameText.Trim();
		Result.Name = MakeName(NameText);
		if (!Arguments.empty())
		{
			String ArgumentText(Arguments);
			ArgumentText.Trim();
			Result.Arguments.EmplaceRef().Text = ArgumentText;
		}

		return Result;
	}
}
