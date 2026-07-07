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
		bool IsInline = false;
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

	struct ParsedParameter
	{
		ParsedType Type;
		String Name;
		ParsedExpression DefaultValue;
		bool HasDefaultValue = false;
		bool IsExplicitObject = false;
	};

	struct ParsedFunction
	{
		enum class ERefQual : uint8
		{
			None,
			LValue,
			RValue,
		};

		ParsedType ReturnType;
		ParsedName Name;
		Array<ParsedParameter> Parameters;
		Array<ParsedAttribute> Attributes;
		ParsedExpression NoExceptExpression;
		ParsedExpression DeletedMessage;
		ParsedExpression RequiresClause;
		ParsedType TrailingReturnType;
		ERefQual RefQualifier = ERefQual::None;
		bool IsConst = false;
		bool IsVolatile = false;
		bool IsVirtual = false;
		bool IsOverride = false;
		bool IsFinal = false;
		bool IsPure = false;
		bool IsStatic = false;
		bool IsInline = false;
		bool IsConstexpr = false;
		bool IsConsteval = false;
		bool IsNoExcept = false;
		bool IsDeleted = false;
		bool IsDefaulted = false;
		bool HasDefinition = false;
		bool IsFriend = false;
		bool IsVariadic = false;
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

	struct ParsedTemplate
	{
		enum class EKind : uint8
		{
			ExplicitInstantiation,
			ExplicitSpecialization,
		};

		EKind Kind = EKind::ExplicitInstantiation;
		EClassType ClassType = EClassType::Class;
		ParsedName Name;
		bool IsExtern = false;
	};

	struct ParsedUsingTypedef
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
