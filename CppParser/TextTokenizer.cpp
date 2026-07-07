#include "TextTokenizer.h"
#include "TextTokenizerInput.h"


namespace CE
{
	bool TextTokenizer::IsWhitespace(WChar Char)
	{
		return Char == ' ' || Char == '\t' || Char == '\r' || Char == '\n';
	}

	bool TextTokenizer::IsEndOfLine(WChar Char)
	{
		return Char == '\n' || Char == '\r' || Char == '\0';
	}

	WChar TextTokenizer::GetChar()
	{
		WChar C = m_Input.GetChar();

		bool SkipLineContinuation = false;
		if (Config.LineContinuations)
		{
			WChar P = m_Input.PeekChar();
			if (C == '\\' && IsEndOfLine(P))
			{
				SkipLineContinuation = true;
			}
		}

		m_Pos++;
		m_LinePos++;
		if (IsEndOfLine(C))
		{
			m_Line++;
			m_LinePos = 0;
		}

		if (SkipLineContinuation)
		{
			WChar Skipped = m_Input.GetChar(); // Skip Peeked Char
			if (IsEndOfLine(Skipped))
			{
				m_Line++;
				m_LinePos = 0;
			}
			return GetChar();
		}

		return C;
	}

	WChar TextTokenizer::PeekChar(size_t Offset /*= 0*/)
	{
		WChar P = m_Input.PeekChar(Offset);

		if (Config.LineContinuations)
		{
			WChar N = m_Input.PeekChar(Offset + 1);
			if (P == '\\' && IsEndOfLine(N))
			{
				return PeekChar(Offset + 2);
			}
		}

		return P;
	}

	WChar TextTokenizer::GetLeadingChar(Array<WChar>* Whitespaces /*= nullptr*/)
	{
		while (true)
		{
			WChar C = GetChar();
			if (IsWhitespace(C))
			{
				if (Whitespaces != nullptr)
				{
					Whitespaces->Add(C);
				}
			}
			else
			{
				return C;
			}
		}
	}

