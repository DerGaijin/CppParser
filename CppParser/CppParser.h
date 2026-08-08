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
		virtual bool OnParsed_Namespace(const ParsedNamespace& Namespace) { return true; }
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
		virtual bool OnParsed_Linkage(const ParsedLinkage& Linkage) { return true; }

	private:
		enum class EScopeType : uint8
		{
			Namespace,
			Class,
			Linkage,
		};

		struct Scope
		{
			EScopeType Type = EScopeType::Namespace;
			String Name;
			EParsedElaboratedType ElaboratedType = EParsedElaboratedType::None;
			ParsedType DeclaredType;
			EParsedVariableFlags VariableFlags = EParsedVariableFlags::None;
		};

		static void AddFlag(EParsedTypeFlags& Flags, EParsedTypeFlags Flag);
		static void AddFlag(EParsedVariableFlags& Flags, EParsedVariableFlags Flag);
		static void AddFlag(EParsedFunctionFlags& Flags, EParsedFunctionFlags Flag);
		static bool IsTypeQualifier(const String& Value);
		static bool IsDeclarationSpecifier(const String& Value);
		static bool IsBuiltinType(const String& Value);
		static void AppendTokenText(String& Text, const TextToken& Token);
		static void AddNameSegment(ParsedName& Name, const String& Value);

		void Advance(TextToken& Token, bool& HasToken);
		bool ConsumeEllipsis(TextToken& Token, bool& HasToken);
		void Expect(const String& Value, TextToken& Token, bool& HasToken);
		void ThrowError(const String& Message, const TextToken& Token, bool HasToken) const;
		void SkipBalanced(const String& Open, const String& Close, TextToken& Token, bool& HasToken, String* Text = nullptr);
		void ReadExpressionUntil(String& Text, const String& EndA, const String& EndB, TextToken& Token, bool& HasToken);

		void ParseDeclaration(TextToken& Token, bool& HasToken);
		void ParseAttributes(Array<ParsedAttribute>& Attributes, TextToken& Token, bool& HasToken);
		void ParseNamespace(bool IsInline, TextToken& Token, bool& HasToken);
		void ParseClass(EClassType Type, bool IsFriend, TextToken& Token, bool& HasToken);
		void ParseEnum(TextToken& Token, bool& HasToken);
		void ParseUsing(bool IsTypedef, TextToken& Token, bool& HasToken);
		void ParseTemplate(TextToken& Token, bool& HasToken);
		void ParseConcept(TextToken& Token, bool& HasToken);
		void ParseStaticAssert(TextToken& Token, bool& HasToken);
		void ParseLinkage(TextToken& Token, bool& HasToken);
		void ParseGeneral(TextToken& Token, bool& HasToken, ParsedType Type = {}, EParsedVariableFlags VariableFlags = EParsedVariableFlags::None);
		void ParseParameters(Array<ParsedFunctionParameter>& Parameters, TextToken& Token, bool& HasToken);
		void ParseFunction(ParsedType ReturnType, const String& Name, bool IsDestructor, bool IsOperator, const String& OperatorSymbol,
			EParsedFunctionFlags Flags, Array<ParsedAttribute>& Attributes, const String& ExplicitExpression, TextToken& Token, bool& HasToken);
		void ParseClosedClassDeclarators(const Scope& ClosedScope, TextToken& Token, bool& HasToken);

		String CurrentClassName() const;

	private:
		Array<Scope> m_Scopes;
		Array<ParsedAttribute> m_PendingAttributes;
		ParsedType m_PendingDeclaredType;
		EParsedVariableFlags m_PendingVariableFlags = EParsedVariableFlags::None;
	};
}
