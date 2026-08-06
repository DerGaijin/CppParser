#include "TextTokenizerInput_String.h"
#include "PrintParser.h"
#include "ParseManager.h"
using namespace CE;

#include <fstream>
#include <iostream>
#include <sstream>

class TempParser : public CppParser
{
public:
	TempParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer, std::wostream& Output) : CppParser(Path, Tokenizer) {}
protected:
	void OnParseBegin() override
	{
		std::cout << "Parsing: " << CurrentFile() << std::endl;
	}
};

int main()
{
	//ParseManager<PrintParser> PM;
	ParseManager<TempParser> PM;
	PM.ParseSystemIncludes = true;
	PM.SetupEnvironment();
#if 0
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
		PM.Run(ResultFile);
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

	system("pause");
	return 0;
}
