#include "TextTokenizerInput_String.h"
#include "PrintParser.h"
#include "ParseManager.h"
using namespace CE;

#include <fstream>
#include <iostream>
#include <sstream>

#define USE_PRINT_PARSER 0

struct ParserContext
{
	std::atomic<size_t> FileCounter = 0;
};

class TempParser : public CppParser
{
public:
	TempParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer, ParserContext& Context) : CppParser(Path, Tokenizer), m_Ctx(Context) {}
protected:
	void OnParseBegin() override
	{
		size_t FileIdx = m_Ctx.FileCounter += 1;
		std::cout << "[" << FileIdx << "] " << "Parsing: " << CurrentFile() << std::endl;
	}
private:
	ParserContext& m_Ctx;
};

int main()
{
#if USE_PRINT_PARSER
	ParseManager<PrintParser> PM;
#else
	ParseManager<TempParser> PM;
#endif
	PM.ParseSystemIncludes = true;
	PM.CppStandard = ECppStandard::Cpp17;
	PM.SetupEnvironment();
#if 1
	PM.AddFile("UnitTest.hpp");
#else
	PM.AddFile("Array.h");
	PM.AddFile("Calculation.h");
	PM.AddFile("Core.h");
	PM.AddFile("CppParser.h");
	PM.AddFile("Definitions.h");
	PM.AddFile("ParsedData.h");
	PM.AddFile("ParseManager.h");
	PM.AddFile("Preprocessor.h");
	PM.AddFile("PrintParser.h");
	PM.AddFile("String.h");
	PM.AddFile("TextTokenizer.h");
	PM.AddFile("TextTokenizerInput.h");
	PM.AddFile("TextTokenizerInput_RawStr.h");
	PM.AddFile("TextTokenizerInput_Stream.h");
	PM.AddFile("TextTokenizerInput_String.h");
	PM.AddFile("UnitTest.hpp");
#endif
	PM.Definitions["CE_API"] = PreprocessorDefinition::Create({}, L"");

	std::wofstream ResultFile("Result.hpp");

	try
	{
#if USE_PRINT_PARSER
		PM.Run(ResultFile);
#else
		ParserContext Ctx;
		PM.Run(Ctx);
#endif
	}
	catch (const CE::TextTokenizerError& Error)
	{
		ResultFile.flush();
		ResultFile.close();
		std::wcout << Error.File.wstring() << L":" << Error.Line << L":" << Error.LinePos << L": " << (std::wstring)Error.Message << L"\n";
		std::ifstream ErrorFile(Error.File);
		if (ErrorFile.is_open())
		{
			std::string ErrorLine;
			for (size_t Line = 1; Line <= Error.Line && std::getline(ErrorFile, ErrorLine); Line++)
			{
				if (Line == Error.Line)
				{
					std::cout << ErrorLine << '\n';
					break;
				}
			}
			ErrorFile.close();
		}
		system("pause");
		return 1;
	}

	ResultFile.flush();
	ResultFile.close();

#if !USE_PRINT_PARSER
	system("pause");
#endif
	return 0;
}
