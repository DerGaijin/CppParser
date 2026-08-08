#pragma once
#include "Core.h"
#include "CppParser.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace CE
{
	enum class ECppStandard : uint8
	{
		Cpp17,
		Cpp20,
	};

	template<typename P = CppParser>
	class ParseManager
	{
	private:
		class Parser : public P
		{
		public:
			template<typename... ARGS>
			Parser(ParseManager* Manager, ARGS&&... Args) : P(std::forward<ARGS>(Args)...), m_Manager(Manager) { }

		protected:
			virtual bool OnParsed_Include(const std::filesystem::path& CurrentPath, const std::filesystem::path& Include, bool IsSystemInclude, std::filesystem::path& Path, String& Content) override
			{
				return m_Manager->ResolveInclude(CurrentPath, Include, IsSystemInclude, Path, Content);
			}

		private:
			ParseManager* m_Manager;
		};


	public:
		template<typename... PARGS>
		void Run(PARGS&&... ParserArgs)
		{
			for (auto& File : m_Files)
			{
				std::wifstream FileStream(File);
				if (!FileStream.is_open())
					continue;

				std::wstringstream Buffer;
				Buffer << FileStream.rdbuf();
				FileStream.close();

				String Content = Buffer.str();

				TextTokenizerInput_String Input(Content);
				TextTokenizer Tokenizer(Input);

				Parser parser(this, File, Tokenizer, std::forward<PARGS>(ParserArgs)...);
				parser.Definitions = Definitions;
				parser.Parse();
			}
		}

		void SetupEnvironment()
		{
			const String LanguageVersion = CppStandard == ECppStandard::Cpp17 ? L"201703L" : L"202002L";
			Definitions[L"__cplusplus"] = PreprocessorDefinition::Create({}, LanguageVersion);
			Definitions[L"_MSVC_LANG"] = PreprocessorDefinition::Create({}, LanguageVersion);
			Definitions[L"_MSC_VER"] = PreprocessorDefinition::Create({}, L"1944");
			Definitions[L"_HAS_STATIC_RTTI"] = PreprocessorDefinition::Create({}, L"1");
			Definitions[L"_M_IX86"] = PreprocessorDefinition::Create({}, L"600");
			Definitions[L"__cdecl"] = PreprocessorDefinition::Create({}, L"");
			Definitions[L"__pragma"] = PreprocessorDefinition::Create({ L"x" }, L"");

			Definitions[L"_In_"] = PreprocessorDefinition::Create({}, L"");
			Definitions[L"_Inout_"] = PreprocessorDefinition::Create({}, L"");
			Definitions[L"_Out_"] = PreprocessorDefinition::Create({}, L"");
			Definitions[L"_Check_return_"] = PreprocessorDefinition::Create({}, L"");
			Definitions[L"_Check_return_opt_"] = PreprocessorDefinition::Create({}, L"");
			Definitions[L"_Success_"] = PreprocessorDefinition::Create({ L"x" }, L"");
			Definitions[L"_Out_writes_z_"] = PreprocessorDefinition::Create({ L"x" }, L"");

			auto AddIncludeDirectory = [this](const std::filesystem::path& Directory)
				{
					if (!Directory.empty() && std::filesystem::exists(Directory))
					{
						IncludeDirectories.AddUnique(Directory);
					}
				};

			size_t IncludeEnvSize = 0;
			getenv_s(&IncludeEnvSize, nullptr, 0, "INCLUDE");
			if (IncludeEnvSize > 0)
			{
				std::vector<char> IncludeEnv(IncludeEnvSize);
				getenv_s(&IncludeEnvSize, IncludeEnv.data(), IncludeEnv.size(), "INCLUDE");

				std::stringstream Stream(IncludeEnv.data());
				std::string Directory;
				while (std::getline(Stream, Directory, ';'))
				{
					AddIncludeDirectory(Directory);
				}
			}

			if (IncludeDirectories.Size() > 0)
				return;

			std::filesystem::path MSVCRoot = "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC";
			std::filesystem::path LatestMSVC;
			if (std::filesystem::exists(MSVCRoot))
			{
				for (auto& Entry : std::filesystem::directory_iterator(MSVCRoot))
				{
					if (Entry.is_directory() && (LatestMSVC.empty() || Entry.path().filename().wstring() > LatestMSVC.filename().wstring()))
					{
						LatestMSVC = Entry.path();
					}
				}
			}

			AddIncludeDirectory(LatestMSVC / "include");

			std::filesystem::path WindowsKitsRoot = "C:/Program Files (x86)/Windows Kits/10/Include";
			std::filesystem::path LatestWindowsKit;
			if (std::filesystem::exists(WindowsKitsRoot))
			{
				for (auto& Entry : std::filesystem::directory_iterator(WindowsKitsRoot))
				{
					if (Entry.is_directory() && (LatestWindowsKit.empty() || Entry.path().filename().wstring() > LatestWindowsKit.filename().wstring()))
					{
						LatestWindowsKit = Entry.path();
					}
				}
			}

			AddIncludeDirectory(LatestWindowsKit / "ucrt");
			AddIncludeDirectory(LatestWindowsKit / "shared");
			AddIncludeDirectory(LatestWindowsKit / "um");
		}

		void AddFile(const std::filesystem::path& File)
		{
			m_Files.Add(File);
		}

		void AddDirectory(const std::filesystem::path& Directory)
		{
			if (!std::filesystem::exists(Directory))
				return;

			for (auto& Entry : std::filesystem::recursive_directory_iterator(Directory))
			{
				if (Entry.is_regular_file())
				{
					auto ext = Entry.path().extension();
					if (ext == L".h" || ext == L".hpp" || ext == L".hxx")
					{
						m_Files.Add(Entry.path());
					}
				}
			}
		}

		bool ResolveInclude(const std::filesystem::path& CurrentPath, const std::filesystem::path& Include, bool IsSystemInclude, std::filesystem::path& OutPath, String& OutContent)
		{
			auto TryRead = [&](const std::filesystem::path& FullPath) -> bool
				{
					if (!std::filesystem::exists(FullPath))
						return false;

					OutPath = FullPath.lexically_normal();
					return true;
				};

			// #include "file" � search relative to current file first
			if (!IsSystemInclude)
			{
				if (TryRead(CurrentPath.parent_path() / Include))
				{
					std::wifstream FileStream(OutPath);
					if (!FileStream.is_open())
						return false;

					std::wstringstream Buffer;
					Buffer << FileStream.rdbuf();
					OutContent = Buffer.str();
					return true;
				}
			}

			// #include <file> or #include "file" � search include directories
			for (auto& Dir : IncludeDirectories)
			{
				if (TryRead(Dir / Include))
				{
					if (IsSystemInclude && !ParseSystemIncludes)
					{
						OutContent.Clear();
						return true;
					}

					std::wifstream FileStream(OutPath);
					if (!FileStream.is_open())
						return false;

					std::wstringstream Buffer;
					Buffer << FileStream.rdbuf();
					OutContent = Buffer.str();
					return true;
				}
			}

			return false;
		}


	public:
		ECppStandard CppStandard = ECppStandard::Cpp20;
		bool ParseSystemIncludes = true;
		Map<String, PreprocessorDefinition> Definitions;
		Array<std::filesystem::path> IncludeDirectories;


	private:
		Array<std::filesystem::path> m_Files;
	};
}
