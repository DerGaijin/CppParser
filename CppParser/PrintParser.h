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
		virtual bool OnParsed_Concept(const ParsedConcept& Concept);
		virtual bool OnParsed_StaticAssert(const ParsedStaticAssert& Assert);

	private:
		bool HasFlag(EParsedVariableFlags Flags, EParsedVariableFlags Flag) const;
		bool HasFlag(EParsedFunctionFlags Flags, EParsedFunctionFlags Flag) const;
		bool HasFlag(EParsedTypeFlags Flags, EParsedTypeFlags Flag) const;
		bool PrintParsedFunction(const String& CallbackName, const ParsedFunctionBase& Function, const ParsedType* ReturnType = nullptr, const String* OperatorSymbol = nullptr, bool IsTrailingReturnType = false);
		void PrintFunctionText(const String& Name);
		void PrintIndentText(int32 Shift = 0);
		String FormatAttributes(const Array<ParsedAttribute>& Attributes) const;
		String FormatName(const ParsedName& Name) const;
		String FormatType(const ParsedType& Type) const;


	public:
		bool PrintFunction = false;


	private:
		std::wostream& m_Output;

		struct ScopeInfo
		{
			bool RequiresSemicolon = false;
			ParsedName Name;
		};
		Array<ScopeInfo> m_Scopes;
		ScopeInfo m_LastClosedScope;
	};
}
