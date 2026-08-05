# CppParser

CppParser is a C++17 source parser and preprocessor for turning C++ code into structured declaration data. It handles tokenization, macro expansion, conditional compilation, include resolution, and declarations such as namespaces, classes, enums, variables, functions, operators, templates, concepts, attributes, and type aliases.

Its callback-based architecture supports custom code analysis and generation tools, while `PrintParser` can reconstruct parsed declarations as C++ source.

## Usage

Use `ParseManager` with `PrintParser` to parse a file and write its declarations:

```cpp
#include "ParseManager.h"
#include "PrintParser.h"

#include <fstream>

int main()
{
    CE::ParseManager<CE::PrintParser> manager;
    manager.ParseSystemIncludes = false;
    manager.SetupEnvironment();
    manager.AddFile("Example.hpp");

    std::wofstream output("Parsed.hpp");
    manager.Run(output);
}
```

For custom processing, derive from `CE::CppParser` and override the `OnParsed_*` callbacks in `CppParser/CppParser.h`.
