#include "Calculation.h"


namespace CE
{
	uint8 Calculation::ActionWeight(ECalculationAction Action)
	{
		switch (Action)
		{
		default:
		case ECalculationAction::Undefined:
			return 0;
		case ECalculationAction::Add:
			return 3;
		case ECalculationAction::Subtract:
			return 3;
		case ECalculationAction::Multiply:
			return 2;
		case ECalculationAction::Divide:
			return 2;
		case ECalculationAction::Modulo:
			return 2;
		case ECalculationAction::BitWise_And:
			return 7;
		case ECalculationAction::BitWise_Or:
			return 9;
		case ECalculationAction::BitWise_XOr:
			return 8;
		case ECalculationAction::BitWise_Not:
			return 1;
		case ECalculationAction::BitWise_LeftShift:
			return 4;
		case ECalculationAction::BitWise_RightShift:
			return 4;
		case ECalculationAction::Equal:
			return 6;
		case ECalculationAction::NotEqual:
			return 6;
		case ECalculationAction::GreaterThan:
			return 5;
		case ECalculationAction::LessThan:
			return 5;
		case ECalculationAction::GreaterThanEqual:
			return 5;
		case ECalculationAction::LessThanEqual:
			return 5;
		case ECalculationAction::Logical_Not:
			return 1;
		case ECalculationAction::Logical_And:
			return 10;
		case ECalculationAction::Logical_Or:
			return 11;
		case ECalculationAction::Condition_Question:
			return 12;
		case ECalculationAction::Condition_Decission:
			return 12;
		}
	}

	void Calculation::AddToken(const TextToken& Token)
	{
		if (Token.Type == ETextTokenType::Constant)
		{
			if (Token.ConstantType == ETextTokenConstantType::Integral)
			{
				Value(Token.Value_Integral);
			}
			else
			{
				throw TextTokenizerError(L"Expected Integral", Token);
			}
		}
		else if (Token.Type == ETextTokenType::Symbol)
		{
			if (Token.Value_Text == L"(")
			{
				Begin();
			}
			else if (Token.Value_Text == L")")
			{
				End();
			}
			else if (Token.Value_Text == L"+")
			{
				Action(ECalculationAction::Add);
			}
			else if (Token.Value_Text == L"-")
			{
				Action(ECalculationAction::Subtract);
			}
			else if (Token.Value_Text == L"*")
			{
				Action(ECalculationAction::Multiply);
			}
			else if (Token.Value_Text == L"/")
			{
				Action(ECalculationAction::Divide);
			}
			else if (Token.Value_Text == L"%")
			{
				Action(ECalculationAction::Modulo);
			}
			else if (Token.Value_Text == L"&")
			{
				Action(ECalculationAction::BitWise_And);
			}
			else if (Token.Value_Text == L"|")
			{
				Action(ECalculationAction::BitWise_Or);
			}
			else if (Token.Value_Text == L"^")
			{
				Action(ECalculationAction::BitWise_XOr);
			}
			else if (Token.Value_Text == L"~")
			{
				Action(ECalculationAction::BitWise_Not);
			}
			else if (Token.Value_Text == L"<<")
			{
				Action(ECalculationAction::BitWise_LeftShift);
			}
			else if (Token.Value_Text == L">>")
			{
				Action(ECalculationAction::BitWise_RightShift);
			}
			else if (Token.Value_Text == L"==")
			{
				Action(ECalculationAction::Equal);
			}
			else if (Token.Value_Text == L"!=")
			{
				Action(ECalculationAction::NotEqual);
			}
			else if (Token.Value_Text == L">")
			{
				Action(ECalculationAction::GreaterThan);
			}
			else if (Token.Value_Text == L"<")
			{
				Action(ECalculationAction::LessThan);
			}
			else if (Token.Value_Text == L">=")
			{
				Action(ECalculationAction::GreaterThanEqual);
			}
			else if (Token.Value_Text == L"<=")
			{
				Action(ECalculationAction::LessThanEqual);
			}
			else if (Token.Value_Text == L"!")
			{
				Action(ECalculationAction::Logical_Not);
			}
			else if (Token.Value_Text == L"&&")
			{
				Action(ECalculationAction::Logical_And);
			}
			else if (Token.Value_Text == L"||")
			{
				Action(ECalculationAction::Logical_Or);
			}
			else if (Token.Value_Text == L"?")
			{
				Action(ECalculationAction::Condition_Question);
			}
			else if (Token.Value_Text == L":")
			{
				Action(ECalculationAction::Condition_Decission);
			}
			else
			{
				throw TextTokenizerError(L"Unknown Symbol", Token);
			}
		}
		else
		{
			throw TextTokenizerError(L"Unexpected Token", Token);
		}
	}

