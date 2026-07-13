#include "PrintParser.h"

#include <string>

namespace CE
{
	PrintParser::PrintParser(const std::filesystem::path& Path, TextTokenizer& Tokenizer, std::wostream& Output) : CppParser(Path, Tokenizer), m_Output(Output)
	{

	}

	void PrintParser::OnParseBegin()
	{
		std::wstring FileName = CurrentFile().wstring();
		std::wstring Border(FileName.size(), '=');
		m_Output << L"\n// " << Border << L"\n// " << FileName << L"\n// " << Border << L"\n\n";
	}

	bool PrintParser::OnParsed_Namespace(const Array<ParsedNamespace>& Namespaces)
	{
		PrintFunctionText(L"OnParsed_Namespace");
		PrintIndentText();
		if (Namespaces.Size() > 0 && Namespaces[0].IsInline)
		{
			m_Output << L"inline ";
		}
		m_Output << L"namespace ";
		for (const ParsedNamespace& Namespace : Namespaces)
		{
			String Attributes = FormatAttributes(Namespace.Attributes);
			if (Attributes.Size() > 0)
			{
				m_Output << Attributes.Data() << L" ";
			}
		}
		bool IsFirst = true;
		for (const ParsedNamespace& Namespace : Namespaces)
		{
			if (IsFirst)
			{
				IsFirst = false;
			}
			else
			{
				m_Output << L"::";
				if (Namespace.IsInline)
				{
					m_Output << L"inline ";
				}
			}
			m_Output << Namespace.Name.Data();
		}
		m_Output << L"\n";
		PrintFunctionText(L"OnParsed_Namespace");
		PrintIndentText();
		m_Output << L"{\n";
		m_Scopes.Emplace();
		return true;
	}

