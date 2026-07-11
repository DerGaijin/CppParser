#pragma once
#include "Core.h"

namespace CE
{
	enum class EAccessSpecifier : uint8
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

	struct ParsedType;

	struct ParsedTemplateArgument
	{
		enum class EKind : uint8
		{
			Type,
			Expression,
		};

		EKind Kind = EKind::Type;
		SharedPtr<ParsedType> Type;
		ParsedExpression Expression;
	};

	struct ParsedNameSegment
	{
		String Name;
		Array<ParsedTemplateArgument> TemplateArguments;
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
			LValueReference,
			RValueReference,
		};

		EKind Kind = EKind::Pointer;
		bool IsConst = false;
		bool IsVolatile = false;
		bool IsMutable = false;
	};

	struct ParsedType
	{
		ParsedName Name;
		Array<ParsedAttribute> Attributes;
		Array<ParsedIndirection> Indirections;
		bool IsConst = false;
		bool IsVolatile = false;
		bool IsUnsigned = false;
		bool IsSigned = false;
		bool IsMutable = false;
		bool IsElaboratedType = false;
		String ElaboratedTypeKeyword;
	};

	struct ParsedNamespace
	{
		ParsedName Name;
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
		EAccessSpecifier Access = EAccessSpecifier::Private;
		bool IsVirtual = false;
	};

	struct ParsedClass
	{
		EClassType Type = EClassType::Class;
		ParsedName Name;
		Array<ParsedBaseClass> BaseClasses;
		Array<ParsedAttribute> Attributes;
		bool IsFinal = false;
		bool HasDefinition = false;
		bool IsForward = false;
		bool IsFriend = false;
	};

	struct ParsedVariable
	{
		ParsedType Type;
		String Name;
		ParsedExpression Initializer;
		ParsedExpression BitfieldSize;
		Array<ParsedAttribute> Attributes;
		bool HasInitializer = false;
		bool IsConstexpr = false;
		bool IsConsteval = false;
		bool IsStatic = false;
		bool IsThreadLocal = false;
		bool IsMutable = false;
		bool IsExtern = false;
		bool IsBitfield = false;
	};

	struct ParsedConcept
	{
		String Name;
		ParsedExpression Constraint;
		Array<ParsedAttribute> Attributes;
	};

	struct ParsedDecltype
	{
		ParsedExpression Expression;
		String Name;
		ParsedExpression Initializer;
		Array<ParsedAttribute> Attributes;
		bool HasInitializer = false;
		bool IsConstexpr = false;
		bool IsConsteval = false;
		bool IsStatic = false;
		bool IsThreadLocal = false;
		bool IsMutable = false;
		bool IsExtern = false;
	};

	struct ParsedParameter
	{
		ParsedType Type;
		String Name;
		ParsedExpression DefaultValue;
		bool HasDefaultValue = false;
		bool IsExplicitObject = false;
	};

	enum class EFunctionRefQualifier : uint8
	{
		None,
		LValue,
		RValue,
	};

	enum class EFunctionFlag : uint32_t
	{
		None = 0,
		Const = 1 << 0,
		Volatile = 1 << 1,
		Virtual = 1 << 2,
		Override = 1 << 3,
		Final = 1 << 4,
		Pure = 1 << 5,
		Static = 1 << 6,
		Inline = 1 << 7,
		Constexpr = 1 << 8,
		Consteval = 1 << 9,
		NoExcept = 1 << 10,
		Deleted = 1 << 11,
		Defaulted = 1 << 12,
		HasDefinition = 1 << 13,
		Friend = 1 << 14,
		Variadic = 1 << 15,
		Explicit = 1 << 16,
	};

	inline EFunctionFlag operator|(EFunctionFlag Left, EFunctionFlag Right)
	{
		return static_cast<EFunctionFlag>(static_cast<uint32_t>(Left) | static_cast<uint32_t>(Right));
	}

	inline EFunctionFlag& operator|=(EFunctionFlag& Left, EFunctionFlag Right)
	{
		Left = Left | Right;
		return Left;
	}

	inline bool HasFunctionFlag(EFunctionFlag Flags, EFunctionFlag Flag)
	{
		return (static_cast<uint32_t>(Flags) & static_cast<uint32_t>(Flag)) != 0;
	}

	struct ParsedFunctionBase
	{
		ParsedName Name;
		Array<ParsedParameter> Parameters;
		Array<ParsedAttribute> Attributes;
		ParsedExpression NoExceptExpression;
		ParsedExpression DeletedMessage;
		ParsedExpression RequiresClause;
		EFunctionRefQualifier RefQualifier = EFunctionRefQualifier::None;
		EFunctionFlag Flags = EFunctionFlag::None;
	};

	struct ParsedFunction : ParsedFunctionBase
	{
		ParsedType ReturnType;
		ParsedType TrailingReturnType;
	};

	struct ParsedConstructor : ParsedFunctionBase
	{
	};

	struct ParsedDestructor : ParsedFunctionBase
	{
	};

	struct ParsedEnum
	{
		ParsedName Name;
		ParsedType UnderlyingType;
		Array<ParsedAttribute> Attributes;
		bool IsScoped = false;
		bool IsOpaque = false;
	};

	struct ParsedEnumValue
	{
		String Name;
		ParsedExpression Value;
		Array<ParsedAttribute> Attributes;
		bool HasValue = false;
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

	struct ParsedTemplateDeclaration
	{
		Array<ParsedTemplateParameter> Parameters;
		ParsedExpression RequiresClause;
	};

	struct ParsedUsing
	{
		enum class EKind : uint8
		{
			Typedef,
			AliasDeclaration,
			UsingDeclaration,
			UsingDirective,
			UsingEnum,
		};

		EKind Kind = EKind::Typedef;
		ParsedType Type;
		ParsedName Name;
		ParsedName Target;
		Array<ParsedAttribute> Attributes;
	};
}