	void Calculation::Begin()
	{
		if (m_Calculations.Size() > 0)
		{
			Calculation& Back = m_Calculations[m_Calculations.Size() - 1];
			if (Back.m_Type == EType::Calculation || Back.m_Type == EType::Priority)
			{
				Back.Begin();
				return;
			}
		}

		Calculation& NewCalc = m_Calculations.EmplaceRef();
		NewCalc.m_Type = EType::Calculation;
	}

	void Calculation::End()
	{
		if (m_Calculations.Size() > 0)
		{
			Calculation& Back = m_Calculations[m_Calculations.Size() - 1];
			if (Back.m_Type == EType::Calculation || Back.m_Type == EType::Priority)
			{
				Back.End();
				return;
			}
		}

		m_Type = EType::Completed;
	}

	void Calculation::Value(int64_t Value)
	{
		if (m_Calculations.Size() > 0)
		{
			Calculation& Back = m_Calculations[m_Calculations.Size() - 1];
			if (Back.m_Type == EType::Calculation || Back.m_Type == EType::Priority)
			{
				Back.Value(Value);
				return;
			}
		}

		Calculation& NewCalc = m_Calculations.EmplaceRef();
		NewCalc.m_Type = EType::Value;
		NewCalc.m_Value = Value;
	}

	void Calculation::Action(ECalculationAction Action)
	{
		int8_t Weight = ActionWeight(Action);
		int8_t LastWeight = 0;
		size_t LastActionIndex = 0;
		while (LastActionIndex < m_Calculations.Size())
		{
			size_t RIndex = m_Calculations.Size() - 1 - LastActionIndex;
			if (m_Calculations[RIndex].m_Type == Calculation::EType::Action)
			{
				LastWeight = ActionWeight(m_Calculations[RIndex].m_Action);
				break;
			}
			LastActionIndex++;
		}

		if (m_Calculations.Size() > 0)
		{
			Calculation& Back = m_Calculations[m_Calculations.Size() - 1];
			if (LastWeight != 0 && Weight >= LastWeight && Back.m_Type == Calculation::EType::Priority)
			{
				Back.m_Type = Calculation::EType::Completed;
			}
			else if (Back.m_Type == EType::Calculation || Back.m_Type == EType::Priority)
			{
				Back.Action(Action);
				return;
			}
		}

		if (Weight < LastWeight || (Weight == LastWeight && Action == ECalculationAction::BitWise_Not || Action == ECalculationAction::Logical_Not))
		{
			Calculation NewCalc;
			NewCalc.m_Type = EType::Priority;

			size_t RIndex = (m_Calculations.Size() - 1 - LastActionIndex) + 1;
			size_t Offset = 0;
			while (RIndex + Offset < m_Calculations.Size())
			{
				NewCalc.m_Calculations.Insert(Offset, m_Calculations[RIndex + Offset]);
				Offset++;
			}
			m_Calculations.RemoveAt(RIndex, m_Calculations.Size());

			Calculation& NewAction = NewCalc.m_Calculations.EmplaceRef();
			NewAction.m_Type = EType::Action;
			NewAction.m_Action = Action;

			m_Calculations.EmplaceRef(NewCalc);
		}
		else
		{
			Calculation& NewCalc = m_Calculations.EmplaceRef();
			NewCalc.m_Type = EType::Action;
			NewCalc.m_Action = Action;
		}
	}