	bool TextTokenizer::GetToken(TextToken& Token)
	{
		// Clear Token
		Token.Pos = 0;
		Token.Line = 0;
		Token.LinePos = 0;
		Token.Whitespaces.Clear(2);
		Token.RawText.Clear(20);
		Token.Type = ETextTokenType::Undefined;
		Token.ConstantType = ETextTokenConstantType::Undefined;
		Token.Value_Text.Clear(20);
		Token.Value_Integral = 0;
		Token.IsRawText = false;

		// Setup Token
		WChar Char = GetLeadingChar(&Token.Whitespaces);
		if (Char == '\0')
		{
			return false;
		}
		WChar Peek = PeekChar(0);
		Token.Pos = m_Pos > 0 ? m_Pos - 1 : m_Pos;
		Token.Line = m_Line;
		Token.LinePos = m_LinePos > 0 ? m_LinePos - 1 : m_LinePos;

		// Raw String Literal
		if (Char == 'R' && Peek == '"')
		{
			ProcessToken_RawStringLiteral(Char, Peek, Token);
		}
		// String Literals
		else if (Char == '"')
		{
			ProcessToken_StringLiteral(Char, Peek, Token);
		}
		// Char Literals
		else if (Char == '\'')
		{
			ProcessToken_CharLiteral(Char, Peek, Token);
		}
		// Identifiers
		else if ((Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z') || Char == '_')
		{
			ProcessToken_Identifier(Char, Peek, Token);
		}
		// Constants
		else if ((Char >= '0' && Char <= '9') || ((Char == '+' || Char == '-') && (Peek >= '0' && Peek <= '9')))
		{
			ProcessToken_Constant(Char, Peek, Token);
		}
		// Symbols
		else
		{
			ProcessToken_Symbol(Char, Peek, Token);
		}

		return true;
	}

	void TextTokenizer::ProcessToken_RawStringLiteral(WChar Char, WChar Peek, TextToken& Token)
	{
		Token.Type = ETextTokenType::Constant;
		Token.ConstantType = ETextTokenConstantType::Text;
		Token.IsRawText = true;

		Token.RawText += Char;
		Token.RawText += GetChar(); // Skip '"' Peeked Char

		String Suffix;
		Char = GetChar();
		while (Char != '(' && Char != '\0')
		{
			Suffix += Char;
			Token.RawText += Char;
			Char = GetChar();
		}

		if (Char != '(')
		{
			throw TextTokenizerError(TEXT("Incomplete Prefix on Raw string"), *this, 1);
		}

		Token.RawText += Char;


		Suffix.Insert(0, ')');
		Suffix.Append('"');

		Char = GetChar();
		while (Char != '\0')
		{
			Token.Value_Text += Char;
			Token.RawText += Char;

			if (Suffix.Size() <= Token.Value_Text.Size() && Token.Value_Text.EndsWith(Suffix))
			{
				Token.Value_Text.RemoveAt(Suffix.Size(), Token.Value_Text.Size());
				break;
			}

			Char = GetChar();
		}

		if (Char == '\0')
		{
			throw TextTokenizerError(TEXT("Unterminated Raw string constant"), *this, 1);
		}
	}

	void TextTokenizer::ProcessToken_StringLiteral(WChar Char, WChar Peek, TextToken& Token)
	{
		Token.Type = ETextTokenType::Constant;
		Token.ConstantType = ETextTokenConstantType::Text;

		Token.RawText += Char;
		Char = GetChar();
		WChar LastChar = '\0';
		while (true)
		{
			Token.RawText += Char;
			if (Char == '"' && LastChar != '\\')
			{
				break;
			}

			Token.Value_Text += Char;
			LastChar = Char;
			Char = GetChar();
		}

		if (Char != '"')
		{
			throw TextTokenizerError(TEXT("Unterminated string constant"), *this, 1);
		}
	}

	void TextTokenizer::ProcessToken_CharLiteral(WChar Char, WChar Peek, TextToken& Token)
	{
		Token.Type = ETextTokenType::Constant;
		Token.ConstantType = ETextTokenConstantType::Integral;

		Token.RawText += Char;

		std::wstring Temp;
		Char = GetChar();
		while ((Char != '\'') && Char != '\0')
		{
			if (Token.Value_Text.Size() == 4)
			{
				throw TextTokenizerError(TEXT("Too many characters in char literal"), *this, 1);
			}

			Token.RawText += Char;
			if (Char == '\\')
			{
				Char = GetChar();
				Token.RawText += Char;
				switch (Char)
				{
				case 'n':
					Char = '\n';
					break;
				case 't':
					Char = '\t';
					break;
				case 'v':
					Char = '\v';
					break;
				case 'b':
					Char = '\b';
					break;
				case 'r':
					Char = '\r';
					break;
				case 'f':
					Char = '\f';
					break;
				case 'a':
					Char = '\a';
					break;
				case '\\':
					Char = '\\';
					break;
				case '?':
					Char = '\?';
					break;
				case '\'':
					Char = '\'';
					break;
				case '"':
					Char = '\"';
					break;
				case 'x':
				{
					std::wstring HexStr = L"0";
					HexStr += Char;
					for (char i = 0; i < 4; i++)
					{
						Peek = std::toupper(PeekChar(0));
						if ((Peek >= '0' && Peek <= '9') || (Peek >= 'A' && Peek <= 'F'))
						{
							Token.RawText += Peek;
							HexStr += GetChar();
						}
						else
						{
							break;
						}
					}

					WChar* End = (WChar*)HexStr.data() + HexStr.size();
					Char = (WChar)std::wcstol(HexStr.c_str(), &End, 0);	//@TODO: Use Traits here
					break;
				}
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				{
					std::wstring OctStr;
					OctStr += Char;
					for (char i = 0; i < 2; i++)
					{
						Peek = PeekChar(0);
						if (Peek >= '0' && Peek <= '7')
						{
							Token.RawText += Peek;
							OctStr += GetChar();
						}
						else
						{
							break;
						}
					}

					WChar* End = (WChar*)OctStr.data() + OctStr.size();
					Char = (WChar)std::wcstol(OctStr.c_str(), &End, 8); //@TODO: Use Traits here

					break;
				}
				default:
					break;
				}
			}

			Token.Value_Text += Char;
			Temp += Char;

			Char = GetChar();
		}

		if (Char != '\'')
		{
			throw TextTokenizerError(TEXT("Unterminated char literal"), *this, 1);
		}

		Token.RawText += Char;

		int Val = 0;
		if (Temp.size() == 4)
		{
			Val = int((unsigned char)(Temp[0]) << 24 |
				(unsigned char)(Temp[1]) << 16 |
				(unsigned char)(Temp[2]) << 8 |
				(unsigned char)(Temp[3]));
		}
		else if (Temp.size() == 3)
		{
			Val = int((unsigned char)(Temp[0]) << 16 |
				(unsigned char)(Temp[1]) << 8 |
				(unsigned char)(Temp[2]));
		}
		else if (Temp.size() == 2)
		{
			Val = int((unsigned char)(Temp[0]) << 8 |
				(unsigned char)(Temp[1]));
		}
		else
		{
			Val = int((unsigned char)(Temp[0]));
		}

		Token.Value_Integral = Val;
	}

	void TextTokenizer::ProcessToken_Identifier(WChar Char, WChar Peek, TextToken& Token)
	{
		Token.Type = ETextTokenType::Identifier;

		Token.Value_Text += Char;
		Token.RawText += Char;
		while (true)
		{
			Peek = PeekChar(0);
			if (((Peek >= 'A') && (Peek <= 'Z')) || ((Peek >= 'a') && (Peek <= 'z')) || ((Peek >= '0') && (Peek <= '9')) || (Peek == '_'))
			{
				Token.Value_Text += Peek;
				Token.RawText += Peek;
				Char = GetChar();
			}
			else
			{
				break;
			}
		}

		if (Token.Value_Text == L"true")
		{
			Token.Type = ETextTokenType::Constant;
			Token.ConstantType = ETextTokenConstantType::Boolean;
			Token.Value_Boolean = true;
		}
		else if (Token.Value_Text == L"false")
		{
			Token.Type = ETextTokenType::Constant;
			Token.ConstantType = ETextTokenConstantType::Boolean;
			Token.Value_Boolean = false;
		}
	}

	void TextTokenizer::ProcessToken_Constant(WChar Char, WChar Peek, TextToken& Token)
	{
		Token.Type = ETextTokenType::Constant;

		std::wstring Temp;
		if (Char == '0' && (Peek == 'x' || Peek == 'X'))
		{
			Token.Value_Text += Char;
			Token.RawText += Char;
			Token.Value_Text += Peek;
			Token.RawText += Peek;
			Temp += Char;
			Temp += std::tolower(GetChar());
			while (true)
			{
				Peek = std::toupper(PeekChar(0));
				if ((Peek >= '0' && Peek <= '9') || (Peek >= 'A' && Peek <= 'F'))
				{
					Token.Value_Text += Peek;
					Temp += Peek;
					Token.RawText += GetChar();
				}
				//@Note: It is possible to write a number like 10'000 so we ignore the separators
				else if (Peek == '\'')
				{
					Token.RawText += GetChar();
				}
				else
				{
					break;
				}
			}

			WChar* End = (WChar*)Temp.data() + Temp.size();
			Token.ConstantType = ETextTokenConstantType::Integral;
			Token.Value_Integral = std::wcstoll(Temp.c_str(), &End, 0);
		}
		else if (Char == '0' && (Peek == 'b' || Peek == 'B'))
		{
			Token.Value_Text += Peek;
			Token.RawText += Peek;
			GetChar();
			while (true)
			{
				Peek = PeekChar(0);
				if (Peek >= '0' && Peek <= '1')
				{
					Token.Value_Text += Peek;
					Token.RawText += Peek;
					Temp += GetChar();
				}
				else if (Peek == '\'')
				{
					Token.RawText += GetChar();
				}
				else
				{
					break;
				}
			}

			WChar* End = (WChar*)Temp.data() + Temp.size();
			Token.ConstantType = ETextTokenConstantType::Integral;
			Token.Value_Integral = std::wcstoll(Temp.c_str(), &End, 2);
		}
		else
		{
			Token.Value_Text += Char;
			Token.RawText += Char;
			Temp += Char;

			bool IsFloat = false;
			while (true)
			{
				Peek = PeekChar(0);
				if ((Peek >= '0' && Peek <= '9') || Peek == '.')
				{
					if (Peek == '.')
					{
						if (IsFloat)
						{
							throw TextTokenizerError(TEXT("Float/Double separator already found"), *this, 1);
						}
						else
						{
							IsFloat = true;
						}
					}
					Token.Value_Text += Peek;
					Token.RawText += Peek;
					Temp += GetChar();
				}
				else if (Peek == '\'')
				{
					Char = GetChar();
					Token.RawText += Peek;
				}
				else if (Peek == 'l' || Peek == 'L' || Peek == 'u' || Peek == 'U')
				{
					//@TODO: Don't just ignore them, handle them
					Char = GetChar();
					Token.RawText += Peek;
				}
				else
				{
					break;
				}
			}
			if (IsFloat && (Peek == 'f' || Peek == 'F'))
			{
				Temp += Peek;
				Token.Value_Text += Peek;
				Token.RawText += Peek;
				Char = GetChar();
			}

			if (IsFloat)
			{
				Token.ConstantType = ETextTokenConstantType::Double;
				Token.Value_Double = _wtof(Temp.c_str());
			}
			else
			{
				Token.ConstantType = ETextTokenConstantType::Integral;
				Token.Value_Integral = _wtoll(Temp.c_str());
			}
		}
	}

	void TextTokenizer::ProcessToken_Symbol(WChar Char, WChar Peek, TextToken& Token)
	{
		Token.Type = ETextTokenType::Symbol;

		Token.Value_Text += Char;
		Token.RawText += Char;

		for (auto& It : Config.SymbolPairs)
		{
			if (It.first == Char && It.second == Peek)
			{
				GetChar();
				Token.Value_Text += Peek;
				Token.RawText += Peek;
				return;
			}
		}
	}

	TextTokenizerError::TextTokenizerError(const String& Msg, size_t Pos, size_t Length, size_t Line, size_t LinePos, const std::filesystem::path& File)
		: Message(Msg), Pos(Pos), Length(Length), Line(Line), LinePos(LinePos), File(File)
	{

	}

	TextTokenizerError::TextTokenizerError(const String& Msg, size_t Pos, size_t Length, size_t Line, size_t LinePos)
		: Message(Msg), Pos(Pos), Length(Length), Line(Line), LinePos(LinePos)
	{

	}

	TextTokenizerError::TextTokenizerError(const String& Msg, const TextTokenizer& Tokenizer, size_t Length, const std::filesystem::path& File)
		: Message(Msg), Pos(Tokenizer.GetPos()), Length(Length), Line(Tokenizer.GetLine()), LinePos(Tokenizer.GetLinePos()), File(File)
	{

	}

	TextTokenizerError::TextTokenizerError(const String& Msg, const TextTokenizer& Tokenizer, size_t Length)
		: Message(Msg), Pos(Tokenizer.GetPos()), Length(Length), Line(Tokenizer.GetLine()), LinePos(Tokenizer.GetLinePos())
	{

	}

	TextTokenizerError::TextTokenizerError(const String& Msg, const TextToken& Token, const std::filesystem::path& File)
		: Message(Msg), Pos(Token.Pos), Length(Token.RawText.Size()), Line(Token.Line), LinePos(Token.LinePos), File(File)
	{

	}

	TextTokenizerError::TextTokenizerError(const String& Msg, const TextToken& Token)
		: Message(Msg), Pos(Token.Pos), Length(Token.RawText.Size()), Line(Token.Line), LinePos(Token.LinePos)
	{

	}
}
