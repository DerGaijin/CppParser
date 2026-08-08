#include "Preprocessor.h"
#include "Calculation.h"


namespace CE
{
	PreprocessorDefinition PreprocessorDefinition::Create(const Array<String>& Parameters, const String& Text, bool RequireParameters /*= false*/)
	{
		PreprocessorDefinition Result;

		Result.SetRequireParameters(RequireParameters);

		for (auto& Param : Parameters)
		{
			Result.AddParameter(Param);
		}

		TextTokenizerInput_String Input(Text);
		TextTokenizer TempTokenizer = TextTokenizer(Input);
		TextToken Token;
		while (TempTokenizer.GetToken(Token))
		{
			Result.AddToken(Token);
		}

		return Result;
	}

	void PreprocessorDefinition::SetRequireParameters(bool RequireParameters)
	{
		m_RequireParameters = m_Parameters.Size() > 0 ? true : RequireParameters;
	}

	void PreprocessorDefinition::AddParameter(const String& Parameter)
	{
		m_RequireParameters = true;
		m_Parameters.Emplace(Parameter);
	}

	void PreprocessorDefinition::AddToken(TextToken& Token)
	{
		if (Token.Whitespaces.Size() > 0)
		{
			m_Text += std::wstring(Token.Whitespaces.Data(), Token.Whitespaces.Size());
		}
		m_Text += Token.RawText;

		if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == L"#")
		{
			if (m_NextTokenIsString)
			{
				m_NextTokenIsString = false;
				m_NextTokenIsWhitespaceless = true;
			}
			else
			{
				m_NextTokenIsString = true;
			}
			return;
		}

		if (!m_NextTokenIsWhitespaceless && Token.Whitespaces.Size() > 0 && m_ResolvedText.Size() > 0)
		{
			m_ResolvedText += std::wstring(Token.Whitespaces.Data(), Token.Whitespaces.Size());
		}

		bool TokenIsParameter = false;
		if (Token.Type == ETextTokenType::Identifier && !m_PrevTokenWasNumber && m_Parameters.Size() > 0)
		{
			bool LastVariadic = IsLastVariadic();
			for (size_t ParamIdx = 0; ParamIdx < m_Parameters.Size(); ParamIdx++)
			{
				if (Token.Value_Text == ((LastVariadic && ParamIdx == m_Parameters.Size() - 1) ? L"__VA_ARGS__" : m_Parameters[ParamIdx]))
				{
					if (m_NextTokenIsString)
					{
						m_ResolvedText += L"\"";
					}
					m_ParameterResolves.emplace(m_ResolvedText.Size(), ParamIdx);
					if (m_NextTokenIsString)
					{
						m_ResolvedText += L"\"";
					}
					TokenIsParameter = true;
					break;
				}
			}

		}

		if (!TokenIsParameter)
		{
			m_ResolvedText += Token.RawText;
		}

