#pragma once
#include "Core.h"


namespace CE
{
	enum class EAccessSpecifier
	{
		Public,
		Protected,
		Private,
	};

	enum class EClassType : uint8
	{
		Class,
		Struct,
		Union,
	};

	struct ParsedExpression
	{
		String Text;
	};

	struct ParsedTemplateArgument;

	struct ParsedNameSegment
	{
		String Name;
		Array<ParsedTemplateArgument> TemplateArguments;
		bool bIsInline = false;
	};

	struct ParsedName
	{
		Array<ParsedNameSegment> Segments;
	};

	struct ParsedAttribute
	{
		enum class EKind : uint8
		{
			Standard,  // [[nodiscard]], [[namespace::attr(args)]]
			Declspec,  // __declspec(dllexport)
			Gnu,       // __attribute__((...))
			Alignas,   // alignas(...)
			Other,
		};
		EKind Kind = EKind::Standard;
		ParsedName Name;
		Array<ParsedExpression> Arguments;
	};

	struct ParsedIndirection
	{
		enum class EKind : uint8
		{
			Pointer,
			LReference,
			RReference,
		};

		EKind Kind = EKind::Pointer;
		bool IsConst = false;
		bool IsVolatile = false;
		bool IsMutable = false;
	};

	enum class EParsedTypeFlags : uint8
	{
		None = 0,
		IsConst = 1 << 0,
		IsVolatile = 1 << 1,
		IsMutable = 1 << 2,
		IsUnsigned = 1 << 3,
		IsSigned = 1 << 4,
	};

	struct ParsedType
	{
		ParsedName Name;
		Array<ParsedAttribute> Attributes;
		Array<ParsedIndirection> Indirections;
		Array<ParsedExpression> ArrayExtents;
		EParsedTypeFlags Flags = EParsedTypeFlags::None;
	};

	struct ParsedTemplateArgument
	{
		enum class EKind : uint8
		{
			Type,
			Expression,
		};

		EKind Kind = EKind::Type;
		ParsedType Type;
		ParsedExpression Expression;
	};

	struct ParsedNamespace
	{
		String Name;
		Array<ParsedAttribute> Attributes;
		bool IsInline = false;
	};

	struct ParsedNamespaceAlias
	{
		ParsedName Name;
		ParsedName Target;
		Array<ParsedAttribute> Attributes;
	};

	struct ParsedBaseClass
	{
		ParsedType Type;
		EAccessSpecifier AccessSpecifier = EAccessSpecifier::Private;
		bool IsVirtual = false;
	};

	struct ParsedClass
	{
		EClassType Type = EClassType::Class;
		ParsedName Name;
		Array<ParsedAttribute> Attributes;
		Array<ParsedTemplateArgument> Specialization;
		Array<ParsedBaseClass> BaseClasses;
		bool IsForward = false;
		bool IsFinal = false;
		bool IsFriend = false;
		bool IsAnonymous = false;
		bool HasBody = false;
	};

	struct ParsedEnum
	{
		ParsedName Name;
		ParsedType UnderlyingType;
		Array<ParsedAttribute> Attributes;
		bool IsScoped = false;
		bool IsStruct = false;
		bool IsForward = false;
		bool IsAnonymous = false;
	};

	struct ParsedEnumValue
	{
		String Name;
		ParsedExpression Value;
		Array<ParsedAttribute> Attributes;
		bool HasValue = false;
	};

	enum class EParsedVariableFlags : uint8
	{
		None = 0,
		HasInitializer = 1 << 0,
		IsConstexpr = 1 << 1,
		IsConsteval = 1 << 2,
		IsStatic = 1 << 3,
		IsThreadLocal = 1 << 4,
		IsMutable = 1 << 5,
		IsExtern = 1 << 6,
		IsBitfield = 1 << 7,
	};

	struct ParsedVariable
	{
		ParsedType Type;
		ParsedName Name;
		ParsedExpression Initializer;
		Array<ParsedAttribute> Attributes;
		EParsedVariableFlags Flags = EParsedVariableFlags::None;
	};

	struct ParsedFunctionParameter
	{
		ParsedType Type;
		ParsedName Name;
		ParsedExpression DefaultValue;
		Array<ParsedAttribute> Attributes;
		bool HasDefaultValue = false;
		bool IsVariadic = false;
	};

	enum class EParsedFunctionFlags : uint16
	{
		None = 0,
		HasBody = 1 << 0,
		HasRequires = 1 << 1,
		IsStatic = 1 << 2,
		IsInline = 1 << 3,
		IsVirtual = 1 << 4,
		IsExplicit = 1 << 5,
		IsConstexpr = 1 << 6,
		IsConsteval = 1 << 7,
		IsPureVirtual = 1 << 8,
		IsDefaulted = 1 << 9,
		IsDeleted = 1 << 10,
		IsNoexcept = 1 << 11,
	};

	struct ParsedFunctionBase
	{
		ParsedName Name;
		Array<ParsedFunctionParameter> Parameters;
		Array<ParsedAttribute> Attributes;
		Array<String> Specifiers;
		Array<String> Qualifiers;
		ParsedExpression RequiresClause;
		ParsedExpression Body;
		ParsedExpression NoexceptExpression;
		EParsedFunctionFlags Flags = EParsedFunctionFlags::None;
	};

	struct ParsedFunction : public ParsedFunctionBase
	{
		ParsedType ReturnType;
		bool IsTrailingType = false;
	};

	struct ParsedConstructor : public ParsedFunctionBase {};

	struct ParsedDestructor : public ParsedFunctionBase {};

	struct ParsedOperator : public ParsedFunction
	{
		String Symbol;
	};

	struct ParsedUsing
	{
		enum class EKind : uint8
		{
			Typedef,
			AliasDeclaration,
			UsingDeclaration,
			UsingDirective,
		};

		EKind Kind = EKind::Typedef;
		ParsedType Type;
		ParsedName Name;
		ParsedName Target;
		Array<ParsedAttribute> Attributes;
	};

	struct ParsedTemplateParameter
	{
		enum class EKind : uint8
		{
			Type,
			NonType,
			TemplateTemplate,
		};
		EKind Kind = EKind::Type;
		String Name;
		ParsedName Constraint;
		ParsedType Type;
		ParsedType DefaultType;
		ParsedExpression DefaultExpression;
		ParsedExpression RequiresClause;
		bool HasDefault = false;
		bool IsVariadic = false;
	};

	struct ParsedTemplate
	{
		Array<ParsedTemplateParameter> Parameters;
		ParsedExpression RequiresClause;
		bool HasRequires = false;
	};

	struct ParsedConcept
	{
		ParsedName Name;
		ParsedExpression Constraint;
		Array<ParsedAttribute> Attributes;
	};

	struct ParsedStaticAssert
	{
		ParsedExpression Condition;
		ParsedExpression Message;
		bool HasMessage = false;
	};
}
