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
		virtual bool OnParsed_Namespace(const ParsedNamespace& Namespace) { return true; }
		virtual bool OnParsed_Class(const ParsedClass& Class) { return true; }
		virtual bool OnParsed_AccessSpecifier(EAccessSpecifier Access) { return true; }
		virtual bool OnParsed_Variable(const ParsedVariable& Variable) { return true; }
		virtual bool OnParsed_Function(const ParsedFunction& Function) { return true; }
		virtual bool OnParsed_Enum(const ParsedEnum& Enum) { return true; }
		virtual bool OnParsed_EnumValue(const ParsedEnumValue& Value) { return true; }
		virtual bool OnParsed_ScopeEnd() { return true; }
		virtual bool OnParsed_TemplateDeclaration(const ParsedTemplateDeclaration& Template) { return true; }
		virtual bool OnParsed_UsingTypedef(const ParsedUsingTypedef& UsingTypedef) { return true; }
	};
}
