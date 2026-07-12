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
		static const WChar* FuncText = L"OnParsed_Namespace";
		PrintFunctionText(FuncText);
		PrintIndentText();
		if (Namespaces.Size() > 0 && Namespaces[0].IsInline)
		{
			m_Output << L"inline ";
		}
		m_Output << L"namespace ";
		if (Namespaces.Size() > 0)
		{
			const String Attributes = FormatAttributes(Namespaces[0].Attributes);
			if (Attributes.Size() > 0)
			{
				m_Output << Attributes.Data() << L" ";
			}
		}
		bool IsFirst = true;
		for (auto& Namespace : Namespaces)
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
		m_Output << "\n";
		PrintFunctionText(FuncText);
		PrintIndentText();
		m_Output << L"{\n";
		m_Scopes.Emplace(ScopeInfo{});
		return true;
	}

	bool PrintParser::OnParsed_NamespaceAlias(const ParsedNamespaceAlias& Alias)
	{
		PrintFunctionText(L"OnParsed_NamespaceAlias");
		PrintIndentText();
		m_Output << L"namespace " << FormatName(Alias.Name).Data() << L" = " << FormatName(Alias.Target).Data() << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_ScopeEnd()
	{
		if (m_Scopes.Size() == 0)
		{
			return false;
		}
		m_LastClosedScope = m_Scopes[m_Scopes.Size() - 1];
		m_Scopes.RemoveAt(m_Scopes.Size() - 1);
		PrintFunctionText(L"OnParsed_ScopeEnd");
		PrintIndentText();
		m_Output << L"}" << (m_LastClosedScope.RequiresSemicolon ? L";" : L"") << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Class(const ParsedClass& Class)
	{
		ParsedType AnonymousType;
		const ParsedName* Name = &Class.Name;
		if (Class.IsAnonymous)
		{
			AnonymousType.Name.Segments.Emplace(ParsedNameSegment{ String::Format(L"__ANONYMOUS_%zu__", ++m_AnonymousTypeCount) });
			Name = &AnonymousType.Name;
		}

		PrintFunctionText(L"OnParsed_Class");
		PrintIndentText();
		const String Attributes = FormatAttributes(Class.Attributes);
		if (Class.IsFriend) m_Output << L"friend ";
		switch (Class.Type)
		{
		case EClassType::Class: m_Output << L"class "; break;
		case EClassType::Struct: m_Output << L"struct "; break;
		case EClassType::Union: m_Output << L"union "; break;
		}
		if (Attributes.Size() > 0) m_Output << Attributes.Data() << L" ";
		m_Output << FormatName(*Name).Data();
		if (Class.IsFinal) m_Output << L" final";
		if (Class.BaseClasses.Size() > 0)
		{
			m_Output << L" : ";
			bool First = true;
			for (const ParsedBaseClass& Base : Class.BaseClasses)
			{
				if (!First) m_Output << L", ";
				First = false;
				switch (Base.AccessSpecifier)
				{
				case EAccessSpecifier::Public: m_Output << L"public "; break;
				case EAccessSpecifier::Protected: m_Output << L"protected "; break;
				case EAccessSpecifier::Private: m_Output << L"private "; break;
				}
				if (Base.IsVirtual) m_Output << L"virtual ";
				m_Output << FormatType(Base.Type).Data();
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
		ScopeInfo Scope;
		Scope.RequiresSemicolon = true;
		Scope.Type = std::move(AnonymousType);
		m_Scopes.Emplace(Scope);
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
		ParsedType AnonymousType;
		const ParsedName* Name = &Enum.Name;
		if (Enum.IsAnonymous)
		{
			AnonymousType.Name.Segments.Emplace(ParsedNameSegment{ String::Format(L"__ANONYMOUS_%zu__", ++m_AnonymousTypeCount) });
			Name = &AnonymousType.Name;
		}

		PrintFunctionText(L"OnParsed_Enum");
		PrintIndentText();
		const String Attributes = FormatAttributes(Enum.Attributes);
		m_Output << L"enum ";
		if (Enum.IsScoped) m_Output << (Enum.IsStruct ? L"struct " : L"class ");
		if (Attributes.Size() > 0) m_Output << Attributes.Data() << L" ";
		m_Output << FormatName(*Name).Data();
		if (Enum.UnderlyingType.Name.Segments.Size() > 0) m_Output << L" : " << FormatType(Enum.UnderlyingType).Data();
		if (Enum.IsForward)
		{
			m_Output << L";\n";
			return true;
		}
		m_Output << L"\n";
		PrintFunctionText(L"OnParsed_Enum");
		PrintIndentText();
		m_Output << L"{\n";
		ScopeInfo Scope;
		Scope.RequiresSemicolon = true;
		Scope.Type = std::move(AnonymousType);
		m_Scopes.Emplace(Scope);
		return true;
	}

	bool PrintParser::OnParsed_EnumValue(const ParsedEnumValue& Value)
	{
		PrintFunctionText(L"OnParsed_EnumValue");
		PrintIndentText();
		const String Attributes = FormatAttributes(Value.Attributes);
		if (Attributes.Size() > 0) m_Output << Attributes.Data() << L" ";
		m_Output << Value.Name.Data();
		if (Value.HasValue) m_Output << L" = " << FormatExpression(Value.Value).Data();
		m_Output << L",\n";
		return true;
	}

	bool PrintParser::OnParsed_Variable(const ParsedVariable& Value)
	{
		PrintFunctionText(L"OnParsed_Variable");
		PrintIndentText();
		const String Attributes = FormatAttributes(Value.Attributes);
		if (Attributes.Size() > 0) m_Output << Attributes.Data() << L" ";
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsExtern)) m_Output << L"extern ";
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsStatic)) m_Output << L"static ";
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsThreadLocal)) m_Output << L"thread_local ";
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsMutable)) m_Output << L"mutable ";
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsConstexpr)) m_Output << L"constexpr ";
		if (HasFlag(Value.Flags, EParsedVariableFlags::IsConsteval)) m_Output << L"consteval ";
		const ParsedType& Type = Value.Type.Name.Segments.Size() > 0 ? Value.Type : m_LastClosedScope.Type;
		m_Output << FormatType(Type, FormatName(Value.Name)).Data();
		if (HasFlag(Value.Flags, EParsedVariableFlags::HasInitializer)) m_Output << L" = " << FormatExpression(Value.Initializer).Data();
		m_Output << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_Constructor(const ParsedConstructor& Constructor)
	{
		PrintFunctionText(L"OnParsed_Constructor");
		PrintIndentText();
		m_Output << FormatFunction(Constructor, FormatName(Constructor.Name)).Data();
		if (HasFlag(Constructor.Flags, EParsedFunctionFlags::HasBody)) m_Output << L" " << FormatExpression(Constructor.Body).Data();
		else m_Output << L";";
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Destructor(const ParsedDestructor& Destructor)
	{
		PrintFunctionText(L"OnParsed_Destructor");
		PrintIndentText();
		m_Output << FormatFunction(Destructor, L"~" + FormatName(Destructor.Name)).Data();
		if (HasFlag(Destructor.Flags, EParsedFunctionFlags::HasBody)) m_Output << L" " << FormatExpression(Destructor.Body).Data();
		else m_Output << L";";
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Function(const ParsedFunction& Function)
	{
		PrintFunctionText(L"OnParsed_Function");
		PrintIndentText();
		String Declaration = Function.IsTrailingType ? L"auto " + FormatName(Function.Name) : FormatType(Function.ReturnType, FormatName(Function.Name));
		m_Output << FormatFunction(Function, Declaration).Data();
		if (Function.IsTrailingType) m_Output << L" -> " << FormatType(Function.ReturnType).Data();
		if (HasFlag(Function.Flags, EParsedFunctionFlags::HasBody)) m_Output << L" " << FormatExpression(Function.Body).Data();
		else m_Output << L";";
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Operator(const ParsedOperator& Operator)
	{
		PrintFunctionText(L"OnParsed_Operator");
		PrintIndentText();
		String Name = FormatName(Operator.Name);
		if (Name.Size() == 0) Name = L"operator" + Operator.Symbol;
		m_Output << FormatFunction(Operator, FormatType(Operator.ReturnType, Name)).Data();
		if (HasFlag(Operator.Flags, EParsedFunctionFlags::HasBody)) m_Output << L" " << FormatExpression(Operator.Body).Data();
		else m_Output << L";";
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Using(const ParsedUsing& Using)
	{
		PrintFunctionText(L"OnParsed_Using");
		PrintIndentText();
		const String Attributes = FormatAttributes(Using.Attributes);
		if (Attributes.Size() > 0) m_Output << Attributes.Data() << L" ";
		switch (Using.Kind)
		{
		case ParsedUsing::EKind::Typedef: m_Output << L"typedef " << FormatType(Using.Type, FormatName(Using.Name)).Data(); break;
		case ParsedUsing::EKind::AliasDeclaration: m_Output << L"using " << FormatName(Using.Name).Data() << L" = " << FormatType(Using.Type).Data(); break;
		case ParsedUsing::EKind::UsingDeclaration: m_Output << L"using " << FormatName(Using.Target).Data(); break;
		case ParsedUsing::EKind::UsingDirective: m_Output << L"using namespace " << FormatName(Using.Target).Data(); break;
		}
		m_Output << L";\n";
		return true;
	}

	bool PrintParser::OnParsed_Template(const ParsedTemplate& Template)
	{
		PrintFunctionText(L"OnParsed_Template");
		PrintIndentText();
		m_Output << L"template<";
		String Parameters;
		for (const ParsedTemplateParameter& Parameter : Template.Parameters) AppendSeparated(Parameters, FormatTemplateParameter(Parameter));
		m_Output << Parameters.Data() << L">";
		if (Template.HasRequires) m_Output << L" requires " << FormatExpression(Template.RequiresClause).Data();
		m_Output << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_Concept(const ParsedTemplate& Concept)
	{
		PrintFunctionText(L"OnParsed_Concept");
		PrintIndentText();
		if (Concept.HasRequires) m_Output << L"requires " << FormatExpression(Concept.RequiresClause).Data() << L"\n";
		return true;
	}

	bool PrintParser::OnParsed_StaticAssert(const ParsedStaticAssert& Assert)
	{
		PrintFunctionText(L"OnParsed_StaticAssert");
		PrintIndentText();
		m_Output << L"static_assert(" << FormatExpression(Assert.Condition).Data();
		if (Assert.HasMessage) m_Output << L", " << FormatExpression(Assert.Message).Data();
		m_Output << L");\n";
		return true;
	}

	void PrintParser::PrintFunctionText(const String& Name)
	{
		if (!PrintFunction)
		{
			return;
		}
		String AlignedName = Name;
		if (AlignedName.Size() < 10)
		{
			AlignedName.Append(' ', 10 - AlignedName.Size());
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

	bool PrintParser::HasFlag(EParsedTypeFlags Flags, EParsedTypeFlags Flag)
	{
		return (static_cast<uint8>(Flags) & static_cast<uint8>(Flag)) != 0;
	}

	bool PrintParser::HasFlag(EParsedVariableFlags Flags, EParsedVariableFlags Flag)
	{
		return (static_cast<uint8>(Flags) & static_cast<uint8>(Flag)) != 0;
	}

	bool PrintParser::HasFlag(EParsedFunctionFlags Flags, EParsedFunctionFlags Flag)
	{
		return (static_cast<uint32>(Flags) & static_cast<uint32>(Flag)) != 0;
	}

	void PrintParser::AppendSeparated(String& Output, const String& Value, const WChar* Separator)
	{
		if (Value.Size() == 0)
		{
			return;
		}
		if (Output.Size() > 0)
		{
			Output += Separator;
		}
		Output += Value;
	}

	String PrintParser::FormatExpression(const ParsedExpression& Expression)
	{
		return Expression.Text;
	}

	String PrintParser::FormatName(const ParsedName& Name)
	{
		String Result;
		for (const ParsedNameSegment& Segment : Name.Segments)
		{
			if (Result.Size() > 0)
			{
				Result += L"::";
			}
			if (Segment.bIsInline)
			{
				Result += L"inline ";
			}
			Result += Segment.Name;
			if (Segment.TemplateArguments.Size() > 0)
			{
				Result += L"<";
				for (const ParsedTemplateArgument& Argument : Segment.TemplateArguments)
				{
					AppendSeparated(Result, FormatTemplateArgument(Argument));
				}
				Result += L">";
			}
		}
		return Result;
	}

	String PrintParser::FormatAttribute(const ParsedAttribute& Attribute)
	{
		String Arguments;
		for (const ParsedExpression& Argument : Attribute.Arguments)
		{
			AppendSeparated(Arguments, FormatExpression(Argument));
		}

		const String Name = FormatName(Attribute.Name);
		switch (Attribute.Kind)
		{
		case ParsedAttribute::EKind::Declspec:
			return L"__declspec(" + Name + (Arguments.Size() > 0 ? L"(" + Arguments + L")" : L"") + L")";
		case ParsedAttribute::EKind::Gnu:
			return L"__attribute__((" + Name + (Arguments.Size() > 0 ? L"(" + Arguments + L")" : L"") + L"))";
		case ParsedAttribute::EKind::Alignas:
			return L"alignas(" + (Arguments.Size() > 0 ? Arguments : Name) + L")";
		default:
			return L"[[" + Name + (Arguments.Size() > 0 ? L"(" + Arguments + L")" : L"") + L"]]";
		}
	}

	String PrintParser::FormatAttributes(const Array<ParsedAttribute>& Attributes)
	{
		String Result;
		for (const ParsedAttribute& Attribute : Attributes)
		{
			AppendSeparated(Result, FormatAttribute(Attribute), L" ");
		}
		return Result;
	}

	String PrintParser::FormatType(const ParsedType& Type, const String& Declarator)
	{
		String Result = FormatAttributes(Type.Attributes);
		if (Result.Size() > 0)
		{
			Result += L" ";
		}
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsConst)) Result += L"const ";
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsVolatile)) Result += L"volatile ";
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsMutable)) Result += L"mutable ";
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsUnsigned)) Result += L"unsigned ";
		if (HasFlag(Type.Flags, EParsedTypeFlags::IsSigned)) Result += L"signed ";
		Result += FormatName(Type.Name);

		for (const ParsedIndirection& Indirection : Type.Indirections)
		{
			switch (Indirection.Kind)
			{
			case ParsedIndirection::EKind::Pointer: Result += L"*"; break;
			case ParsedIndirection::EKind::LReference: Result += L"&"; break;
			case ParsedIndirection::EKind::RReference: Result += L"&&"; break;
			}
			if (Indirection.IsConst) Result += L" const";
			if (Indirection.IsVolatile) Result += L" volatile";
			if (Indirection.IsMutable) Result += L" mutable";
		}

		if (Declarator.Size() > 0)
		{
			if (Result.Size() > 0)
			{
				Result += L" ";
			}
			Result += Declarator;
		}
		for (const ParsedExpression& Extent : Type.ArrayExtents)
		{
			Result += L"[" + FormatExpression(Extent) + L"]";
		}
		return Result;
	}

	String PrintParser::FormatTemplateArgument(const ParsedTemplateArgument& Argument)
	{
		return Argument.Kind == ParsedTemplateArgument::EKind::Type ? FormatType(Argument.Type) : FormatExpression(Argument.Expression);
	}

	String PrintParser::FormatParameter(const ParsedFunctionParameter& Parameter)
	{
		if (Parameter.IsVariadic)
		{
			return L"...";
		}
		String Result = FormatAttributes(Parameter.Attributes);
		if (Result.Size() > 0)
		{
			Result += L" ";
		}
		Result += FormatType(Parameter.Type, FormatName(Parameter.Name));
		if (Parameter.HasDefaultValue)
		{
			Result += L" = " + FormatExpression(Parameter.DefaultValue);
		}
		return Result;
	}

	String PrintParser::FormatTemplateParameter(const ParsedTemplateParameter& Parameter)
	{
		String Result;
		switch (Parameter.Kind)
		{
		case ParsedTemplateParameter::EKind::Type:
			Result = FormatName(Parameter.Constraint);
			if (Result.Size() > 0) Result += L" ";
			Result += L"typename";
			break;
		case ParsedTemplateParameter::EKind::NonType:
			Result = FormatType(Parameter.Type);
			break;
		case ParsedTemplateParameter::EKind::TemplateTemplate:
			Result = L"template<typename> class";
			break;
		}
		if (Parameter.IsVariadic) Result += L"...";
		if (Parameter.Name.Size() > 0) Result += L" " + Parameter.Name;
		if (Parameter.HasDefault)
		{
			const String Default = Parameter.Kind == ParsedTemplateParameter::EKind::NonType ? FormatExpression(Parameter.DefaultExpression) : FormatType(Parameter.DefaultType);
			Result += L" = " + Default;
		}
		return Result;
	}

	String PrintParser::FormatFunctionSuffix(const ParsedFunctionBase& Function)
	{
		String Result;
		for (const String& Qualifier : Function.Qualifiers)
		{
			Result += L" " + Qualifier;
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsNoexcept))
		{
			Result += L" noexcept";
			if (Function.NoexceptExpression.Text.Size() > 0) Result += L"(" + FormatExpression(Function.NoexceptExpression) + L")";
		}
		if (HasFlag(Function.Flags, EParsedFunctionFlags::HasRequires)) Result += L" requires " + FormatExpression(Function.RequiresClause);
		return Result;
	}

	void PrintParser::AppendFunctionPrefix(String& Result, const ParsedFunctionBase& Function)
	{
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsStatic)) Result += L"static ";
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsInline)) Result += L"inline ";
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsVirtual)) Result += L"virtual ";
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsExplicit)) Result += L"explicit ";
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsConstexpr)) Result += L"constexpr ";
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsConsteval)) Result += L"consteval ";
		for (const String& Specifier : Function.Specifiers) Result += Specifier + L" ";
	}

	String PrintParser::FormatFunction(const ParsedFunctionBase& Function, const String& Declaration)
	{
		String Result = FormatAttributes(Function.Attributes);
		if (Result.Size() > 0) Result += L" ";
		AppendFunctionPrefix(Result, Function);
		Result += Declaration + L"(";
		for (const ParsedFunctionParameter& Parameter : Function.Parameters) AppendSeparated(Result, FormatParameter(Parameter));
		Result += L")" + FormatFunctionSuffix(Function);
		if (HasFlag(Function.Flags, EParsedFunctionFlags::IsPureVirtual)) Result += L" = 0";
		else if (HasFlag(Function.Flags, EParsedFunctionFlags::IsDefaulted)) Result += L" = default";
		else if (HasFlag(Function.Flags, EParsedFunctionFlags::IsDeleted)) Result += L" = delete";
		return Result;
	}
}
