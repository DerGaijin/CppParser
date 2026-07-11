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
		virtual bool OnParsed_Namespace(const Array<ParsedNamespace>& Namespaces) override;
		virtual bool OnParsed_NamespaceAlias(const ParsedNamespaceAlias& NamespaceAlias) override;
		virtual bool OnParsed_Class(const ParsedClass& Class) override;
		virtual bool OnParsed_AccessSpecifier(EAccessSpecifier Access) override;
		virtual bool OnParsed_Variable(const ParsedVariable& Variable) override;
		virtual bool OnParsed_Concept(const ParsedConcept& Concept) override;
		virtual bool OnParsed_Decltype(const ParsedDecltype& Decltype) override;
		virtual bool OnParsed_Function(const ParsedFunction& Function) override;
		virtual bool OnParsed_Constructor(const ParsedConstructor& Constructor) override;
		virtual bool OnParsed_Destructor(const ParsedDestructor& Destructor) override;
		virtual bool OnParsed_Enum(const ParsedEnum& Enum) override;
		virtual bool OnParsed_EnumValue(const ParsedEnumValue& Value) override;
		virtual bool OnParsed_ScopeEnd() override;
		virtual bool OnParsed_TemplateDeclaration(const ParsedTemplateDeclaration& Template) override;
		virtual bool OnParsed_UsingTypedef(const ParsedUsingTypedef& UsingTypedef) override;
		virtual bool OnParsed_Using(const ParsedUsing& Using) override;


	private:
		enum class EScopeKind : uint8
		{
			Namespace,
			Type,
		};


	private:
		static const WChar* ToString(EAccessSpecifier Access);
		static const WChar* ToString(EClassType Type);

		std::wstring FormatName(const ParsedName& Name) const;
		std::wstring FormatType(const ParsedType& Type) const;
		std::wstring FormatAttributes(const Array<ParsedAttribute>& Attributes) const;
		std::wstring FormatParameter(const ParsedParameter& Parameter) const;
		std::wstring FormatTemplateParameter(const ParsedTemplateParameter& Parameter) const;

		void PrintFunctionPrefix(const WChar* FunctionName);
		void PrintIndent(const WChar* FunctionName = nullptr);


	private:
		std::wostream& m_Output;
		size_t m_Indent = 0;
		Array<EScopeKind> m_ScopeStack;
	};
}
