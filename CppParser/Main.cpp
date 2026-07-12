#include "TextTokenizerInput_String.h"
#include "PrintParser.h"
#include "ParseManager.h"
#include "UnitTest.hpp"
using namespace CE;

#include <fstream>
#include <iostream>
#include <sstream>

int main()
{
	ParseManager<PrintParser> PM;
	PM.ParseSystemIncludes = false;
	PM.SetupEnvironment();
	PM.AddFile("UnitTest.hpp");
	PM.Definitions["CE_API"] = PreprocessorDefinition::Create({}, L"");

	std::wofstream ResultFile("Result.hpp");

	try
	{
		PM.Run(ResultFile);
	}
	catch (const CE::TextTokenizerError& Error)
	{
		std::wcerr << Error.File.wstring() << L":" << Error.Line << L":" << Error.LinePos << L": " << (std::wstring)Error.Message << L"\n";
		ResultFile.flush();
		ResultFile.close();
		system("pause");
		return 1;
	}

	ResultFile.flush();
	ResultFile.close();

	return 0;
}
