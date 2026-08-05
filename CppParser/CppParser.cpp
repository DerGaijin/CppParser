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

	void CppParser::Parse_Name(TextToken& Token, ParsedName& Name, bool AllowTemplate, bool AllowInline, bool AllowLeadingScope)
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

			if (Token.Type != ETextTokenType::Symbol || Token.Value_Text != TEXT(":"))
			{
				return;
			}

			RequireToken(Token, ETextTokenType::Symbol, TEXT(":"));
			RequireToken(Token);
		}
	}

	void CppParser::Parse_Namespace(TextToken& Token, bool IsInline)
	{
		RequireToken(Token);
		if (Token.Type == ETextTokenType::Identifier)
		{
			ParsedNamespace Namespace;
			Parse_Name(Token, Namespace.Name, false, true, false);
			Namespace.Name.Segments[0].IsInline = IsInline;
			if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == TEXT("="))
			{
				ParsedNamespaceAlias Alias;
				Alias.Name = Namespace.Name;
				RequireToken(Token);
				Parse_Name(Token, Alias.Target, false, false, true);
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

	void CppParser::Parse_Access(TextToken& Token, EAccessSpecifier Access)
	{
		RequireToken(Token, ETextTokenType::Symbol, TEXT(":"));
		OnParsed_Access(Access);
	}
}

/* Keywords - Type
unsigned
signed
const
volatile
========
decltype
typename
========
class
struct
union
enum
*/

/* Keywords - Variable
static
thread_local
mutable
constexpr
extern
*/

/* Keywords - Function
inline
friend
final
virtual
static
consteval
explicit
noexcept
default
delete
extern
*/

/* Keywords
namespace
inline
class
public
protected
private
__declspec
friend
struct
final
alignas
virtual
enum
unsigned
signed
const
volatile
static
thread_local
mutable
typename
constexpr
consteval
decltype
explicit
noexcept
default
delete
typedef
using
union
extern
template
operator
concept
requires
static_assert
*/