	bool PrintParser::OnParsed_NamespaceAlias(const ParsedNamespaceAlias& Alias)
	{
		PrintFunctionText(L"OnParsed_NamespaceAlias");
		PrintIndentText();
		m_Output << L"namespace ";
		String Attributes = FormatAttributes(Alias.Attributes);
		if (Attributes.Size() > 0)
		{
			m_Output << Attributes.Data() << L" ";
		}
		m_Output << FormatName(Alias.Name).Data() << L" = " << FormatName(Alias.Target).Data() << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_ScopeEnd()
	{
		if (m_Scopes.Size() == 0)
		{
			return false;
		}

		PrintFunctionText(L"OnParsed_ScopeEnd");
		PrintIndentText(-1);
		m_Output << L"}";

		m_LastClosedScope = std::move(m_Scopes[m_Scopes.Size() - 1]);
		m_Scopes.RemoveAt(m_Scopes.Size() - 1);
		if (m_LastClosedScope.RequiresSemicolon)
		{
			m_Output << L";";
		}
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Class(const ParsedClass& Class)
	{
		PrintFunctionText(L"OnParsed_Class");
		PrintIndentText();
		if (Class.IsFriend)
		{
			m_Output << L"friend ";
		}

		switch (Class.Type)
		{
		case EClassType::Class:
			m_Output << L"class";
			break;
		case EClassType::Struct:
			m_Output << L"struct";
			break;
		case EClassType::Union:
			m_Output << L"union";
			break;
		}

		String Attributes = FormatAttributes(Class.Attributes);
		if (Attributes.Size() > 0)
		{
			m_Output << L" " << Attributes.Data();
		}
		if (!Class.IsAnonymous)
		{
			m_Output << L" " << FormatName(Class.Name).Data();
		}

		if (Class.Specialization.Size() > 0)
		{
			m_Output << L"<";
			for (size_t Index = 0; Index < Class.Specialization.Size(); ++Index)
			{
				if (Index > 0)
				{
					m_Output << L", ";
				}

				const ParsedTemplateArgument& Argument = Class.Specialization[Index];
				if (Argument.Kind == ParsedTemplateArgument::EKind::Type)
				{
					m_Output << FormatName(Argument.Type.Name).Data();
				}
				else
				{
					m_Output << Argument.Expression.Text.Data();
				}
			}
			m_Output << L">";
		}

		if (Class.IsFinal)
		{
			m_Output << L" final";
		}

		if (Class.BaseClasses.Size() > 0)
		{
			m_Output << L" : ";
			for (size_t Index = 0; Index < Class.BaseClasses.Size(); ++Index)
			{
				if (Index > 0)
				{
					m_Output << L", ";
				}

				const ParsedBaseClass& BaseClass = Class.BaseClasses[Index];
				switch (BaseClass.AccessSpecifier)
				{
				case EAccessSpecifier::Public:
					m_Output << L"public ";
					break;
				case EAccessSpecifier::Protected:
					m_Output << L"protected ";
					break;
				case EAccessSpecifier::Private:
					m_Output << L"private ";
					break;
				}
				if (BaseClass.IsVirtual)
				{
					m_Output << L"virtual ";
				}
				m_Output << FormatName(BaseClass.Type.Name).Data();
			}
		}

		if (!Class.HasBody)
		{
			m_Output << L";\n";
			return true;
		}

		m_Output << L"\n";
		PrintFunctionText(L"OnParsed_Class");
		PrintIndentText();
		m_Output << L"{\n";
		m_Scopes.Emplace();
		m_Scopes[m_Scopes.Size() - 1].RequiresSemicolon = true;
		m_Scopes[m_Scopes.Size() - 1].Name = Class.Name;
		return true;
	}

	bool PrintParser::OnParsed_Access(EAccessSpecifier Access)
	{
		PrintFunctionText(L"OnParsed_Access");
		PrintIndentText(-1);
		switch (Access)
		{
		default:
		case EAccessSpecifier::Public:
			m_Output << L"public:\n";
			break;
		case EAccessSpecifier::Protected:
			m_Output << L"protected:\n";
			break;
		case EAccessSpecifier::Private:
			m_Output << L"private:\n";
			break;
		}
		return true;
	}

	bool PrintParser::OnParsed_Enum(const ParsedEnum& Enum)
	{
		PrintFunctionText(L"OnParsed_Enum");
		PrintIndentText();
		m_Output << L"enum";
		if (Enum.IsScoped)
		{
			m_Output << (Enum.IsStruct ? L" struct" : L" class");
		}

		String Attributes = FormatAttributes(Enum.Attributes);
		if (Attributes.Size() > 0)
		{
			m_Output << L" " << Attributes.Data();
		}
		if (!Enum.IsAnonymous)
		{
			m_Output << L" " << FormatName(Enum.Name).Data();
		}
		if (Enum.UnderlyingType.Name.Segments.Size() > 0)
		{
			m_Output << L" : " << FormatType(Enum.UnderlyingType).Data();
		}

		if (Enum.IsForward)
		{
			m_Output << L";\n";
			return true;
		}

		m_Output << L"\n";
		PrintFunctionText(L"OnParsed_Enum");
		PrintIndentText();
		m_Output << L"{\n";
		m_Scopes.Emplace();
		m_Scopes[m_Scopes.Size() - 1].RequiresSemicolon = true;
		m_Scopes[m_Scopes.Size() - 1].Name = Enum.Name;
		return true;
	}

	bool PrintParser::OnParsed_EnumValue(const ParsedEnumValue& Value)
	{
		PrintFunctionText(L"OnParsed_EnumValue");
		PrintIndentText();
		m_Output << Value.Name.Data();

		String Attributes = FormatAttributes(Value.Attributes);
		if (Attributes.Size() > 0)
		{
			m_Output << L" " << Attributes.Data();
		}

		if (Value.HasValue)
		{
			m_Output << L" = " << Value.Value.Text.Data();
		}

		m_Output << L",\n";
		return true;
	}

	bool PrintParser::OnParsed_Variable(const ParsedVariable& Value)
	{
		PrintFunctionText(L"OnParsed_Variable");
		PrintIndentText();
		String Attributes = FormatAttributes(Value.Attributes);
		if (Attributes.Size() > 0)
		{
			m_Output << Attributes.Data() << L" ";
		}
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsStatic))
		{
			m_Output << L"static ";
		}
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsThreadLocal))
		{
			m_Output << L"thread_local ";
		}
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsExtern))
		{
			m_Output << L"extern ";
		}
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsMutable))
		{
			m_Output << L"mutable ";
		}
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsConstexpr))
		{
			m_Output << L"constexpr ";
		}
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsConsteval))
		{
			m_Output << L"consteval ";
		}

		m_Output << FormatType(Value.Type).Data() << L" " << FormatName(Value.Name).Data();
		if (HasFlag(Value.Flags, EParsedVariableFlags::HasInitializer))
		{
			m_Output << (HasFlag(Value.Flags, EParsedVariableFlags::IsBitfield) ? L" : " : L" = ") << Value.Initializer.Text.Data();
		}
		m_Output << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_Constructor(const ParsedConstructor& Constructor)
	{
		return PrintParsedFunction(L"OnParsed_Constructor", Constructor);
	}

	bool PrintParser::OnParsed_Destructor(const ParsedDestructor& Destructor)
	{
		return PrintParsedFunction(L"OnParsed_Destructor", Destructor);
	}

	bool PrintParser::OnParsed_Function(const ParsedFunction& Function)
	{
		return PrintParsedFunction(L"OnParsed_Function", Function, &Function.ReturnType, nullptr, Function.IsTrailingType);
	}

	bool PrintParser::OnParsed_Operator(const ParsedOperator& Operator)
	{
		return PrintParsedFunction(L"OnParsed_Operator", Operator, &Operator.ReturnType, &Operator.Symbol, Operator.IsTrailingType);
	}

	bool PrintParser::PrintParsedFunction(const String& CallbackName, const ParsedFunctionBase& Function, const ParsedType* ReturnType, const String* OperatorSymbol, bool IsTrailingReturnType)
	{
		PrintFunctionText(CallbackName);
		PrintIndentText();

		String Attributes = FormatAttributes(Function.Attributes);
		if (Attributes.Size() > 0)
		{
			m_Output << Attributes.Data() << L" ";
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsStatic))
		{
			m_Output << L"static ";
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsInline))
		{
			m_Output << L"inline ";
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsVirtual))
		{
			m_Output << L"virtual ";
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsExplicit))
		{
			m_Output << L"explicit ";
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsConstexpr))
		{
			m_Output << L"constexpr ";
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsConsteval))
		{
			m_Output << L"consteval ";
		}

		if (ReturnType != nullptr)
		{
			m_Output << (IsTrailingReturnType ? L"auto" : FormatType(*ReturnType).Data()) << L" ";
		}
		if (OperatorSymbol != nullptr && OperatorSymbol->Size() > 0)
		{
			m_Output << L"operator" << OperatorSymbol->Data();
		}
		else
		{
			m_Output << FormatName(Function.Name).Data();
		}

		m_Output << L"(";
		for (size_t Index = 0; Index < Function.Parameters.Size(); ++Index)
		{
			if (Index > 0)
			{
				m_Output << L", ";
			}

			const ParsedFunctionParameter& Parameter = Function.Parameters[Index];
			String ParameterAttributes = FormatAttributes(Parameter.Attributes);
			if (ParameterAttributes.Size() > 0)
			{
				m_Output << ParameterAttributes.Data() << L" ";
			}
			if (Parameter.IsVariadic)
			{
				m_Output << L"...";
			}
			else
			{
				m_Output << FormatType(Parameter.Type).Data();
				if (Parameter.Name.Segments.Size() > 0)
				{
					m_Output << L" " << FormatName(Parameter.Name).Data();
				}
			}
			if (Parameter.HasDefaultValue)
			{
				m_Output << L" = " << Parameter.DefaultValue.Text.Data();
			}
		}
		m_Output << L")";

		for (const String& Specifier : Function.Specifiers)
		{
			m_Output << L" " << Specifier.Data();
		}
		for (const String& Qualifier : Function.Qualifiers)
		{
			m_Output << L" " << Qualifier.Data();
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsNoexcept))
		{
			m_Output << L" noexcept";
			if (Function.NoexceptExpression.Text.Size() > 0)
			{
				m_Output << L"(" << Function.NoexceptExpression.Text.Data() << L")";
			}
		}
		if (IsTrailingReturnType)
		{
			m_Output << L" -> " << FormatType(*ReturnType).Data();
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::HasRequires))
		{
			m_Output << L" requires " << Function.RequiresClause.Text.Data();
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsPureVirtual))
		{
			m_Output << L" = 0";
		}
		else if (HasFlag(Function.Flags, EParsedFunctionFlags::IsDefaulted))
		{
			m_Output << L" = default";
		}
		else if (HasFlag(Function.Flags, EParsedFunctionFlags::IsDeleted))
		{
			m_Output << L" = delete";
		}

		if (HasFlag(Function.Flags, EParsedFunctionFlags::HasBody))
		{
			m_Output << L" " << Function.Body.Text.Data();
		}
		else
		{
			m_Output << L";";
		}
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Using(const ParsedUsing& Using)
	{
		PrintFunctionText(L"OnParsed_Using");
		PrintIndentText();

		String Attributes = FormatAttributes(Using.Attributes);
		switch (Using.Kind)
		{
		case ParsedUsing::EKind::Typedef:
			if (Attributes.Size() > 0)
			{
				m_Output << Attributes.Data() << L" ";
			}
			m_Output << L"typedef " << FormatType(Using.Type).Data() << L" " << FormatName(Using.Name).Data();
			break;
		case ParsedUsing::EKind::AliasDeclaration:
			m_Output << L"using " << FormatName(Using.Name).Data();
			if (Attributes.Size() > 0)
			{
				m_Output << L" " << Attributes.Data();
			}
			m_Output << L" = " << FormatType(Using.Type).Data();
			break;
		case ParsedUsing::EKind::UsingDeclaration:
			m_Output << L"using ";
			if (Attributes.Size() > 0)
			{
				m_Output << Attributes.Data() << L" ";
			}
			m_Output << FormatName(Using.Target).Data();
			break;
		case ParsedUsing::EKind::UsingDirective:
			m_Output << L"using namespace ";
			if (Attributes.Size() > 0)
			{
				m_Output << Attributes.Data() << L" ";
			}
			m_Output << FormatName(Using.Target).Data();
			break;
		}

		m_Output << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_Template(const ParsedTemplate& Template)
	{
		PrintFunctionText(L"OnParsed_Template");
		PrintIndentText();
		m_Output << L"template<";
		for (size_t Index = 0; Index < Template.Parameters.Size(); ++Index)
		{
			if (Index > 0)
			{
				m_Output << L", ";
			}

			const ParsedTemplateParameter& Parameter = Template.Parameters[Index];
			switch (Parameter.Kind)
			{
			case ParsedTemplateParameter::EKind::Type:
				if (Parameter.Constraint.Segments.Size() > 0)
				{
					m_Output << FormatName(Parameter.Constraint).Data() << L" ";
				}
				else
				{
					m_Output << L"typename ";
				}
				break;
			case ParsedTemplateParameter::EKind::NonType:
			case ParsedTemplateParameter::EKind::TemplateTemplate:
				m_Output << FormatType(Parameter.Type).Data() << L" ";
				break;
			}

			if (Parameter.IsVariadic)
			{
				m_Output << L"...";
			}
			m_Output << Parameter.Name.Data();

			if (Parameter.HasDefault)
			{
				m_Output << L" = ";
				if (Parameter.Kind == ParsedTemplateParameter::EKind::NonType)
				{
					m_Output << Parameter.DefaultExpression.Text.Data();
				}
				else
				{
					m_Output << FormatType(Parameter.DefaultType).Data();
				}
			}
		}
		m_Output << L">";
		if (Template.HasRequires)
		{
			m_Output << L" requires " << Template.RequiresClause.Text.Data();
		}
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Concept(const ParsedConcept& Concept)
	{
		PrintFunctionText(L"OnParsed_Concept");
		PrintIndentText();
		String Attributes = FormatAttributes(Concept.Attributes);
		if (Attributes.Size() > 0)
		{
			m_Output << Attributes.Data() << L" ";
		}
		m_Output << L"concept " << FormatName(Concept.Name).Data() << L" = " << Concept.Constraint.Text.Data() << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_StaticAssert(const ParsedStaticAssert& Assert)
	{
		PrintFunctionText(L"OnParsed_StaticAssert");
		PrintIndentText();
		m_Output << L"static_assert(" << Assert.Condition.Text.Data();
		if (Assert.HasMessage)
		{
			m_Output << L", " << Assert.Message.Text.Data();
		}
		m_Output << L");\n";
		return true;
	}

	bool PrintParser::HasFlag(EParsedVariableFlags Flags, EParsedVariableFlags Flag) const
	{
		return (static_cast<uint8>(Flags) & static_cast<uint8>(Flag)) != 0;
	}

	bool PrintParser::HasFlag(EParsedFunctionFlags Flags, EParsedFunctionFlags Flag) const
	{
		return (static_cast<uint16>(Flags) & static_cast<uint16>(Flag)) != 0;
	}

	bool PrintParser::HasFlag(EParsedTypeFlags Flags, EParsedTypeFlags Flag) const
	{
		return (static_cast<uint8>(Flags) & static_cast<uint8>(Flag)) != 0;
	}

	void PrintParser::PrintFunctionText(const String& Name)
	{
		if (!PrintFunction)
		{
			return;
		}
		String AlignedName = Name;
		if (AlignedName.Size() < 25)
		{
			AlignedName.Append(' ', 25 - AlignedName.Size());
		}
		m_Output << L"[" << AlignedName.Data() << L"] ";
	}

	void PrintParser::PrintIndentText(int32 Shift)
	{
		const int32 Indentation = static_cast<int32>(m_Scopes.Size()) + Shift;
		if (Indentation > 0)
		{
			m_Output << String('\t', static_cast<size_t>(Indentation)).Data();
		}
	}

	String PrintParser::FormatAttributes(const Array<ParsedAttribute>& Attributes) const
	{
		String Result;
		for (const ParsedAttribute& Attribute : Attributes)
		{
			if (Result.Size() > 0)
			{
				Result.Append(L" ");
			}

			switch (Attribute.Kind)
			{
			case ParsedAttribute::EKind::Standard:
				Result.Append(L"[[");
				break;
			case ParsedAttribute::EKind::Declspec:
				Result.Append(L"__declspec(");
				break;
			case ParsedAttribute::EKind::Gnu:
				Result.Append(L"__attribute__((");
				break;
			case ParsedAttribute::EKind::Alignas:
				Result.Append(L"alignas(");
				break;
			case ParsedAttribute::EKind::Other:
				break;
			}

			Result.Append(FormatName(Attribute.Name));
			const bool HasArguments = Attribute.Arguments.Size() > 0;
			const bool ArgumentsFollowName = HasArguments && Attribute.Name.Segments.Size() > 0;
			if (ArgumentsFollowName)
			{
				Result.Append(L"(");
			}
			if (HasArguments)
			{
				for (size_t Index = 0; Index < Attribute.Arguments.Size(); ++Index)
				{
					if (Index > 0)
					{
						Result.Append(L", ");
					}
					Result.Append(Attribute.Arguments[Index].Text);
				}
			}
			if (ArgumentsFollowName)
			{
				Result.Append(L")");
			}

			switch (Attribute.Kind)
			{
			case ParsedAttribute::EKind::Standard:
				Result.Append(L"]]");
				break;
			case ParsedAttribute::EKind::Declspec:
			case ParsedAttribute::EKind::Alignas:
				Result.Append(L")");
				break;
			case ParsedAttribute::EKind::Gnu:
				Result.Append(L"))");
				break;
			case ParsedAttribute::EKind::Other:
				break;
			}
		}
		return Result;
	}

	String PrintParser::FormatName(const ParsedName& Name) const
	{
		String Result;
		for (size_t SegmentIndex = 0; SegmentIndex < Name.Segments.Size(); ++SegmentIndex)
		{
			if (SegmentIndex > 0)
			{
				Result.Append(L"::");
			}

			const ParsedNameSegment& Segment = Name.Segments[SegmentIndex];
			if (Segment.bIsInline)
			{
				Result.Append(L"inline ");
			}
			Result.Append(Segment.Name);

			if (Segment.TemplateArguments.Size() > 0)
			{
				Result.Append(L"<");
				for (size_t ArgumentIndex = 0; ArgumentIndex < Segment.TemplateArguments.Size(); ++ArgumentIndex)
				{
					if (ArgumentIndex > 0)
					{
						Result.Append(L", ");
					}

					const ParsedTemplateArgument& Argument = Segment.TemplateArguments[ArgumentIndex];
					if (Argument.Kind == ParsedTemplateArgument::EKind::Type)
					{
						Result.Append(FormatType(Argument.Type));
					}
					else
					{
						Result.Append(Argument.Expression.Text);
					}
				}
				Result.Append(L">");
			}
		}
		return Result;
	}

	String PrintParser::FormatType(const ParsedType& Type) const
	{
		String Result;
		const auto AppendSpecifier = [&Result](const WChar* Specifier)
		{
			if (Result.Size() > 0)
			{
				Result.Append(L" ");
			}
			Result.Append(Specifier);
		};

		if (HasFlag(Type.Flags, EParsedTypeFlags::IsConst))
		{
			AppendSpecifier(L"const");
		}
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsVolatile))
		{
			AppendSpecifier(L"volatile");
		}
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsMutable))
		{
			AppendSpecifier(L"mutable");
		}
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsUnsigned))
		{
			AppendSpecifier(L"unsigned");
		}
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsSigned))
		{
			AppendSpecifier(L"signed");
		}

		const String Attributes = FormatAttributes(Type.Attributes);
		if (Attributes.Size() > 0)
		{
			AppendSpecifier(Attributes.Data());
		}
		if (Type.Name.Segments.Size() > 0)
		{
			AppendSpecifier(FormatName(Type.Name).Data());
		}

		for (const ParsedIndirection& Indirection : Type.Indirections)
		{
			switch (Indirection.Kind)
			{
			case ParsedIndirection::EKind::Pointer:
				Result.Append(L"*");
				break;
			case ParsedIndirection::EKind::LReference:
				Result.Append(L"&");
				break;
			case ParsedIndirection::EKind::RReference:
				Result.Append(L"&&");
				break;
			}

			if (Indirection.IsConst)
			{
				Result.Append(L" const");
			}
			if (Indirection.IsVolatile)
			{
				Result.Append(L" volatile");
			}
			if (Indirection.IsMutable)
			{
				Result.Append(L" mutable");
			}
		}

		for (const ParsedExpression& Extent : Type.ArrayExtents)
		{
			Result.Append(L"[");
			Result.Append(Extent.Text);
			Result.Append(L"]");
		}

		return Result;
	}
}
