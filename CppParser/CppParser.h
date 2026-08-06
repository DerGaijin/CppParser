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
		static bool IsFunctionTailSpecifier(const String& Value);
		static bool IsBuiltinType(const String& Value);

		bool HasToken(size_t Offset = 0) const;
		const TextToken& PeekToken(size_t Offset = 0) const;
		TextToken TakeToken();
		bool IsToken(const String& Value, size_t Offset = 0) const;
		bool ConsumeToken(const String& Value);
		void ExpectToken(const String& Value);
		void ThrowError(const String& Message) const;
		bool IsClassDeclaration() const;
		size_t FindDeclaratorName(size_t Begin, size_t End) const;
		size_t FindDeclaratorTypeEnd(size_t Begin, size_t NameAt) const;
		String BuildDeclarator(size_t Begin, size_t End, size_t NameAt) const;

		String TokensToText(size_t Begin, size_t End) const;
		size_t FindMatching(size_t Open, const String& OpenValue, const String& CloseValue) const;
		size_t FindTopLevel(size_t Begin, size_t End, const String& Value) const;
		Array<std::pair<size_t, size_t>> SplitTopLevel(size_t Begin, size_t End, const String& Value) const;

		void Parse_Declaration();
		void Parse_Name(size_t Begin, size_t End, ParsedName& Name, bool AllowInline = false);
		void Parse_Type(size_t Begin, size_t End, ParsedType& Type);
		void Parse_Attributes(Array<ParsedAttribute>& Attributes);
		void Parse_Attributes(size_t& At, size_t End, Array<ParsedAttribute>& Attributes);
		void Parse_Namespace(bool IsInline);
		void Parse_Class(EClassType Type, bool IsFriend = false);
		void Parse_Enum();
		void Parse_Access(EAccessSpecifier Access);
		void Parse_Using(bool IsTypedef);
		void Parse_Template();
		void Parse_Concept();
		void Parse_StaticAssert();
		void Parse_Linkage();
		void Parse_General();
		void Parse_ClosedClassDeclarators(const Scope& ClosedScope);
		void Parse_Parameters(size_t Begin, size_t End, Array<ParsedFunctionParameter>& Parameters);
		void Parse_FunctionTail(size_t Begin, size_t End, ParsedFunctionBase& Function, ParsedType* TrailingType, bool& IsTrailingType);
		void Parse_Variable(size_t Begin, size_t End, size_t NameAt, const ParsedType& BaseType, EParsedVariableFlags Flags);

		String CurrentClassName() const;


	private:
		Array<TextToken> m_Tokens;
		size_t m_TokenAt = 0;
		Array<Scope> m_Scopes;
		Array<ParsedAttribute> m_PendingAttributes;
		ParsedType m_PendingDeclaredType;
		EParsedVariableFlags m_PendingVariableFlags = EParsedVariableFlags::None;
	};
}
