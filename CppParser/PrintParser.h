#pragma once
#include "CppParser.h"
#include <ostream>


namespace CE
{
	class CE_API PrintParser : public CppParser
	{
	public:
		PrintParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer, std::wostream& Output);
	

	protected:
		virtual void OnParseBegin() override;


	private:
		std::wostream& m_Output;
	};
}