	int64_t Calculation::Solve() const
	{
		switch (m_Type)
		{
		default:
		case Calculation::EType::Action:
			return 0;
		case Calculation::EType::Value:
			return m_Value;
		case Calculation::EType::Calculation:
		case Calculation::EType::Priority:
		case Calculation::EType::Completed:
		{
			int64_t Result = 0;
			ECalculationAction LastAction = ECalculationAction::Undefined;
			bool SkipCondition = false;
			for (auto& It : m_Calculations)
			{
				switch (It.m_Type)
				{
				case Calculation::EType::Action:
					LastAction = It.m_Action;
					break;
				case Calculation::EType::Value:
				case Calculation::EType::Calculation:
				case Calculation::EType::Priority:
				case Calculation::EType::Completed:
					if (LastAction != ECalculationAction::Undefined)
					{
						switch (LastAction)
						{
						case ECalculationAction::Add:
							Result = Result + It.Solve();
							break;
						case ECalculationAction::Subtract:
							Result = Result - It.Solve();
							break;
						case ECalculationAction::Multiply:
							Result = Result * It.Solve();
							break;
						case ECalculationAction::Divide:
							Result = Result / It.Solve();
							break;
						case ECalculationAction::Modulo:
							Result = Result % It.Solve();
							break;
						case ECalculationAction::BitWise_And:
							Result = Result & It.Solve();
							break;
						case ECalculationAction::BitWise_Or:
							Result = Result | It.Solve();
							break;
						case ECalculationAction::BitWise_XOr:
							Result = Result ^ It.Solve();
							break;
						case ECalculationAction::BitWise_Not:
							Result = ~It.Solve();
							break;
						case ECalculationAction::BitWise_LeftShift:
							Result = Result << It.Solve();
							break;
						case ECalculationAction::BitWise_RightShift:
							Result = Result >> It.Solve();
							break;
						case ECalculationAction::Equal:
							Result = Result == It.Solve();
							break;
						case ECalculationAction::NotEqual:
							Result = Result != It.Solve();
							break;
						case ECalculationAction::GreaterThan:
							Result = Result > It.Solve();
							break;
						case ECalculationAction::LessThan:
							Result = Result < It.Solve();
							break;
						case ECalculationAction::GreaterThanEqual:
							Result = Result >= It.Solve();
							break;
						case ECalculationAction::LessThanEqual:
							Result = Result <= It.Solve();
							break;
						case ECalculationAction::Logical_Not:
							Result = Result + !It.Solve();
							break;
						case ECalculationAction::Logical_And:
							Result = Result && It.Solve();
							break;
						case ECalculationAction::Logical_Or:
							Result = Result || It.Solve();
							break;
						case ECalculationAction::Condition_Question:
							SkipCondition = Result;
							Result = SkipCondition ? It.Solve() : 0;
							break;
						case ECalculationAction::Condition_Decission:
							if (!SkipCondition)
							{
								Result = It.Solve();
							}
							SkipCondition = false;
							break;
						}
					}
					else
					{
						Result = It.Solve();
					}
					break;
				}
			}
			return Result;
		}
		}

		return 0;
	}

	String Calculation::Format() const
	{
		String Result;
		Format(Result);
		return Result;
	}

	void Calculation::Format(String& Result) const
	{
		switch (m_Type)
		{
		case Calculation::EType::Calculation:
		case Calculation::EType::Priority:
		case Calculation::EType::Completed:
		{
			Result += L"(";
			bool IsFirst = true;
			for (auto& It : m_Calculations)
			{
				if (IsFirst)
				{
					IsFirst = false;
				}
				else
				{
					Result += L" ";
				}
				It.Format(Result);
			}
			Result += L")";
			break;
		}
		case Calculation::EType::Value:
			Result += std::to_wstring(m_Value);
			break;
		case Calculation::EType::Action:
			switch (m_Action)
			{
			case ECalculationAction::Undefined:
				Result += L"U";
				break;
			case ECalculationAction::Add:
				Result += L"+";
				break;
			case ECalculationAction::Subtract:
				Result += L"-";
				break;
			case ECalculationAction::Multiply:
				Result += L"*";
				break;
			case ECalculationAction::Divide:
				Result += L"/";
				break;
			case ECalculationAction::Modulo:
				Result += L"%";
				break;
			case ECalculationAction::BitWise_And:
				Result += L"&";
				break;
			case ECalculationAction::BitWise_Or:
				Result += L"|";
				break;
			case ECalculationAction::BitWise_XOr:
				Result += L"^";
				break;
			case ECalculationAction::BitWise_Not:
				Result += L"~";
				break;
			case ECalculationAction::BitWise_LeftShift:
				Result += L"<<";
				break;
			case ECalculationAction::BitWise_RightShift:
				Result += L">>";
				break;
			case ECalculationAction::Equal:
				Result += L"==";
				break;
			case ECalculationAction::NotEqual:
				Result += L"!=";
				break;
			case ECalculationAction::GreaterThan:
				Result += L">";
				break;
			case ECalculationAction::LessThan:
				Result += L"<";
				break;
			case ECalculationAction::GreaterThanEqual:
				Result += L">=";
				break;
			case ECalculationAction::LessThanEqual:
				Result += L"<=";
				break;
			case ECalculationAction::Logical_Not:
				Result += L"!";
				break;
			case ECalculationAction::Logical_And:
				Result += L"&&";
				break;
			case ECalculationAction::Logical_Or:
				Result += L"||";
				break;
			case ECalculationAction::Condition_Question:
				Result += L"?";
				break;
			case ECalculationAction::Condition_Decission:
				Result += L":";
				break;
			}
			break;
		}
	}
}