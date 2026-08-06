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
			if (!GetToken(Token))
			{
				break;
			}

			if (Token.Type == ETextTokenType::Identifier)
			{
				if (Token.Value_Text == TEXT("namespace"))
				{
					Parse_Namespace(Token, false);
					continue;
				}
				else if (Token.Value_Text == TEXT("class"))
				{
					Parse_Class(Token, EClassType::Class);
					continue;
				}
				else if (Token.Value_Text == TEXT("struct"))
				{
					Parse_Class(Token, EClassType::Struct);
					continue;
				}
				else if (Token.Value_Text == TEXT("enum"))
				{
					Parse_Enum(Token);
					continue;
				}
				else if (Token.Value_Text == TEXT("inline"))
				{
					RequireToken(Token);
					if (Token.Type == ETextTokenType::Identifier && Token.Value_Text == TEXT("namespace"))
					{
						Parse_Namespace(Token, true);
						continue;
					}
					// Inline Function
				}
				else if (Token.Value_Text == TEXT("public"))
				{
					Parse_Access(Token, EAccessSpecifier::Public);
					continue;
				}
				else if (Token.Value_Text == TEXT("protected"))
				{
					Parse_Access(Token, EAccessSpecifier::Protected);
					continue;
				}
				else if (Token.Value_Text == TEXT("private"))
				{
					Parse_Access(Token, EAccessSpecifier::Private);
					continue;
				}
			}
			else if (Token.Type == ETextTokenType::Symbol)
			{
				if (Token.Value_Text == TEXT("}"))
				{
					OnParsed_ScopeEnd();
					continue;
				}
				else if (Token.Value_Text == TEXT(";"))
				{
					continue;
				}
			}

			throw TextTokenizerError(TEXT("Unexpected Token '") + Token.Value_Text + TEXT("'"), Token, CurrentFile());
		}

		OnParseEnd();
	}

	void CppParser::RequireToken(TextToken& Token, ETextTokenType RequiredType /*= ETextTokenType::Undefined*/, const String& RequiredValue /*= TEXT("")*/)
	{
		if (!GetToken(Token))
		{
			throw TextTokenizerError(TEXT("Expected Token"), Token, CurrentFile());
		}

		if (RequiredType != ETextTokenType::Undefined && Token.Type != RequiredType)
		{
			String RequiredTypeName = TEXT("Undefined");
			switch (RequiredType)
			{
			case ETextTokenType::Identifier:
				RequiredTypeName = TEXT("Identifier");
				break;
			case ETextTokenType::Constant:
				RequiredTypeName = TEXT("Constant");
				break;
			case ETextTokenType::Symbol:
				RequiredTypeName = TEXT("Symbol");
				break;
			}
			throw TextTokenizerError(TEXT("Unexpected Token, expected ") + RequiredTypeName, Token, CurrentFile());
		}

		if (RequiredValue.Size() > 0 && Token.Value_Text != RequiredValue)
		{
			throw TextTokenizerError(TEXT("Unxpected Token, expected '") + RequiredValue + TEXT("'"), Token, CurrentFile());
		}
	}

	void CppParser::Parse_Name(TextToken& Token, ParsedName& Name, bool AllowTemplate, bool AllowInline, bool AllowLeadingScope, bool AllowScope)
	{
		Name.Segments.Clear();
		if (AllowLeadingScope && Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT(":"))
		{
			Name.Segments.EmplaceRef();
			RequireToken(Token, ETextTokenType::Symbol, TEXT(":"));
			RequireToken(Token);
		}

		while (true)
		{
			bool IsInline = false;
			if (AllowInline && Token.Type == ETextTokenType::Identifier && Token.Value_Text == TEXT("inline"))
			{
				IsInline = true;
				RequireToken(Token, ETextTokenType::Identifier);
			}

			if (Token.Type != ETextTokenType::Identifier)
			{
				throw TextTokenizerError(TEXT("Expected name segment"), Token, CurrentFile());
			}

			ParsedNameSegment& Segment = Name.Segments.EmplaceRef();
			Segment.Name = Token.Value_Text;
			Segment.IsInline = IsInline;

			RequireToken(Token);
			if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT("<"))
			{
				if (!AllowTemplate)
				{
					throw TextTokenizerError(TEXT("Template arguments are not allowed in this name"), Token, CurrentFile());
				}

				String ArgumentText;
				size_t AngleDepth = 1;
				size_t ParenDepth = 0;
				size_t BracketDepth = 0;
				size_t BraceDepth = 0;

				auto AppendToken = [&ArgumentText](const TextToken& ArgumentToken)
					{
						if (ArgumentText.Size() > 0 && ArgumentToken.Whitespaces.Size() > 0)
						{
							ArgumentText += TEXT(" ");
						}
						ArgumentText += ArgumentToken.RawText.Size() > 0 ? ArgumentToken.RawText : ArgumentToken.Value_Text;
					};

				auto AddArgument = [this, &Segment, &ArgumentText, &Token]()
					{
						ArgumentText.Trim();
						if (ArgumentText.Size() == 0)
						{
							throw TextTokenizerError(TEXT("Expected template argument"), Token, CurrentFile());
						}

						ParsedTemplateArgument& Argument = Segment.TemplateArguments.EmplaceRef();
						Argument.Kind = ParsedTemplateArgument::EKind::Type;
						Argument.Type.Name.Segments.EmplaceRef().Name = ArgumentText;
						ArgumentText.Clear();
					};

				while (true)
				{
					RequireToken(Token);

					if (Token.Type == ETextTokenType::Symbol)
					{
						if (Token.Value_Text == TEXT("("))
						{
							++ParenDepth;
						}
						else if (Token.Value_Text == TEXT(")") && ParenDepth > 0)
						{
							--ParenDepth;
						}
						else if (Token.Value_Text == TEXT("[") || Token.Value_Text == TEXT("[["))
						{
							++BracketDepth;
						}
						else if ((Token.Value_Text == TEXT("]") || Token.Value_Text == TEXT("]]")) && BracketDepth > 0)
						{
							--BracketDepth;
						}
						else if (Token.Value_Text == TEXT("{"))
						{
							++BraceDepth;
						}
						else if (Token.Value_Text == TEXT("}") && BraceDepth > 0)
						{
							--BraceDepth;
						}
						else if (ParenDepth == 0 && BracketDepth == 0 && BraceDepth == 0)
						{
							if (Token.Value_Text == TEXT("<"))
							{
								++AngleDepth;
							}
							else if (Token.Value_Text == TEXT(">"))
							{
								--AngleDepth;
								if (AngleDepth == 0)
								{
									if (ArgumentText.Size() > 0 || Segment.TemplateArguments.Size() > 0)
									{
										AddArgument();
									}
									RequireToken(Token);
									break;
								}
							}
							else if (Token.Value_Text == TEXT(">>") && AngleDepth >= 2)
							{
								AngleDepth -= 2;
								if (AngleDepth == 0)
								{
									ArgumentText += TEXT(">");
									AddArgument();
									RequireToken(Token);
									break;
								}
							}
							else if (Token.Value_Text == TEXT(",") && AngleDepth == 1)
							{
								AddArgument();
								continue;
							}
						}
					}

					AppendToken(Token);
				}
			}

			if (!AllowScope || Token.Type != ETextTokenType::Symbol || Token.Value_Text != TEXT(":"))
			{
				return;
			}

			RequireToken(Token, ETextTokenType::Symbol, TEXT(":"));
			RequireToken(Token);
		}
	}

	void CppParser::Parse_Type(TextToken& Token, ParsedType& Type)
	{
		auto AddFlag = [&Type](EParsedTypeFlags Flag)
			{
				Type.Flags = static_cast<EParsedTypeFlags>(static_cast<uint8>(Type.Flags) | static_cast<uint8>(Flag));
			};

		while (Token.Type == ETextTokenType::Identifier)
		{
			if (Token.Value_Text == TEXT("const"))
			{
				AddFlag(EParsedTypeFlags::IsConst);
			}
			else if (Token.Value_Text == TEXT("volatile"))
			{
				AddFlag(EParsedTypeFlags::IsVolatile);
			}
			else if (Token.Value_Text == TEXT("unsigned"))
			{
				AddFlag(EParsedTypeFlags::IsUnsigned);
			}
			else if (Token.Value_Text == TEXT("signed"))
			{
				AddFlag(EParsedTypeFlags::IsSigned);
			}
			else
			{
				break;
			}
			RequireToken(Token);
		}

		if (Token.Type != ETextTokenType::Identifier && (Token.Type != ETextTokenType::Symbol || Token.Value_Text != TEXT(":")))
		{
			return;
		}

		Parse_Name(Token, Type.Name, true, false, true, true);
		while (Token.Type == ETextTokenType::Identifier)
		{
			Type.Name.Segments[Type.Name.Segments.Size() - 1].Name += TEXT(" ") + Token.Value_Text;
			RequireToken(Token);
		}
	}

	void CppParser::Parse_Namespace(TextToken& Token, bool IsInline)
	{
		RequireToken(Token);
		if (Token.Type == ETextTokenType::Identifier)
		{
			ParsedNamespace Namespace;
			Parse_Name(Token, Namespace.Name, false, true, false, true);
			Namespace.Name.Segments[0].IsInline = IsInline;
			if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT("="))
			{
				ParsedNamespaceAlias Alias;
				Alias.Name = Namespace.Name;
				RequireToken(Token);
				Parse_Name(Token, Alias.Target, false, false, true, true);
				if (Token.Type != ETextTokenType::Symbol || Token.Value_Text != TEXT(";"))
				{
					throw TextTokenizerError(TEXT("Expected semicolon after namespace alias"), Token, CurrentFile());
				}
				OnParsed_NamespaceAlias(Alias);
				return;
			}
			if (Token.Type != ETextTokenType::Symbol || Token.Value_Text != TEXT("{"))
			{
				throw TextTokenizerError(TEXT("Expected scope begin after namespace name"), Token, CurrentFile());
			}
			OnParsed_Namespace(Namespace);
		}
		else if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT("{"))
		{
			OnParsed_Namespace({});
		}
		else
		{
			throw TextTokenizerError(TEXT("Unexpected Token after namespace '") + Token.Value_Text + TEXT("'"), Token, CurrentFile());
		}
	}

	void CppParser::Parse_Class(TextToken& Token, EClassType Type)
	{
		ParsedClass Class;
		Class.Type = Type;
		RequireToken(Token);

		while (Token.Type == ETextTokenType::Identifier && (Token.Value_Text == TEXT("__declspec") || Token.Value_Text == TEXT("alignas")))
		{
			ParsedAttribute& Attribute = Class.Attributes.EmplaceRef();
			Attribute.Kind = Token.Value_Text == TEXT("alignas") ? ParsedAttribute::EKind::Alignas : ParsedAttribute::EKind::Declspec;
			RequireToken(Token, ETextTokenType::Symbol, TEXT("("));
			RequireToken(Token);
			String Text;
			size_t Depth = 1;
			while (Depth > 0)
			{
				if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT("("))
				{
					++Depth;
				}
				else if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT(")") && --Depth == 0)
				{
					break;
				}
				Text += Token.RawText.Size() > 0 ? Token.RawText : Token.Value_Text;
				RequireToken(Token);
			}
			Text.Trim();
			if (Attribute.Kind == ParsedAttribute::EKind::Declspec)
			{
				Attribute.Name.Segments.EmplaceRef().Name = Text;
			}
			else
			{
				Attribute.Arguments.EmplaceRef().Text = Text;
			}
			RequireToken(Token);
		}

		if (Token.Type == ETextTokenType::Identifier)
		{
			Parse_Name(Token, Class.Name, true, false, false, false);
		}
		else
		{
			Class.IsAnonymous = true;
		}

		if (Token.Type == ETextTokenType::Identifier && Token.Value_Text == TEXT("final"))
		{
			Class.IsFinal = true;
			RequireToken(Token);
		}

		if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT(":"))
		{
			do
			{
				RequireToken(Token);
				ParsedBaseClass& Base = Class.BaseClasses.EmplaceRef();
				Base.AccessSpecifier = Type == EClassType::Struct ? EAccessSpecifier::Public : EAccessSpecifier::Private;
				while (Token.Type == ETextTokenType::Identifier)
				{
					if (Token.Value_Text == TEXT("public"))
					{
						Base.AccessSpecifier = EAccessSpecifier::Public;
					}
					else if (Token.Value_Text == TEXT("protected"))
					{
						Base.AccessSpecifier = EAccessSpecifier::Protected;
					}
					else if (Token.Value_Text == TEXT("private"))
					{
						Base.AccessSpecifier = EAccessSpecifier::Private;
					}
					else if (Token.Value_Text == TEXT("virtual"))
					{
						Base.IsVirtual = true;
					}
					else
					{
						break;
					}
					RequireToken(Token);
				}

				Parse_Type(Token, Base.Type);
			} while (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT(","));
		}

		if (Token.Type != ETextTokenType::Symbol || (Token.Value_Text != TEXT("{") && Token.Value_Text != TEXT(";")))
		{
			throw TextTokenizerError(TEXT("Expected class body or semicolon"), Token, CurrentFile());
		}

		Class.HasBody = Token.Value_Text == TEXT("{");
		Class.IsForward = !Class.HasBody;
		OnParsed_Class(Class);
	}

	void CppParser::Parse_Enum(TextToken& Token)
	{
		ParsedEnum Enum;
		RequireToken(Token);
		if (Token.Type == ETextTokenType::Identifier && (Token.Value_Text == TEXT("class") || Token.Value_Text == TEXT("struct")))
		{
			Enum.IsScoped = true;
			Enum.IsStruct = Token.Value_Text == TEXT("struct");
			RequireToken(Token);
		}
		if (Token.Type == ETextTokenType::Identifier)
		{
			Parse_Name(Token, Enum.Name, false, false, false, false);
		}
		else
		{
			Enum.IsAnonymous = true;
		}

		if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT(":"))
		{
			RequireToken(Token);
			Parse_Type(Token, Enum.UnderlyingType);
		}

		if (Token.Type != ETextTokenType::Symbol || (Token.Value_Text != TEXT("{") && Token.Value_Text != TEXT(";")))
		{
			throw TextTokenizerError(TEXT("Expected enum body or semicolon"), Token, CurrentFile());
		}
		Enum.IsForward = Token.Value_Text == TEXT(";");
		OnParsed_Enum(Enum);
		if (Enum.IsForward) return;

		RequireToken(Token);
		while (Token.Type != ETextTokenType::Symbol || Token.Value_Text != TEXT("}"))
		{
			ParsedEnumValue Value;
			if (Token.Type != ETextTokenType::Identifier)
			{
				throw TextTokenizerError(TEXT("Expected enum value"), Token, CurrentFile());
			}
			Value.Name = Token.Value_Text;
			RequireToken(Token);

			if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT("="))
			{
				Value.HasValue = true;
				size_t Depth = 0;
				RequireToken(Token);
				while (Depth > 0 || Token.Type != ETextTokenType::Symbol || (Token.Value_Text != TEXT(",") && Token.Value_Text != TEXT("}")))
				{
					if (Value.Value.Text.Size() > 0 && Token.Whitespaces.Size() > 0) Value.Value.Text += TEXT(" ");
					Value.Value.Text += Token.RawText.Size() > 0 ? Token.RawText : Token.Value_Text;
					if (Token.Type == ETextTokenType::Symbol && (Token.Value_Text == TEXT("(") || Token.Value_Text == TEXT("[") || Token.Value_Text == TEXT("{"))) ++Depth;
					else if (Token.Type == ETextTokenType::Symbol && (Token.Value_Text == TEXT(")") || Token.Value_Text == TEXT("]") || Token.Value_Text == TEXT("}"))) --Depth;
					RequireToken(Token);
				}
				Value.Value.Text.Trim();
			}
			if (Token.Type != ETextTokenType::Symbol || (Token.Value_Text != TEXT(",") && Token.Value_Text != TEXT("}")))
			{
				throw TextTokenizerError(TEXT("Expected comma or enum end"), Token, CurrentFile());
			}
			OnParsed_EnumValue(Value);
			if (Token.Value_Text == TEXT(",")) RequireToken(Token);
		}
		OnParsed_ScopeEnd();
	}

	void CppParser::Parse_Access(TextToken& Token, EAccessSpecifier Access)
	{
		RequireToken(Token, ETextTokenType::Symbol, TEXT(":"));
		OnParsed_Access(Access);
	}
}