		m_NextTokenIsString = false;
		m_NextTokenIsWhitespaceless = false;
		m_PrevTokenWasNumber = Token.Type == ETextTokenType::Constant && (Token.ConstantType == ETextTokenConstantType::Integral || Token.ConstantType == ETextTokenConstantType::Double);
	}

	String PreprocessorDefinition::Resolve(const Array<String>& Parameters) const
	{
		bool LastVariadic = IsLastVariadic();
		std::wstring Result = m_ResolvedText;
		for (auto It = m_ParameterResolves.rbegin(); It != m_ParameterResolves.rend(); It++)
		{
			if (LastVariadic && It->second == m_Parameters.Size() - 1)
			{
				std::wstring Arg;
				size_t Idx = It->second;
				while (Idx < Parameters.Size())
				{
					if (Idx != It->second)
					{
						Arg += L", ";
					}
					Arg += Parameters[Idx];
					Idx++;
				}

				if (Arg.size() >= 0)
				{
					Result.insert(It->first, Arg);
				}
			}
			else if (It->second < Parameters.Size())
			{
				Result.insert(It->first, Parameters[It->second]);
			}
		}

		return Result;
	}

	bool PreprocessorDefinition::IsLastVariadic() const
	{
		return m_Parameters.Size() > 0 ? m_Parameters[m_Parameters.Size() - 1] == L"..." : false;
	}

	bool Preprocessor::IsTokenOnNewLine(const TextToken& Token)
	{
		for (auto& C : Token.Whitespaces)
		{
			if (TextTokenizer::IsEndOfLine(C))
			{
				return true;
			}
		}
		return false;
	}

	Preprocessor::Preprocessor(const std::filesystem::path& Path, TextTokenizer& Tokenizer) : m_Path(Path), m_Tokenizer(Tokenizer)
	{
		// Preprocessor double Symbols for Comments
		Tokenizer.Config.SymbolPairs.AddUnique({ '/', '/' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '/', '*' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '*', '/' });

		// Preprocessor double Symbols for Calculations
		Tokenizer.Config.SymbolPairs.AddUnique({ '<', '<' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '>', '>' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '=', '=' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '!', '=' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '>', '=' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '<', '=' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '&', '&' });
		Tokenizer.Config.SymbolPairs.AddUnique({ '|', '|' });
	}

	std::filesystem::path Preprocessor::NormalizeFileIdentity(const std::filesystem::path& Path)
	{
		if (Path.empty())
		{
			return {};
		}

		std::error_code Error;
		std::filesystem::path AbsolutePath = std::filesystem::absolute(Path, Error);
		if (Error)
		{
			Error.clear();
			AbsolutePath = Path;
		}

		std::filesystem::path CanonicalPath = std::filesystem::weakly_canonical(AbsolutePath, Error);
		if (!Error)
		{
			return CanonicalPath.lexically_normal();
		}

		return AbsolutePath.lexically_normal();
	}

	bool Preprocessor::HasPragmaOnce(const std::filesystem::path& Path) const
	{
		const std::filesystem::path Identity = NormalizeFileIdentity(Path);
		if (Identity.empty())
		{
			return false;
		}

		for (const auto& PragmaOnceFile : m_PragmaOnceFiles)
		{
			if (PragmaOnceFile == Identity)
			{
				return true;
			}

			std::error_code Error;
			if (std::filesystem::equivalent(PragmaOnceFile, Identity, Error) && !Error)
			{
				return true;
			}
		}

		return false;
	}

	void Preprocessor::RegisterPragmaOnce(const std::filesystem::path& Path)
	{
		const std::filesystem::path Identity = NormalizeFileIdentity(Path);
		if (!Identity.empty() && !HasPragmaOnce(Identity))
		{
			m_PragmaOnceFiles.Emplace(Identity);
		}
	}

	bool Preprocessor::GetToken(TextToken& Token)
	{
		try
		{
			bool TokenIsValid = false;
			while (true)
			{
				if (!TokenIsValid)
				{
					// Get Token from Current Tokenizer
					if (!GetTokenFromActiveTokenizer(Token))
					{
						return false;
					}
				}
				TokenIsValid = false;

				// Preprocess if Token is Comment
				if (PreprocessComment(Token, IsBlockEnabled()))
				{
					continue;
				}

				// Preprocess '#' Directives
				if (PreprocessDirective(Token, TokenIsValid))
				{
					continue;
				}

				// Skip Token if the Block is disabled
				if (!IsBlockEnabled())
				{
					continue;
				}

				// Resolve Definition
				if (PreprocessDefinition(Token, TokenIsValid))
				{
					continue;
				}

				break;
			}
		}
		catch (const TextTokenizerError& Err)
		{
			if (Err.File.empty())
			{
				TextTokenizerError NewErr = Err;
				NewErr.File = CurrentFile();
				throw NewErr;
			}
			throw Err;
		}

		return true;
	}

	const std::filesystem::path& Preprocessor::CurrentFile() const
	{
		for (size_t i = 0; i < m_SubTokenizers.Size(); i++)
		{
			size_t Index = (m_SubTokenizers.Size() - 1) - i;
			const SubTokenizer& It = m_SubTokenizers[Index];
			if (!It.Path.empty())
			{
				return It.Path;
			}
		}

		return m_Path;
	}

	const TextTokenizer& Preprocessor::CurrentFileTokenizer() const
	{
		for (size_t i = 0; i < m_SubTokenizers.Size(); i++)
		{
			size_t Index = (m_SubTokenizers.Size() - 1) - i;
			const SubTokenizer& It = m_SubTokenizers[Index];
			if (!It.Path.empty())
			{
				return *It.Tokenizer;
			}
		}

		return m_Tokenizer;
	}

	const TextTokenizer& Preprocessor::CurrentTokenizer() const
	{
		if (m_SubTokenizers.Size() > 0)
		{
			return *m_SubTokenizers[m_SubTokenizers.Size() - 1].Tokenizer;
		}

		return m_Tokenizer;
	}

	TextTokenizer& Preprocessor::GetActiveTokenizer()
	{
		if (m_SubTokenizers.Size() > 0)
		{
			return *m_SubTokenizers[m_SubTokenizers.Size() - 1].Tokenizer;
		}
		return m_Tokenizer;
	}

	void Preprocessor::PushSubTokenizer(const std::filesystem::path& Path, const String& DefinitionName, SharedPtr<String> Text,
		const Array<WChar>& Whitespace, const TextToken* Origin)
	{
		SharedPtr<TextTokenizerInput_String> Input = SharedPtr<TextTokenizerInput_String>(new TextTokenizerInput_String(*Text));
		SharedPtr<TextTokenizer> Tokenizer = SharedPtr<TextTokenizer>(new TextTokenizer(*Input, GetActiveTokenizer().Config));
		m_SubTokenizers.EmplaceRef(SubTokenizer{ Path, DefinitionName, Text, Input, Tokenizer, Whitespace,
			Origin != nullptr ? Origin->Pos : 0,
			Origin != nullptr ? Origin->Line : 0,
			Origin != nullptr ? Origin->LinePos : 0,
			Origin != nullptr });
		if (!Path.empty())
		{
			OnParseBegin();
		}
	}

	void Preprocessor::PopSubTokenizer()
	{
		if (m_SubTokenizers.Size() > 0)
		{
			if (!m_SubTokenizers[m_SubTokenizers.Size() - 1].Path.empty())
			{
				OnParseEnd();
			}
			m_SubTokenizers.RemoveAt(m_SubTokenizers.Size() - 1);
		}
	}

	bool Preprocessor::IsBlockEnabled() const
	{
		if (m_BlockStack.Size() > 0)
		{
			return m_BlockStack[m_BlockStack.Size() - 1] == EBlockState::Enabled;
		}
		return true;
	}

	void Preprocessor::PushBlockState(EBlockState State)
	{
		m_BlockStack.Emplace(State);
	}

	void Preprocessor::PopBlockState()
	{
		if (m_BlockStack.Size() > 0)
		{
			m_BlockStack.RemoveAt(m_BlockStack.Size() - 1);
		}
	}

	void Preprocessor::SetDefinition(const String& Name, const PreprocessorDefinition& Definition)
	{
		Definitions[Name] = Definition;

		if (!OnParsed_Define(Name, Definition))
		{
			throw TextTokenizerError(TEXT("OnParsed_Define returned false"), GetActiveTokenizer(), 0, CurrentFile());
		}
	}

	bool Preprocessor::GetTokenFromActiveTokenizer(TextToken& Token)
	{
		while (true)
		{
			TextTokenizer& Current = GetActiveTokenizer();
			if (Current.GetToken(Token))
			{
				if (m_SubTokenizers.Size() > 0)
				{
					SubTokenizer& Sub = m_SubTokenizers[m_SubTokenizers.Size() - 1];
					if (Sub.Whitespaces.Size() > 0)
					{
						Token.Whitespaces.Insert(0, Sub.Whitespaces);
						Sub.Whitespaces.Clear();
					}
					if (Sub.HasOrigin)
					{
						// Macro replacement text has no physical source position; report its invocation.
						Token.Pos = Sub.OriginPos;
						Token.Line = Sub.OriginLine;
						Token.LinePos = Sub.OriginLinePos;
					}
				}

				return true;
			}
			else if (m_SubTokenizers.Size() > 0)
			{
				PopSubTokenizer();
			}
			else
			{
				return false;
			}
		}
	}

	bool Preprocessor::PreprocessComment(TextToken& Token, bool ProcessComment)
	{
		if (Token.Type == ETextTokenType::Symbol)
		{
			bool IsMultiline = Token.Value_Text == L"/*";
			if (Token.Value_Text == L"//" || IsMultiline)
			{
				std::wstring Comment;

				WChar Peek = GetActiveTokenizer().PeekChar();
				while (true)
				{
					if (!IsMultiline && TextTokenizer::IsEndOfLine(Peek))
					{
						break;
					}

					WChar Char = GetActiveTokenizer().GetChar();
					Peek = GetActiveTokenizer().PeekChar();
					if (IsMultiline && Char == '*' && Peek == '/')
					{
						GetActiveTokenizer().GetChar();
						break;
					}

					Comment += Char;
				}

				if (Peek == '\0' && IsMultiline)
				{
					throw TextTokenizerError(TEXT("Unexpected end of file, expected Multiline Comment ending"), GetActiveTokenizer(), 0, CurrentFile());
				}

				if (ProcessComment && Comment.size() > 0)
				{
					if (!OnParsed_Comment(Comment, true))
					{
						throw TextTokenizerError(TEXT("OnParsed_Comment returned false"), GetActiveTokenizer(), 0, CurrentFile());
					}
				}

				return true;
			}
		}

		return false;
	}

	bool Preprocessor::GetTokenPreprocessedComments(TextToken& Token, bool ProcessComment)
	{
		while (true)
		{
			if (!GetTokenFromActiveTokenizer(Token))
			{
				return false;
			}

			if (PreprocessComment(Token, ProcessComment))
			{
				continue;
			}

			break;
		}

		return true;
	}

	bool Preprocessor::PreprocessDirective(TextToken& Token, bool& TokenIsValid)
	{
		if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == L"#")
		{
			Array<WChar> Whitespaces = Token.Whitespaces;

			if (!GetTokenPreprocessedComments(Token, true) || Token.Type != ETextTokenType::Identifier)
			{
				throw TextTokenizerError(TEXT("Expected Identifier after preprocessor symbol"), Token, CurrentFile());
			}

			bool BlockEnabled = IsBlockEnabled();
			if (BlockEnabled && Token.Value_Text == L"pragma")
			{
				return PreprocessDirective_Pragma(Token, TokenIsValid);
			}
			else if (BlockEnabled && Token.Value_Text == L"line")
			{
				return PreprocessDirective_Line(Token, TokenIsValid);
			}
			else if (BlockEnabled && Token.Value_Text == L"include")
			{
				return PreprocessDirective_Include(Token, TokenIsValid, Whitespaces);
			}
			else if (BlockEnabled && Token.Value_Text == L"error")
			{
				return PreprocessDirective_Error(Token, TokenIsValid);
			}
			else if (BlockEnabled && Token.Value_Text == L"define")
			{
				return PreprocessDirective_Define(Token, TokenIsValid);
			}
			else if (BlockEnabled && Token.Value_Text == L"undef")
			{
				return PreprocessDirective_Undefine(Token, TokenIsValid);
			}
			else if (Token.Value_Text == L"if" || Token.Value_Text == L"ifdef" || Token.Value_Text == L"ifndef" || Token.Value_Text == L"elif" || Token.Value_Text == L"elifdef" || Token.Value_Text == L"elifndef" || Token.Value_Text == L"else" || Token.Value_Text == L"endif")
			{
				return PreprocessDirective_Condition(Token, TokenIsValid);
			}
			else if (!BlockEnabled)
			{
				SkipDirectiveEnd(Token, false, TokenIsValid);
				return true;
			}
			else
			{
				throw TextTokenizerError(TEXT("Unknown Preprocessor Directive found"), Token, CurrentFile());
			}
		}

		return false;
	}

	void Preprocessor::SkipDirectiveEnd(TextToken& Token, bool ProcessComment, bool& TokenIsValid)
	{
		while (true)
		{
			if (!GetTokenFromActiveTokenizer(Token))
			{
				TokenIsValid = false;
				return;
			}

			if (IsTokenOnNewLine(Token))
			{
				TokenIsValid = true;
				break;
			}

			PreprocessComment(Token, ProcessComment);
		}
	}

	bool Preprocessor::PreprocessDirective_Pragma(TextToken& Token, bool& TokenIsValid)
	{
		if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token))
		{
			throw TextTokenizerError(TEXT("Expected Token after pragma directive"), GetActiveTokenizer(), 0, CurrentFile());
		}

		if (Token.Type != ETextTokenType::Identifier)
		{
			throw TextTokenizerError(TEXT("Expected Identifier after pragma directive"), GetActiveTokenizer(), 0, CurrentFile());
		}

		if (Token.Type == ETextTokenType::Identifier && Token.Value_Text == L"once")
		{
			RegisterPragmaOnce(CurrentFile());
		}

		SkipDirectiveEnd(Token, IsBlockEnabled(), TokenIsValid);

		return true;
	}

	bool Preprocessor::PreprocessDirective_Line(TextToken& Token, bool& TokenIsValid)
	{
		if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token))
		{
			throw TextTokenizerError(TEXT("Expected Token after line directive"), GetActiveTokenizer(), 0, CurrentFile());
		}

		if (Token.Type != ETextTokenType::Constant || Token.ConstantType != ETextTokenConstantType::Integral)
		{
			throw TextTokenizerError(TEXT("Expected Integer after line directive"), GetActiveTokenizer(), 0, CurrentFile());
		}

		size_t NewLine = static_cast<size_t>(Token.Value_Integral);
		//@TODO: Set Line

		bool IsFirst = true;
		while (true)
		{
			if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()))
			{
				TokenIsValid = false;
				break;
			}

			if (IsFirst)
			{
				IsFirst = false;
				if (!IsTokenOnNewLine(Token) && Token.Type == ETextTokenType::Constant && Token.ConstantType == ETextTokenConstantType::Text && !Token.IsRawText)
				{
					//@TODO: Set New File Name
				}
			}

			if (IsTokenOnNewLine(Token))
			{
				TokenIsValid = true;
				break;
			}
		}

		return true;
	}

	bool Preprocessor::PreprocessDirective_Include(TextToken& Token, bool& TokenIsValid, const Array<WChar>& Whitespaces)
	{
		if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token))
		{
			throw TextTokenizerError(TEXT("Expected Token after include directive"), GetActiveTokenizer(), 0, CurrentFile());
		}

		if (Token.Type == ETextTokenType::Identifier)
		{
			bool IncludeTokenIsValid = false;
			if (PreprocessDefinition(Token, IncludeTokenIsValid) && !GetTokenPreprocessedComments(Token, IsBlockEnabled()))
			{
				throw TextTokenizerError(TEXT("Expected Token after include directive"), GetActiveTokenizer(), 0, CurrentFile());
			}
		}

		bool IsSystemInclude = false;
		if (Token.Type == ETextTokenType::Constant && Token.ConstantType == ETextTokenConstantType::Text && !Token.IsRawText)
		{
			IsSystemInclude = false;
		}
		else if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == L"<")
		{
			IsSystemInclude = true;
			Token.Type = ETextTokenType::Constant;
			Token.ConstantType = ETextTokenConstantType::Text;
			Token.Value_Text.Clear();

			WChar Char = GetActiveTokenizer().GetChar();
			Token.RawText += Char;
			while (Char != '>')
			{
				Token.Value_Text += Char;
				Char = GetActiveTokenizer().GetChar();
				Token.RawText += Char;
			}

			if (Char != '>')
			{
				throw TextTokenizerError(TEXT("Unterminated include constant"), GetActiveTokenizer(), 0, CurrentFile());
			}
		}
		else
		{
			throw TextTokenizerError(TEXT("Expected either RawString Literal or '<' after include directive"), GetActiveTokenizer(), 0, CurrentFile());
		}

		std::filesystem::path Include = Token.Value_Text.Data();
		Include = Include.lexically_normal();

		SharedPtr<String> IncludeContent = SharedPtr<String>(new String());
		std::filesystem::path FinalIncludePath;

		if (!OnParsed_Include(CurrentFile(), Include, IsSystemInclude, FinalIncludePath, *IncludeContent))
		{
			throw TextTokenizerError(TEXT("OnParsed_Include returned false"), GetActiveTokenizer(), 0, CurrentFile());
		}

		if (HasPragmaOnce(FinalIncludePath))
		{
			SkipDirectiveEnd(Token, false, TokenIsValid);
			return true;
		}

		if (IncludeContent->Size() > 0)
		{
			PushSubTokenizer(FinalIncludePath, L"", IncludeContent, Whitespaces);

			TokenIsValid = false;
			return true;
		}

		SkipDirectiveEnd(Token, false, TokenIsValid);

		return true;
	}

	bool Preprocessor::PreprocessDirective_Error(TextToken& Token, bool& TokenIsValid)
	{
		WChar Char = GetActiveTokenizer().GetChar();
		std::wstring Error;
		while (!TextTokenizer::IsEndOfLine(Char))
		{
			Error += Char;
			Char = GetActiveTokenizer().GetChar();
		}

		if (!OnParsed_Error(Error))
		{
			throw TextTokenizerError(TEXT("OnParsed_Error returned false"), GetActiveTokenizer(), 0, CurrentFile());
		}

		throw TextTokenizerError(TEXT("Error Directive"), GetActiveTokenizer(), 0, CurrentFile());
	}

	bool Preprocessor::PreprocessDirective_Define(TextToken& Token, bool& TokenIsValid)
	{
		if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token) || Token.Type != ETextTokenType::Identifier)
		{
			throw TextTokenizerError(TEXT("Expected Definition Name after 'define'"), GetActiveTokenizer(), 0, CurrentFile());
		}

		std::wstring Name = Token.Value_Text;
		PreprocessorDefinition NewDefinition;

		if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()))
		{
			SetDefinition(Name, NewDefinition);
			TokenIsValid = false;
			return true;
		}

		if (IsTokenOnNewLine(Token))
		{
			SetDefinition(Name, NewDefinition);
			TokenIsValid = true;
			return true;
		}

		bool TokenIsNew = true;
		if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == L"(" && Token.Whitespaces.Size() == 0)
		{
			NewDefinition.SetRequireParameters(true);
			bool RequireSeparator = false;
			while (true)
			{
				if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token))
				{
					throw TextTokenizerError(TEXT("Expected definition parameters"), GetActiveTokenizer(), 0, CurrentFile());
				}

				if (Token.Type == ETextTokenType::Symbol)
				{
					if (Token.Value_Text == L")")
					{
						break;
					}
					else if (Token.Value_Text == L",")
					{
						RequireSeparator = false;
					}
					else if (Token.Value_Text == L".")
					{
						if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token))
						{
							throw TextTokenizerError(TEXT("Expected '.' in Definition Params"), GetActiveTokenizer(), 0, CurrentFile());
						}

						if (Token.Type != ETextTokenType::Symbol || Token.Value_Text != L".")
						{
							throw TextTokenizerError(TEXT("Expected '.' in Definition Params"), GetActiveTokenizer(), 0, CurrentFile());
						}

						if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token))
						{
							throw TextTokenizerError(TEXT("Expected '.' in Definition Params"), GetActiveTokenizer(), 0, CurrentFile());
						}

						if (Token.Type != ETextTokenType::Symbol || Token.Value_Text != L".")
						{
							throw TextTokenizerError(TEXT("Expected '.' in Definition Params"), GetActiveTokenizer(), 0, CurrentFile());
						}

						if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token))
						{
							throw TextTokenizerError(TEXT("Expected '.' in Definition Params"), GetActiveTokenizer(), 0, CurrentFile());
						}

						if (Token.Type != ETextTokenType::Symbol || Token.Value_Text != L")")
						{
							throw TextTokenizerError(TEXT("Expected ')' after variadic Param Definition Params"), GetActiveTokenizer(), 0, CurrentFile());
						}

						NewDefinition.AddParameter(L"...");
						break;
					}
					else
					{
						throw TextTokenizerError(TEXT("Unknown Symbol in Definition Parameters"), GetActiveTokenizer(), 0, CurrentFile());
					}
				}
				else if (Token.Type == ETextTokenType::Identifier)
				{
					if (RequireSeparator)
					{
						throw TextTokenizerError(TEXT("Expected Separator after Definition Param"), GetActiveTokenizer(), 0, CurrentFile());
					}

					NewDefinition.AddParameter(Token.Value_Text);
					RequireSeparator = true;
				}
			}

			TokenIsNew = false;
		}

		while (true)
		{
			if (!TokenIsNew)
			{
				if (!GetTokenPreprocessedComments(Token, false))
				{
					break;
				}
			}
			TokenIsNew = false;

			if (IsTokenOnNewLine(Token))
			{
				TokenIsValid = true;
				break;
			}

			NewDefinition.AddToken(Token);
		}

		SetDefinition(Name, NewDefinition);

		return true;
	}

	bool Preprocessor::PreprocessDirective_Undefine(TextToken& Token, bool& TokenIsValid)
	{
		if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()) || IsTokenOnNewLine(Token))
		{
			throw TextTokenizerError(TEXT("Expected Token after undef directive"), GetActiveTokenizer(), 0, CurrentFile());
		}

		if (Token.Type != ETextTokenType::Identifier)
		{
			throw TextTokenizerError(TEXT("Expected Identifier after undef directive"), GetActiveTokenizer(), 0, CurrentFile());
		}

		if (!OnParsed_Undefine(Token.Value_Text))
		{
			throw TextTokenizerError(TEXT("OnParsed_Undefine returned false"), GetActiveTokenizer(), 0, CurrentFile());
		}

		Definitions.erase(Token.Value_Text);

		SkipDirectiveEnd(Token, IsBlockEnabled(), TokenIsValid);

		return true;
	}

	bool Preprocessor::PreprocessDirective_Condition(TextToken& Token, bool& TokenIsValid)
	{
		bool ParentBlockEnabled = IsBlockEnabled();
		bool PrevBlockEnabled = true;
		for (size_t i = 0; i + 1 < m_BlockStack.Size(); i++)
		{
			if (m_BlockStack[i] != EBlockState::Enabled)
			{
				PrevBlockEnabled = false;
				break;
			}
		}

		if (Token.Value_Text == L"ifdef" || Token.Value_Text == L"ifndef" || Token.Value_Text == L"elifdef" || Token.Value_Text == L"elifndef")
		{
			bool IsRequired = Token.Value_Text == L"ifdef" || Token.Value_Text == L"elifdef";
			bool IsElse = Token.Value_Text == L"elifdef" || Token.Value_Text == L"elifndef";
			if (IsElse && m_BlockStack.Size() == 0)
			{
				throw TextTokenizerError(TEXT("Found elifdef/elifndef without if directive"), GetActiveTokenizer(), 0, CurrentFile());
			}

			bool EvaluateCondition = IsElse
				? PrevBlockEnabled && m_BlockStack[m_BlockStack.Size() - 1] == EBlockState::Disabled
				: ParentBlockEnabled;
			if (!GetTokenPreprocessedComments(Token, EvaluateCondition) || Token.Type != ETextTokenType::Identifier || IsTokenOnNewLine(Token))
			{
				throw TextTokenizerError(TEXT("Expected Identifier after ifdef/ifndef/elifdef/elifndef"), GetActiveTokenizer(), 0, CurrentFile());
			}

			std::wstring DefinitionName = Token.Value_Text;
			bool Enable = EvaluateCondition && Definitions.find(DefinitionName) != Definitions.end();
			if (!IsRequired)
			{
				Enable = EvaluateCondition && !Enable;
			}

			if (IsElse)
			{
				EBlockState& Current = m_BlockStack[m_BlockStack.Size() - 1];
				Current = EvaluateCondition
					? (Enable ? EBlockState::Enabled : EBlockState::Disabled)
					: EBlockState::Completed;
			}
			else
			{
				m_BlockStack.Emplace(Enable ? EBlockState::Enabled : EBlockState::Disabled);
			}
		}
		else if (Token.Value_Text == L"if" || Token.Value_Text == L"elif")
		{
			bool IsElse = Token.Value_Text == L"elif";
			if (IsElse)
			{
				if (m_BlockStack.Size() == 0)
				{
					throw TextTokenizerError(TEXT("Found elif without if directive"), GetActiveTokenizer(), 0, CurrentFile());
				}
			}

			bool EvaluateCondition = IsElse
				? PrevBlockEnabled && m_BlockStack[m_BlockStack.Size() - 1] == EBlockState::Disabled
				: ParentBlockEnabled;
			bool Enable = false;
			if (EvaluateCondition)
			{
				Enable = PreprocessCondition(Token, true, TokenIsValid);
			}
			else
			{
				SkipDirectiveEnd(Token, false, TokenIsValid);
			}

			if (IsElse)
			{
				EBlockState& Current = m_BlockStack[m_BlockStack.Size() - 1];
				Current = EvaluateCondition
					? (Enable ? EBlockState::Enabled : EBlockState::Disabled)
					: EBlockState::Completed;
			}
			else
			{
				m_BlockStack.Emplace(Enable ? EBlockState::Enabled : EBlockState::Disabled);
			}
		}
		else if (Token.Value_Text == L"else")
		{
			if (m_BlockStack.Size() == 0)
			{
				throw TextTokenizerError(TEXT("Found else without if directive"), GetActiveTokenizer(), 0, CurrentFile());
			}

			m_BlockStack[m_BlockStack.Size() - 1] = (PrevBlockEnabled && m_BlockStack[m_BlockStack.Size() - 1] == EBlockState::Disabled) ? EBlockState::Enabled : EBlockState::Completed;

			SkipDirectiveEnd(Token, PrevBlockEnabled, TokenIsValid);
		}
		else if (Token.Value_Text == L"endif")
		{
			if (m_BlockStack.Size() == 0)
			{
				throw TextTokenizerError(TEXT("Found endif without if directive"), GetActiveTokenizer(), 0, CurrentFile());
			}
			m_BlockStack.RemoveAt(m_BlockStack.Size() - 1);

			SkipDirectiveEnd(Token, PrevBlockEnabled, TokenIsValid);
		}
		else
		{
			return false;
		}

		return true;
	}

	bool Preprocessor::PreprocessDefinition(TextToken& Token, bool& TokenIsValid)
	{
		if (Token.Type == ETextTokenType::Identifier)
		{
			for (auto& Definition : Definitions)
			{
				if (Token.Value_Text == Definition.first)
				{
					const TextToken ExpansionOrigin = Token;
					bool IsAlreadyExpanding = false;
					for (auto& Sub : m_SubTokenizers)
					{
						if (Sub.DefinitionName == Definition.first)
						{
							IsAlreadyExpanding = true;
							break;
						}
					}

					if (IsAlreadyExpanding)
						return false;

					Array<WChar> Whitespaces = Token.Whitespaces;
					String Name = Definition.first;

					Array<String> Parameters;
					if (Definition.second.RequireParameters())
					{
						if (!GetTokenPreprocessedComments(Token, IsBlockEnabled()))
						{
							return false;
						}

						if (Token.Type != ETextTokenType::Symbol || Token.Value_Text != L"(")
						{
							SharedPtr<String> Text = SharedPtr<String>(new String());
							if (Token.Whitespaces.Size() > 0)
							{
								*Text += std::wstring(Token.Whitespaces.Data(), Token.Whitespaces.Size());
							}
							*Text += Token.RawText;
							PushSubTokenizer("", "", Text, {});
							return false;
						}

						std::stack<char> Scopes;
						Scopes.push('(');

						std::wstring CurrentParam;
						while (true)
						{
							if (!TokenIsValid)
							{
								if (!GetTokenPreprocessedComments(Token, false))
								{
									throw TextTokenizerError(TEXT("File ends inside a Definition Argument List"), GetActiveTokenizer(), 0, CurrentFile());
								}
							}
							TokenIsValid = false;

							if (Token.Type == ETextTokenType::Symbol)
							{
								if (Token.Value_Text == L"(")
								{
									Scopes.push('(');
								}
								else if (Token.Value_Text == L")")
								{
									if (Scopes.top() != '(')
									{
										throw TextTokenizerError(TEXT("Expected '(' as closing bracket"), GetActiveTokenizer(), 0, CurrentFile());
									}
									Scopes.pop();
								}
								else if (Token.Value_Text == L"[")
								{
									Scopes.push('[');
								}
								else if (Token.Value_Text == L"]")
								{
									if (Scopes.top() != '[')
									{
										throw TextTokenizerError(TEXT("Expected ']' as closing bracket"), GetActiveTokenizer(), 0, CurrentFile());
									}
									Scopes.pop();
								}
								else if (Token.Value_Text == L"{")
								{
									Scopes.push('{');
								}
								else if (Token.Value_Text == L"}")
								{
									if (Scopes.top() != '{')
									{
										throw TextTokenizerError(TEXT("Expected '}' as closing bracket"), GetActiveTokenizer(), 0, CurrentFile());
									}
									Scopes.pop();
								}
								else if (Token.Value_Text == L",")
								{
									if (Scopes.size() == 1)
									{
										Parameters.Emplace(CurrentParam);
										CurrentParam.clear();
										continue;
									}
								}
							}

							if (Scopes.empty())
							{
								break;
							}

							if (Token.Type == ETextTokenType::Identifier && m_SubTokenizers.Size() > 0 && m_SubTokenizers[m_SubTokenizers.Size() - 1].DefinitionName.Size() > 0)
							{
								if (PreprocessDefinition(Token, TokenIsValid))
								{
									continue;
								}
							}

							if (Token.Whitespaces.Size() > 0)
							{
								CurrentParam += ' ';
							}
							CurrentParam += Token.RawText;
						}

						if (!CurrentParam.empty())
						{
							Parameters.Emplace(CurrentParam);
						}
					}

					SharedPtr<String> Text = SharedPtr<String>(new String(Definition.second.Resolve(Parameters)));
					PushSubTokenizer("", Name, Text, Whitespaces, &ExpansionOrigin);
					TokenIsValid = false;
					return true;
				}
			}

			if (Token.Value_Text == L"__LINE__")
			{
				Token.Type = ETextTokenType::Constant;
				Token.ConstantType = ETextTokenConstantType::Integral;
				Token.Value_Integral = CurrentFileTokenizer().GetLine();
				Token.RawText = std::to_wstring(Token.Value_Integral);
				Token.Value_Text = Token.RawText;
				TokenIsValid = true;
				return true;
			}
			else if (Token.Value_Text == L"__FILE__")
			{
				Token.Type = ETextTokenType::Constant;
				Token.ConstantType = ETextTokenConstantType::Text;
				Token.Value_Text = CurrentFile().wstring();
				Token.RawText = L"\"" + Token.Value_Text + L"\"";
				TokenIsValid = true;
				return true;
			}
		}

		return false;
	}

	bool Preprocessor::PreprocessCondition(TextToken& Token, bool ProcessComment, bool& TokenIsValid)
	{
		Calculation Calc;

		while (true)
		{
			if (!TokenIsValid)
			{
				if (!GetTokenFromActiveTokenizer(Token))
				{
					throw TextTokenizerError(TEXT("Unexpected end of if directive"), GetActiveTokenizer(), 0, CurrentFile());
				}
			}
			TokenIsValid = false;

			if (IsTokenOnNewLine(Token))
			{
				TokenIsValid = true;
				break;
			}

			if (PreprocessComment(Token, ProcessComment))
			{
				continue;
			}

			if (Token.Type == ETextTokenType::Identifier)
			{
				if (Token.Value_Text == L"defined")
				{
					if (!GetTokenPreprocessedComments(Token, ProcessComment) || IsTokenOnNewLine(Token))
					{
						throw TextTokenizerError(TEXT("Expected Token after 'defined' if condition"), GetActiveTokenizer(), 0, CurrentFile());
					}

					bool InBraces = false;
					if (Token.Type == ETextTokenType::Symbol && Token.Value_Text == L"(")
					{
						if (!GetTokenPreprocessedComments(Token, ProcessComment) || IsTokenOnNewLine(Token))
						{
							throw TextTokenizerError(TEXT("Expected Identifier Token after 'defined' if condition"), GetActiveTokenizer(), 0, CurrentFile());
						}
						InBraces = true;
					}

					String DefinitionName = Token.Value_Text.Size() > 0 ? Token.Value_Text : Token.RawText;
					bool Found = Definitions.find(DefinitionName) != Definitions.end();
					Calc.Value((int64_t)Found);

					if (InBraces)
					{
						if (!GetTokenPreprocessedComments(Token, ProcessComment) || IsTokenOnNewLine(Token) || Token.Type != ETextTokenType::Symbol || Token.Value_Text != L")")
						{
							throw TextTokenizerError(TEXT("Expected defined close bracket ')'"), GetActiveTokenizer(), 0, CurrentFile());
						}
					}

					continue;
				}

				if (PreprocessDefinition(Token, TokenIsValid))
				{
					continue;
				}

				if (Token.Type == ETextTokenType::Identifier)
				{
					Calc.Value(0);
					continue;
				}
			}

			try
			{
				Calc.AddToken(Token);
			}
			catch (TextTokenizerError& E)
			{
				throw TextTokenizerError(E.Message, E.Pos, E.Length, E.Line, E.LinePos, CurrentFile());
			}
		}

		return Calc.Solve();
	}
}
