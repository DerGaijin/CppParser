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
		}

		OnParseEnd();
	}
}
