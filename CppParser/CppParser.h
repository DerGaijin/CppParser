#pragma once
#include "Core.h"
#include "Preprocessor.h"
#include "ParsedData.h"


namespace CE
{
	class CE_API CppParser : public Preprocessor
	{
	public:
		CppParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer);

		virtual void Parse();

	protected:
		virtual bool OnParsed_Namespace(const Array<ParsedNamespace>& Namespaces) { return true; }
		virtual bool OnParsed_NamespaceAlias(const ParsedNamespaceAlias& Alias) { return true; }
		virtual bool OnParsed_ScopeEnd() { return true; }
		virtual bool OnParsed_Class(const ParsedClass& Class) { return true; }
		virtual bool OnParsed_Access(EAccessSpecifier Access) { return true; }
		virtual bool OnParsed_Enum(const ParsedEnum& Enum) { return true; }
		virtual bool OnParsed_EnumValue(const ParsedEnumValue& Value) { return true; }
		virtual bool OnParsed_Variable(const ParsedVariable& Value) { return true; }
		virtual bool OnParsed_Constructor(const ParsedConstructor& Constructor) { return true; }
		virtual bool OnParsed_Destructor(const ParsedDestructor& Destructor) { return true; }
		virtual bool OnParsed_Function(const ParsedFunction& Function) { return true; }
		virtual bool OnParsed_Operator(const ParsedOperator& Operator) { return true; }
		virtual bool OnParsed_Using(const ParsedUsing& Using) { return true; }
		virtual bool OnParsed_Template(const ParsedTemplate& Template) { return true; }
		virtual bool OnParsed_Concept(const ParsedConcept& Concept) { return true; }
		virtual bool OnParsed_StaticAssert(const ParsedStaticAssert& Assert) { return true; }

	};
}
