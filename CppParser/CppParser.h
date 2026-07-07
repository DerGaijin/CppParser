#pragma once
#include "Core.h"
#include "Preprocessor.h"


namespace CE
{
	class CE_API CppParser : public Preprocessor
	{
	public:
		CppParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer);

		virtual void Parse();
	};
}
