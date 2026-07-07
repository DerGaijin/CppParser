#pragma once
#include "Core.h"


namespace CE
{
	class TextTokenizerInput;


	enum class ETextTokenType
	{
		Undefined,
		Identifier,
		Constant,
		Symbol,
	};


	enum class ETextTokenConstantType
	{
		Undefined,
		Text,
		Integral,
		Double,
		Boolean,
	};

	struct CE_API TextToken
	{
		size_t Pos = 0;
		size_t Line = 0;
		size_t LinePos = 0;
		Array<WChar> Whitespaces;
		String RawText;

		ETextTokenType Type = ETextTokenType::Undefined;
		ETextTokenConstantType ConstantType = ETextTokenConstantType::Undefined;
		String Value_Text;
		union
		{
			int64_t Value_Integral = 0;
			double Value_Double;
			bool Value_Boolean;
		};
		bool IsRawText = false;
	};


	struct TextTokenizerConfig
	{
		bool LineContinuations = true;
		Array<std::pair<WChar, WChar>> SymbolPairs;
	};


	class CE_API TextTokenizerBase
	{
	public:
		virtual ~TextTokenizerBase() = default;

		virtual bool GetToken(TextToken& Token) = 0;
	};


	class CE_API TextTokenizer : public TextTokenizerBase
	{
	public:
		static bool IsWhitespace(WChar Char);

		static bool IsEndOfLine(WChar Char);

	public:
		TextTokenizer(TextTokenizerInput& Input) : m_Input(Input) {}

		TextTokenizer(TextTokenizerInput& Input, const TextTokenizerConfig& Config) : m_Input(Input), Config(Config) {}

		virtual WChar GetChar();

		virtual WChar PeekChar(size_t Offset = 0);

		WChar GetLeadingChar(Array<WChar>* Whitespaces = nullptr);

		virtual bool GetToken(TextToken& Token) override;


	public:
		inline size_t GetPos() const
		{
			return m_Pos;
		}

		inline size_t GetLine() const
		{
			return m_Line;
		}

		inline size_t GetLinePos() const
		{
			return m_LinePos;
		}



	private:
		void ProcessToken_RawStringLiteral(WChar Char, WChar Peek, TextToken& Token);
		void ProcessToken_StringLiteral(WChar Char, WChar Peek, TextToken& Token);
		void ProcessToken_CharLiteral(WChar Char, WChar Peek, TextToken& Token);
		void ProcessToken_Identifier(WChar Char, WChar Peek, TextToken& Token);
		void ProcessToken_Constant(WChar Char, WChar Peek, TextToken& Token);
		void ProcessToken_Symbol(WChar Char, WChar Peek, TextToken& Token);


	public:
		TextTokenizerConfig Config;


	private:
		TextTokenizerInput& m_Input;

		size_t m_Pos = 0;
		size_t m_Line = 1;
		size_t m_LinePos = 0;

	};


	class CE_API TextTokenizerError
	{
	public:
		TextTokenizerError(const String& Msg, size_t Pos, size_t Length, size_t Line, size_t LinePos, const std::filesystem::path& File);

		TextTokenizerError(const String& Msg, size_t Pos, size_t Length, size_t Line, size_t LinePos);

		TextTokenizerError(const String& Msg, const TextTokenizer& Tokenizer, size_t Length, const std::filesystem::path& File);

		TextTokenizerError(const String& Msg, const TextTokenizer& Tokenizer, size_t Length);

		TextTokenizerError(const String& Msg, const TextToken& Token, const std::filesystem::path& File);

		TextTokenizerError(const String& Msg, const TextToken& Token);


	public:
		String Message;
		size_t Pos = 0;
		size_t Length = 0;
		size_t Line = 1;
		size_t LinePos = 0;
		std::filesystem::path File;
	};
}