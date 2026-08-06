#pragma once
#include "Core.h"
#include "TextTokenizer.h"
#include "TextTokenizerInput_String.h"


namespace CE
{
	class CE_API PreprocessorDefinition
	{
	public:
		static PreprocessorDefinition Create(const Array<String>& Parameters, const String& Text, bool RequireParameters = false);

		void SetRequireParameters(bool RequireParameters);

		void AddParameter(const String& Parameter);

		void AddToken(TextToken& Token);

		String Resolve(const Array<String>& Parameters) const;


	public:
		const String& Text() const
		{
			return m_Text;
		}

		bool RequireParameters() const
		{
			return m_RequireParameters;
		}

		const Array<String>& Parameters() const
		{
			return m_Parameters;
		}

		const String& ResolvedText() const
		{
			return m_ResolvedText;
		}

		bool IsLastVariadic() const;


	private:
		String m_Text;
		Array<String> m_Parameters;
		bool m_RequireParameters = false;

		String m_ResolvedText;
		MultiMap<size_t, size_t> m_ParameterResolves;

		bool m_NextTokenIsString = false;
		bool m_NextTokenIsWhitespaceless = false;
		bool m_PrevTokenWasNumber = false;
	};


	class CE_API Preprocessor : public TextTokenizerBase
	{
	private:
		struct SubTokenizer
		{
			std::filesystem::path Path;
			String DefinitionName;
			SharedPtr<String> Text;
			SharedPtr<TextTokenizerInput_String> TokenizerInput;
			SharedPtr<TextTokenizer> Tokenizer;
			Array<WChar> Whitespaces;
		};

		enum class EBlockState
		{
			Disabled,
			Enabled,
			Completed,
		};


	public:
		static bool IsTokenOnNewLine(const TextToken& Token);


	public:
		Preprocessor(const std::filesystem::path& Path, TextTokenizer& Tokenizer);

		virtual bool GetToken(TextToken& Token) override;


	public:
		const std::filesystem::path& CurrentFile() const;

		const TextTokenizer& CurrentFileTokenizer() const;

		const TextTokenizer& CurrentTokenizer() const;


	protected:
		virtual void OnParseBegin() {}

		virtual void OnParseEnd() {}

		virtual bool OnParsed_Comment(const String& Comment, bool IsMultiline) { return true; }

		virtual bool OnParsed_Include(const std::filesystem::path& CurrentPath, const std::filesystem::path& Include, bool IsSystemInclude, std::filesystem::path& Path, String& Content) { return true; }

		virtual bool OnParsed_Error(const String& Error) { return true; }

		virtual bool OnParsed_Define(const String& Name, const PreprocessorDefinition& Definition) { return true; }

		virtual bool OnParsed_Undefine(const String& Name) { return true; }


	private:
		static std::filesystem::path NormalizeFileIdentity(const std::filesystem::path& Path);

		bool HasPragmaOnce(const std::filesystem::path& Path) const;

		void RegisterPragmaOnce(const std::filesystem::path& Path);

		TextTokenizer& GetActiveTokenizer();

		void PushSubTokenizer(const std::filesystem::path& Path, const String& DefinitionName, SharedPtr<String> Text, const Array<WChar>& Whitespace);

		void PopSubTokenizer();

		bool IsBlockEnabled() const;

		void PushBlockState(EBlockState State);

		void PopBlockState();

		void SetDefinition(const String& Name, const PreprocessorDefinition& Definition);

		bool GetTokenFromActiveTokenizer(TextToken& Token);

		bool PreprocessComment(TextToken& Token, bool ProcessComment);

		bool GetTokenPreprocessedComments(TextToken& Token, bool ProcessComment);

		bool PreprocessDirective(TextToken& Token, bool& TokenIsValid);

		void SkipDirectiveEnd(TextToken& Token, bool ProcessComment, bool& TokenIsValid);

		bool PreprocessDirective_Pragma(TextToken& Token, bool& TokenIsValid);

		bool PreprocessDirective_Line(TextToken& Token, bool& TokenIsValid);

		bool PreprocessDirective_Include(TextToken& Token, bool& TokenIsValid, const Array<WChar>& Whitespaces);

		bool PreprocessDirective_Error(TextToken& Token, bool& TokenIsValid);

		bool PreprocessDirective_Define(TextToken& Token, bool& TokenIsValid);

		bool PreprocessDirective_Undefine(TextToken& Token, bool& TokenIsValid);

		bool PreprocessDirective_Condition(TextToken& Token, bool& TokenIsValid);

		bool PreprocessDefinition(TextToken& Token, bool& TokenIsValid);

		bool PreprocessCondition(TextToken& Token, bool ProcessComment, bool& TokenIsValid);


	public:
		Map<String, PreprocessorDefinition> Definitions;


	private:
		std::filesystem::path m_Path;
		TextTokenizer& m_Tokenizer;

		Array<SubTokenizer> m_SubTokenizers;
		Array<EBlockState> m_BlockStack;
		Array<std::filesystem::path> m_PragmaOnceFiles;
	};
}
