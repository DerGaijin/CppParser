#pragma once
#include "Core.h"
#include "Preprocessor.h"
#include "ParseData.h"


namespace CE
{
	class CE_API CppParser : public Preprocessor
	{
	public:
		CppParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer);

		virtual void Parse();



	protected:
		virtual bool OnParsed_Namespace(const Array<ParsedNamespace>& Namespaces) { return true; }
		virtual bool OnParsed_NamespaceAlias(const ParsedNamespaceAlias& NamespaceAlias) { return true; }
		virtual bool OnParsed_Class(const ParsedClass& Class) { return true; }
		virtual bool OnParsed_AccessSpecifier(EAccessSpecifier Access) { return true; }
		virtual bool OnParsed_Variable(const ParsedVariable& Variable) { return true; }
		virtual bool OnParsed_Concept(const ParsedConcept& Concept) { return true; }
		virtual bool OnParsed_Decltype(const ParsedDecltype& Decltype) { return true; }
		virtual bool OnParsed_Function(const ParsedFunction& Function) { return true; }
		virtual bool OnParsed_Constructor(const ParsedConstructor& Constructor) { return true; }
		virtual bool OnParsed_Destructor(const ParsedDestructor& Destructor) { return true; }
		virtual bool OnParsed_Enum(const ParsedEnum& Enum) { return true; }
		virtual bool OnParsed_EnumValue(const ParsedEnumValue& Value) { return true; }
		virtual bool OnParsed_ScopeEnd() { return true; }
		virtual bool OnParsed_TemplateDeclaration(const ParsedTemplateDeclaration& Template) { return true; }
		virtual bool OnParsed_UsingTypedef(const ParsedUsingTypedef& UsingTypedef) { return true; }
		virtual bool OnParsed_Using(const ParsedUsing& Using) { return true; }


	private:
		bool GetParserToken(TextToken& Token);

		void PushParserToken(const TextToken& Token);

		bool ParseNamespace(bool IsInline);

		bool ParseClass(EClassType Type, Array<ParsedAttribute> LeadingAttributes = {});

		bool ParseEnum(Array<ParsedAttribute> LeadingAttributes = {});

		bool ParseUsing(Array<ParsedAttribute> LeadingAttributes = {});

		bool ParseTypedef(Array<ParsedAttribute> LeadingAttributes = {});

		bool ParseVariableDeclaration(const TextToken& FirstToken, Array<ParsedAttribute> LeadingAttributes = {});

		bool ParseFunctionDeclaration(const Array<TextToken>& Declarator, const Array<TextToken>& BaseTypeTokens, Array<ParsedAttribute> LeadingAttributes, bool HasDefinition);

		bool ParseFriendClassDeclaration(const Array<TextToken>& Tokens, Array<ParsedAttribute> LeadingAttributes);

		ParsedParameter ParseFunctionParameter(const Array<TextToken>& Tokens);

		bool ParseTemplateDeclaration();

		ParsedTemplateParameter ParseTemplateParameter(const Array<TextToken>& Tokens);

		bool ParseConceptDeclaration(Array<ParsedAttribute> LeadingAttributes = {});

		Array<ParsedTemplateArgument> ParseTemplateArguments();

		Array<ParsedAttribute> ParseAttributes();

		ParsedName ParseQualifiedName(const TextToken& FirstToken, TextToken& NextToken);

		ParsedType ParseTypeUntil(TextToken& NextToken);

		ParsedExpression ParseExpressionUntilEnumValueEnd(TextToken& NextToken);

		bool ParseEnumValue(const TextToken& FirstToken, TextToken& NextToken);

		void SkipBalancedBlock(const WChar* Open, const WChar* Close);

		void SkipFunctionDefinitionBody();

		static void AppendTokenText(String& Text, const TextToken& Token);

		bool ReadScopeSeparator();

		Array<TextToken> ReadDeclarationTokensUntilSemicolon();

		Array<TextToken> ReadVariableDeclarationTokens(const TextToken& FirstToken, bool& HasSemicolon);

		void PushParserTokensAfterFirst(const Array<TextToken>& Tokens);

		static Array<Array<TextToken>> SplitTopLevelCommas(const Array<TextToken>& Tokens);

		static String TokensToText(const Array<TextToken>& Tokens, size_t Begin, size_t End);

		static ParsedName MakeTextName(const String& Text);

		static ParsedType MakeTextType(const String& Text);

		static size_t FindTypedefDeclaratorStart(const Array<TextToken>& Tokens);

		static size_t FindVariableNameIndex(const Array<TextToken>& Tokens, size_t Begin, size_t End);

		static size_t FindVariableDeclaratorTypeBegin(const Array<TextToken>& Tokens, size_t NameIndex);

		static size_t FindVariableDeclaratorEnd(const Array<TextToken>& Tokens);

		static ParsedType ParseVariableType(const Array<TextToken>& Tokens);

		static ParsedExpression MakeTextExpression(const Array<TextToken>& Tokens, size_t Begin, size_t End);

		static size_t FindFunctionParameterListOpen(const Array<TextToken>& Tokens, size_t NameIndex);

		static size_t FindMatchingSymbol(const Array<TextToken>& Tokens, size_t OpenIndex, const WChar* Open, const WChar* Close);

		static size_t FindTopLevelToken(const Array<TextToken>& Tokens, size_t Begin, const WChar* Text);

		static size_t FindTopLevelRequiresClauseEnd(const Array<TextToken>& Tokens, size_t Begin);

		static size_t FindDecltypeSpecifierEnd(const Array<TextToken>& Tokens, size_t Index);

		static bool IsRequiresExpressionBodyStart(const Array<TextToken>& Tokens);

		static bool IsFunctionSpecifier(const TextToken& Token);

		static bool IsDestructorToken(const TextToken& Token);

		static bool IsVariableDeclarationBlocked(const Array<TextToken>& Tokens);

		static bool IsVariableStartToken(const TextToken& Token);

		static bool IsFunctionDeclarator(const Array<TextToken>& Tokens, size_t NameIndex);

		bool ParseTrailingClassDeclarator(const ParsedClass& Class, const Array<TextToken>& TypePrefixTokens);

		static TextToken MakeIdentifierToken(const String& Text);

		static String NameToText(const ParsedName& Name);

		static bool IsTopLevelClassBody(const Array<TextToken>& Tokens);

		static bool HasTopLevelClassBody(const Array<TextToken>& Tokens);

		static bool IsCurrentScopeTokenDepth(size_t BraceDepth, const Array<size_t>& NamespaceDepths, const Array<size_t>& ParsedScopeDepths);

		static bool IsAttributeStart(const TextToken& Token);

		static bool IsIdentifier(const TextToken& Token, const WChar* Text);

		static bool IsSymbol(const TextToken& Token, const WChar* Text);

		static ParsedName MakeName(const String& Name);

		static ParsedAttribute MakeStandardAttribute(const std::wstring& Text);


	private:
		Array<TextToken> m_TokenBuffer;
		Array<size_t> m_NamespaceDepths;
		Array<size_t> m_ParsedScopeDepths;
		Array<ParsedClass> m_ParsedScopeClasses;
		Array<Array<TextToken>> m_ParsedScopeTypePrefixTokens;
		Array<TextToken> m_PendingClassTypePrefixTokens;
		size_t m_BraceDepth = 0;
	};
}
