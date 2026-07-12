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
		virtual bool OnParsed_Namespace(const Array<ParsedNamespace>& Namespaces);
		virtual bool OnParsed_NamespaceAlias(const ParsedNamespaceAlias& Alias);
		virtual bool OnParsed_ScopeEnd();
		virtual bool OnParsed_Class(const ParsedClass& Class);
		virtual bool OnParsed_Access(EAccessSpecifier Access);
		virtual bool OnParsed_Enum(const ParsedEnum& Enum);
		virtual bool OnParsed_EnumValue(const ParsedEnumValue& Value);
		virtual bool OnParsed_Variable(const ParsedVariable& Value);
		virtual bool OnParsed_Constructor(const ParsedConstructor& Constructor);
		virtual bool OnParsed_Destructor(const ParsedDestructor& Destructor);
		virtual bool OnParsed_Function(const ParsedFunction& Function);
		virtual bool OnParsed_Operator(const ParsedOperator& Operator);
		virtual bool OnParsed_Using(const ParsedUsing& Using);
		virtual bool OnParsed_Template(const ParsedTemplate& Template);
		virtual bool OnParsed_Concept(const ParsedTemplate& Concept);
		virtual bool OnParsed_StaticAssert(const ParsedStaticAssert& Assert);


	private:
		static bool HasFlag(EParsedTypeFlags Flags, EParsedTypeFlags Flag);
		static bool HasFlag(EParsedVariableFlags Flags, EParsedVariableFlags Flag);
		static bool HasFlag(EParsedFunctionFlags Flags, EParsedFunctionFlags Flag);
		static void AppendSeparated(String& Output, const String& Value, const WChar* Separator = L", ");
		static String FormatExpression(const ParsedExpression& Expression);
		static String FormatName(const ParsedName& Name);
		static String FormatAttribute(const ParsedAttribute& Attribute);
		static String FormatAttributes(const Array<ParsedAttribute>& Attributes);
		static String FormatType(const ParsedType& Type, const String& Declarator = L"");
		static String FormatTemplateArgument(const ParsedTemplateArgument& Argument);
		static String FormatParameter(const ParsedFunctionParameter& Parameter);
		static String FormatTemplateParameter(const ParsedTemplateParameter& Parameter);
		static String FormatFunctionSuffix(const ParsedFunctionBase& Function);
		static void AppendFunctionPrefix(String& Result, const ParsedFunctionBase& Function);
		static String FormatFunction(const ParsedFunctionBase& Function, const String& Declaration);

		void PrintFunctionText(const String& Name);
		void PrintIndentText(int32 Shift = 0);


	public:
		bool PrintFunction = true;


	private:
		std::wostream& m_Output;

		struct ScopeInfo 
		{
			bool RequiresSemicolon = false;
			ParsedType Type;
		};
		Array<ScopeInfo> m_Scopes;
		ScopeInfo m_LastClosedScope;
		size_t m_AnonymousTypeCount = 0;
	};
}
