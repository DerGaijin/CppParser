#include "PrintParser.h"

namespace CE
{
	PrintParser::PrintParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer, std::wostream& Output) : CppParser(Path, Tokenizer), m_Output(Output)
	{

	}

	void PrintParser::OnParseBegin()
	{
		std::wstring FileName = CurrentFile().wstring();
		std::wstring Border(FileName.size(), '=');
		m_Output << L"\n// ";
		PrintFunctionPrefix(L"OnParseBegin");
		m_Output << Border << L"\n// ";
		PrintFunctionPrefix(L"OnParseBegin");
		m_Output << FileName << L"\n// ";
		PrintFunctionPrefix(L"OnParseBegin");
		m_Output << Border << L"\n\n";
	}

	bool PrintParser::OnParsed_Namespace(const Array<ParsedNamespace>& Namespaces)
	{
		PrintIndent(L"OnParsed_Namespace");
		if (Namespaces.Size() > 0)
		{
			const ParsedNamespace& FirstNamespace = Namespaces[0];
			if (FirstNamespace.IsInline)
			{
				m_Output << L"inline ";
			}
			const std::wstring Attributes = FormatAttributes(FirstNamespace.Attributes);
			if (!Attributes.empty())
			{
				m_Output << Attributes << L" ";
			}
		}
		m_Output << L"namespace ";
		for (size_t Index = 0; Index < Namespaces.Size(); ++Index)
		{
			if (Index > 0)
			{
				m_Output << L"::";
				if (Namespaces[Index].IsInline)
				{
					m_Output << L"inline ";
				}
			}
			m_Output << FormatName(Namespaces[Index].Name);
		}
		m_Output << L"\n";
		PrintIndent(L"OnParsed_Namespace");
		m_Output << L"{\n";
		++m_Indent;
		m_ScopeStack.Add(EScopeKind::Namespace);
		return true;
	}

	bool PrintParser::OnParsed_NamespaceAlias(const ParsedNamespaceAlias& NamespaceAlias)
	{
		PrintIndent(L"OnParsed_NamespaceAlias");
		const std::wstring Attributes = FormatAttributes(NamespaceAlias.Attributes);
		if (!Attributes.empty())
		{
			m_Output << Attributes << L" ";
		}
		m_Output << L"namespace " << FormatName(NamespaceAlias.Name) << L" = " << FormatName(NamespaceAlias.Target) << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_Class(const ParsedClass& Class)
	{
		PrintIndent(L"OnParsed_Class");
		if (Class.IsFriend)
		{
			m_Output << L"friend ";
		}
		m_Output << ToString(Class.Type);
		const std::wstring Attributes = FormatAttributes(Class.Attributes);
		if (!Attributes.empty())
		{
			m_Output << L" " << Attributes;
		}
		m_Output << L" " << FormatName(Class.Name);
		if (Class.IsFinal)
		{
			m_Output << L" final";
		}
		if (Class.BaseClasses.Size() > 0)
		{
			m_Output << L" : ";
			for (size_t Index = 0; Index < Class.BaseClasses.Size(); ++Index)
			{
				const ParsedBaseClass& BaseClass = Class.BaseClasses[Index];
				if (Index > 0)
				{
					m_Output << L", ";
				}
				if (BaseClass.IsVirtual)
				{
					m_Output << L"virtual ";
				}
				m_Output << ToString(BaseClass.Access) << L" " << FormatType(BaseClass.Type);
			}
		}

		if (Class.HasDefinition)
		{
			m_Output << L"\n";
			PrintIndent(L"OnParsed_Class");
			m_Output << L"{\n";
			++m_Indent;
			m_ScopeStack.Add(EScopeKind::Type);
		}
		else
		{
			m_Output << L";\n";
		}

		return true;
	}

	bool PrintParser::OnParsed_AccessSpecifier(EAccessSpecifier Access)
	{
		const size_t SavedIndent = m_Indent;
		if (m_Indent > 0)
		{
			--m_Indent;
		}
		PrintIndent(L"OnParsed_AccessSpecifier");
		m_Output << ToString(Access) << L":\n";
		m_Indent = SavedIndent;
		return true;
	}

	bool PrintParser::OnParsed_Variable(const ParsedVariable& Variable)
	{
		PrintIndent(L"OnParsed_Variable");
		const std::wstring Attributes = FormatAttributes(Variable.Attributes);
		if (!Attributes.empty())
		{
			m_Output << Attributes << L" ";
		}
		if (Variable.IsConstexpr)
		{
			m_Output << L"constexpr ";
		}
		if (Variable.IsConsteval)
		{
			m_Output << L"consteval ";
		}
		if (Variable.IsStatic)
		{
			m_Output << L"static ";
		}
		if (Variable.IsThreadLocal)
		{
			m_Output << L"thread_local ";
		}
		if (Variable.IsExtern)
		{
			m_Output << L"extern ";
		}
		if (Variable.IsMutable)
		{
			m_Output << L"mutable ";
		}
		m_Output << FormatType(Variable.Type) << L" " << static_cast<std::wstring>(Variable.Name);
		if (Variable.IsBitfield)
		{
			m_Output << L" : " << static_cast<std::wstring>(Variable.BitfieldSize.Text);
		}
		if (Variable.HasInitializer)
		{
			m_Output << L" = " << static_cast<std::wstring>(Variable.Initializer.Text);
		}
		m_Output << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_Concept(const ParsedConcept& Concept)
	{
		PrintIndent(L"OnParsed_Concept");
		const std::wstring Attributes = FormatAttributes(Concept.Attributes);
		if (!Attributes.empty())
		{
			m_Output << Attributes << L" ";
		}
		m_Output << L"concept " << static_cast<std::wstring>(Concept.Name) << L" = " << static_cast<std::wstring>(Concept.Constraint.Text) << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_Decltype(const ParsedDecltype& Decltype)
	{
		PrintIndent(L"OnParsed_Decltype");
		const std::wstring Attributes = FormatAttributes(Decltype.Attributes);
		if (!Attributes.empty())
		{
			m_Output << Attributes << L" ";
		}
		if (Decltype.IsConstexpr)
		{
			m_Output << L"constexpr ";
		}
		if (Decltype.IsConsteval)
		{
			m_Output << L"consteval ";
		}
		if (Decltype.IsStatic)
		{
			m_Output << L"static ";
		}
		if (Decltype.IsThreadLocal)
		{
			m_Output << L"thread_local ";
		}
		if (Decltype.IsExtern)
		{
			m_Output << L"extern ";
		}
		if (Decltype.IsMutable)
		{
			m_Output << L"mutable ";
		}
		m_Output << L"decltype(" << static_cast<std::wstring>(Decltype.Expression.Text) << L") " << static_cast<std::wstring>(Decltype.Name);
		if (Decltype.HasInitializer)
		{
			m_Output << L" = " << static_cast<std::wstring>(Decltype.Initializer.Text);
		}
		m_Output << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_Function(const ParsedFunction& Function)
	{
		PrintIndent(L"OnParsed_Function");
		PrintFunctionLike(Function, EFunctionKind::Function, &Function.ReturnType, &Function.TrailingReturnType);
		return true;
	}

	bool PrintParser::OnParsed_Constructor(const ParsedConstructor& Constructor)
	{
		PrintIndent(L"OnParsed_Constructor");
		PrintFunctionLike(Constructor, EFunctionKind::Constructor);
		return true;
	}

	bool PrintParser::OnParsed_Destructor(const ParsedDestructor& Destructor)
	{
		PrintIndent(L"OnParsed_Destructor");
		PrintFunctionLike(Destructor, EFunctionKind::Destructor);
		return true;
	}

	void PrintParser::PrintFunctionLike(const ParsedFunctionBase& Function, EFunctionKind Kind, const ParsedType* ReturnType, const ParsedType* TrailingReturnType)
	{
		const std::wstring Attributes = FormatAttributes(Function.Attributes);
		if (!Attributes.empty())
		{
			m_Output << Attributes << L" ";
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Friend))
		{
			m_Output << L"friend ";
		}
		if (Kind == EFunctionKind::Constructor && HasFunctionFlag(Function.Flags, EFunctionFlag::Explicit))
		{
			m_Output << L"explicit ";
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Virtual))
		{
			m_Output << L"virtual ";
		}
		if (Kind == EFunctionKind::Function && HasFunctionFlag(Function.Flags, EFunctionFlag::Static))
		{
			m_Output << L"static ";
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Inline))
		{
			m_Output << L"inline ";
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Constexpr))
		{
			m_Output << L"constexpr ";
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Consteval))
		{
			m_Output << L"consteval ";
		}

		if (ReturnType != nullptr)
		{
			const std::wstring ReturnTypeText = FormatType(*ReturnType);
			if (!ReturnTypeText.empty())
			{
				m_Output << ReturnTypeText << L" ";
			}
		}

		if (Kind == EFunctionKind::Destructor)
		{
			m_Output << L"~";
		}
		m_Output << FormatName(Function.Name) << L"(";
		for (size_t Index = 0; Index < Function.Parameters.Size(); ++Index)
		{
			if (Index > 0)
			{
				m_Output << L", ";
			}
			m_Output << FormatParameter(Function.Parameters[Index]);
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Variadic))
		{
			if (Function.Parameters.Size() > 0)
			{
				m_Output << L", ";
			}
			m_Output << L"...";
		}
		m_Output << L")";
		if (Kind == EFunctionKind::Function && HasFunctionFlag(Function.Flags, EFunctionFlag::Const))
		{
			m_Output << L" const";
		}
		if (Kind == EFunctionKind::Function && HasFunctionFlag(Function.Flags, EFunctionFlag::Volatile))
		{
			m_Output << L" volatile";
		}
		if (Function.RefQualifier == EFunctionRefQualifier::LValue)
		{
			m_Output << L" &";
		}
		else if (Function.RefQualifier == EFunctionRefQualifier::RValue)
		{
			m_Output << L" &&";
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::NoExcept))
		{
			m_Output << L" noexcept";
			if (Function.NoExceptExpression.Text.Size() > 0)
			{
				m_Output << L"(" << static_cast<std::wstring>(Function.NoExceptExpression.Text) << L")";
			}
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Override))
		{
			m_Output << L" override";
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Final))
		{
			m_Output << L" final";
		}
		if (TrailingReturnType != nullptr)
		{
			const std::wstring TrailingReturnTypeText = FormatType(*TrailingReturnType);
			if (!TrailingReturnTypeText.empty())
			{
				m_Output << L" -> " << TrailingReturnTypeText;
			}
		}
		if (Function.RequiresClause.Text.Size() > 0)
		{
			m_Output << L" requires " << static_cast<std::wstring>(Function.RequiresClause.Text);
		}
		if (HasFunctionFlag(Function.Flags, EFunctionFlag::Pure))
		{
			m_Output << L" = 0";
		}
		else if (HasFunctionFlag(Function.Flags, EFunctionFlag::Deleted))
		{
			m_Output << L" = delete";
			if (Function.DeletedMessage.Text.Size() > 0)
			{
				m_Output << L"(" << static_cast<std::wstring>(Function.DeletedMessage.Text) << L")";
			}
		}
		else if (HasFunctionFlag(Function.Flags, EFunctionFlag::Defaulted))
		{
			m_Output << L" = default";
		}

		m_Output << (HasFunctionFlag(Function.Flags, EFunctionFlag::HasDefinition) ? L" { }\n" : L";\n");
	}

	bool PrintParser::OnParsed_Enum(const ParsedEnum& Enum)
	{
		PrintIndent(L"OnParsed_Enum");
		const std::wstring Attributes = FormatAttributes(Enum.Attributes);
		if (!Attributes.empty())
		{
			m_Output << Attributes << L" ";
		}
		m_Output << L"enum";
		if (Enum.IsScoped)
		{
			m_Output << L" class";
		}
		const std::wstring Name = FormatName(Enum.Name);
		if (!Name.empty())
		{
			m_Output << L" " << Name;
		}
		const std::wstring UnderlyingType = FormatType(Enum.UnderlyingType);
		if (!UnderlyingType.empty())
		{
			m_Output << L" : " << UnderlyingType;
		}
		if (Enum.IsOpaque)
		{
			m_Output << L";\n";
		}
		else
		{
			m_Output << L"\n";
			PrintIndent(L"OnParsed_Enum");
			m_Output << L"{\n";
			++m_Indent;
			m_ScopeStack.Add(EScopeKind::Type);
		}
		return true;
	}

	bool PrintParser::OnParsed_EnumValue(const ParsedEnumValue& Value)
	{
		PrintIndent(L"OnParsed_EnumValue");
		const std::wstring Attributes = FormatAttributes(Value.Attributes);
		if (!Attributes.empty())
		{
			m_Output << Attributes << L" ";
		}
		m_Output << static_cast<std::wstring>(Value.Name);
		if (Value.HasValue)
		{
			m_Output << L" = " << static_cast<std::wstring>(Value.Value.Text);
		}
		m_Output << L",\n";
		return true;
	}

	bool PrintParser::OnParsed_ScopeEnd()
	{
		EScopeKind ScopeKind = EScopeKind::Type;
		if (m_ScopeStack.Size() > 0)
		{
			ScopeKind = m_ScopeStack[m_ScopeStack.Size() - 1];
			m_ScopeStack.RemoveAt(m_ScopeStack.Size() - 1);
		}
		if (m_Indent > 0)
		{
			--m_Indent;
		}
		PrintIndent(L"OnParsed_ScopeEnd");
		m_Output << (ScopeKind == EScopeKind::Namespace ? L"}\n" : L"};\n");
		return true;
	}

	bool PrintParser::OnParsed_TemplateDeclaration(const ParsedTemplateDeclaration& Template)
	{
		PrintIndent(L"OnParsed_TemplateDeclaration");
		m_Output << L"template<";
		for (size_t Index = 0; Index < Template.Parameters.Size(); ++Index)
		{
			if (Index > 0)
			{
				m_Output << L", ";
			}
			m_Output << FormatTemplateParameter(Template.Parameters[Index]);
		}
		m_Output << L">";
		if (Template.RequiresClause.Text.Size() > 0)
		{
			m_Output << L" requires " << static_cast<std::wstring>(Template.RequiresClause.Text);
		}
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Using(const ParsedUsing& Using)
	{
		PrintIndent(L"OnParsed_Using");
		const std::wstring Attributes = FormatAttributes(Using.Attributes);
		if (!Attributes.empty())
		{
			m_Output << Attributes << L" ";
		}

		switch (Using.Kind)
		{
		case ParsedUsing::EKind::Typedef:
			m_Output << L"typedef " << FormatType(Using.Type) << L" " << FormatName(Using.Name) << L";\n";
			break;
		case ParsedUsing::EKind::AliasDeclaration:
			m_Output << L"using " << FormatName(Using.Name) << L" = " << FormatType(Using.Type) << L";\n";
			break;
		case ParsedUsing::EKind::UsingDeclaration:
			m_Output << L"using " << FormatName(Using.Target) << L";\n";
			break;
		case ParsedUsing::EKind::UsingDirective:
			m_Output << L"using namespace " << FormatName(Using.Target) << L";\n";
			break;
		case ParsedUsing::EKind::UsingEnum:
			m_Output << L"using enum " << FormatName(Using.Target) << L";\n";
			break;
		}

		return true;
	}

	const WChar* PrintParser::ToString(EAccessSpecifier Access)
	{
		switch (Access)
		{
		case EAccessSpecifier::Public:
			return L"public";
		case EAccessSpecifier::Protected:
			return L"protected";
		case EAccessSpecifier::Private:
			return L"private";
		default:
			return L"private";
		}
	}

	const WChar* PrintParser::ToString(EClassType Type)
	{
		switch (Type)
		{
		case EClassType::Class:
			return L"class";
		case EClassType::Struct:
			return L"struct";
		case EClassType::Union:
			return L"union";
		default:
			return L"class";
		}
	}

	std::wstring PrintParser::FormatName(const ParsedName& Name) const
	{
		std::wstring Result;
		for (size_t Index = 0; Index < Name.Segments.Size(); ++Index)
		{
			if (Index > 0)
			{
				Result += L"::";
			}

			const ParsedNameSegment& Segment = Name.Segments[Index];
			Result += static_cast<std::wstring>(Segment.Name);
			if (Segment.TemplateArguments.Size() > 0)
			{
				Result += L"<";
				for (size_t ArgIndex = 0; ArgIndex < Segment.TemplateArguments.Size(); ++ArgIndex)
				{
					if (ArgIndex > 0)
					{
						Result += L", ";
					}

					const ParsedTemplateArgument& Argument = Segment.TemplateArguments[ArgIndex];
					if (Argument.Kind == ParsedTemplateArgument::EKind::Expression)
					{
						Result += static_cast<std::wstring>(Argument.Expression.Text);
					}
					else if (Argument.Type)
					{
						Result += FormatType(*Argument.Type);
					}
				}
				Result += L">";
			}
		}

		return Result;
	}

	std::wstring PrintParser::FormatType(const ParsedType& Type) const
	{
		std::wstring Result;
		const std::wstring Attributes = FormatAttributes(Type.Attributes);
		if (!Attributes.empty())
		{
			Result += Attributes;
		}
		if (Type.IsConst)
		{
			if (!Result.empty()) { Result += L" "; }
			Result += L"const";
		}
		if (Type.IsVolatile)
		{
			if (!Result.empty()) { Result += L" "; }
			Result += L"volatile";
		}
		if (Type.IsMutable)
		{
			if (!Result.empty()) { Result += L" "; }
			Result += L"mutable";
		}
		if (Type.IsSigned)
		{
			if (!Result.empty()) { Result += L" "; }
			Result += L"signed";
		}
		if (Type.IsUnsigned)
		{
			if (!Result.empty()) { Result += L" "; }
			Result += L"unsigned";
		}
		if (Type.IsElaboratedType && Type.ElaboratedTypeKeyword.Size() > 0)
		{
			if (!Result.empty()) { Result += L" "; }
			Result += static_cast<std::wstring>(Type.ElaboratedTypeKeyword);
		}

		const std::wstring Name = FormatName(Type.Name);
		if (!Name.empty())
		{
			if (!Result.empty()) { Result += L" "; }
			Result += Name;
		}

		for (const ParsedIndirection& Indirection : Type.Indirections)
		{
			switch (Indirection.Kind)
			{
			case ParsedIndirection::EKind::Pointer:
				Result += L"*";
				break;
			case ParsedIndirection::EKind::LValueReference:
				Result += L"&";
				break;
			case ParsedIndirection::EKind::RValueReference:
				Result += L"&&";
				break;
			}
			if (Indirection.IsConst)
			{
				Result += L" const";
			}
			if (Indirection.IsVolatile)
			{
				Result += L" volatile";
			}
			if (Indirection.IsMutable)
			{
				Result += L" mutable";
			}
		}

		return Result;
	}

	std::wstring PrintParser::FormatAttributes(const Array<ParsedAttribute>& Attributes) const
	{
		std::wstring Result;
		for (const ParsedAttribute& Attribute : Attributes)
		{
			std::wstring Arguments;
			for (size_t Index = 0; Index < Attribute.Arguments.Size(); ++Index)
			{
				if (Index > 0)
				{
					Arguments += L", ";
				}
				Arguments += static_cast<std::wstring>(Attribute.Arguments[Index].Text);
			}

			const std::wstring Name = FormatName(Attribute.Name);
			std::wstring Text;
			switch (Attribute.Kind)
			{
			case ParsedAttribute::EKind::Standard:
				Text = L"[[" + Name + (Arguments.empty() ? L"" : L"(" + Arguments + L")") + L"]]";
				break;
			case ParsedAttribute::EKind::Declspec:
				Text = L"__declspec(" + Name + (Arguments.empty() ? L"" : L"(" + Arguments + L")") + L")";
				break;
			case ParsedAttribute::EKind::Gnu:
				Text = L"__attribute__((" + Name + (Arguments.empty() ? L"" : L"(" + Arguments + L")") + L"))";
				break;
			case ParsedAttribute::EKind::Alignas:
				Text = L"alignas(" + (Arguments.empty() ? Name : Arguments) + L")";
				break;
			case ParsedAttribute::EKind::Other:
			default:
				Text = Name + (Arguments.empty() ? L"" : L"(" + Arguments + L")");
				break;
			}

			if (!Result.empty())
			{
				Result += L" ";
			}
			Result += Text;
		}

		return Result;
	}

	std::wstring PrintParser::FormatParameter(const ParsedParameter& Parameter) const
	{
		std::wstring Result;
		if (Parameter.IsExplicitObject)
		{
			Result += L"this";
		}

		const std::wstring Type = FormatType(Parameter.Type);
		if (!Type.empty())
		{
			if (!Result.empty())
			{
				Result += L" ";
			}
			Result += Type;
		}

		if (Parameter.Name.Size() > 0)
		{
			if (!Result.empty())
			{
				Result += L" ";
			}
			Result += static_cast<std::wstring>(Parameter.Name);
		}

		if (Parameter.HasDefaultValue)
		{
			Result += L" = " + static_cast<std::wstring>(Parameter.DefaultValue.Text);
		}

		return Result;
	}

	std::wstring PrintParser::FormatTemplateParameter(const ParsedTemplateParameter& Parameter) const
	{
		std::wstring Result;

		switch (Parameter.Kind)
		{
		case ParsedTemplateParameter::EKind::Type:
		{
			const std::wstring Constraint = FormatName(Parameter.Constraint);
			Result += Constraint.empty() ? std::wstring(L"typename") : Constraint;
			if (Parameter.IsVariadic)
			{
				Result += L"...";
			}
			if (Parameter.Name.Size() > 0)
			{
				if (!Result.empty()) { Result += L" "; }
				Result += static_cast<std::wstring>(Parameter.Name);
			}
			if (Parameter.HasDefault)
			{
				Result += L" = " + FormatType(Parameter.DefaultType);
			}
			break;
		}
		case ParsedTemplateParameter::EKind::NonType:
			Result += FormatType(Parameter.Type);
			if (Parameter.IsVariadic)
			{
				Result += L"...";
			}
			if (Parameter.Name.Size() > 0)
			{
				if (!Result.empty()) { Result += L" "; }
				Result += static_cast<std::wstring>(Parameter.Name);
			}
			if (Parameter.HasDefault)
			{
				Result += L" = " + static_cast<std::wstring>(Parameter.DefaultExpression.Text);
			}
			break;
		case ParsedTemplateParameter::EKind::TemplateTemplate:
			Result += L"template";
			if (Parameter.Name.Size() > 0)
			{
				Result += L" ";
				Result += static_cast<std::wstring>(Parameter.Name);
			}
			if (Parameter.HasDefault)
			{
				Result += L" = " + FormatType(Parameter.DefaultType);
			}
			break;
		}

		if (Parameter.RequiresClause.Text.Size() > 0)
		{
			Result += L" requires " + static_cast<std::wstring>(Parameter.RequiresClause.Text);
		}

		return Result;
	}

	void PrintParser::PrintFunctionPrefix(const WChar* FunctionName)
	{
		static constexpr size_t PrefixWidth = 32;

		m_Output << L"[" << FunctionName << L"]";

		const size_t PrefixLength = std::char_traits<WChar>::length(FunctionName) + 2;
		for (size_t Index = PrefixLength; Index < PrefixWidth; ++Index)
		{
			m_Output << L" ";
		}
	}

	void PrintParser::PrintIndent(const WChar* FunctionName)
	{
		if (FunctionName)
		{
			PrintFunctionPrefix(FunctionName);
		}
		for (size_t Index = 0; Index < m_Indent; ++Index)
		{
			m_Output << L"\t";
		}
	}
}
