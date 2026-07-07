#pragma once
#include "Core.h"
#include "TextTokenizer.h"


namespace CE
{
	enum class ECalculationAction
	{
		Undefined,

		Add,
		Subtract,
		Multiply,
		Divide,
		Modulo,

		BitWise_And,
		BitWise_Or,
		BitWise_XOr,
		BitWise_Not,
		BitWise_LeftShift,
		BitWise_RightShift,

		Equal,
		NotEqual,
		GreaterThan,
		LessThan,
		GreaterThanEqual,
		LessThanEqual,

		Logical_Not,
		Logical_And,
		Logical_Or,

		Condition_Question,
		Condition_Decission,
	};


	class CE_API Calculation
	{
	public:
		static uint8 ActionWeight(ECalculationAction Action);


	public:
		void AddToken(const TextToken& Token);

		void Begin();

		void End();

		void Value(int64_t Value);

		void Action(ECalculationAction Action);

		int64_t Solve() const;

		String Format() const;


	private:
		void Format(String& Result) const;


	private:
		enum class EType
		{
			Calculation,
			Value,
			Action,
			Priority,
			Completed,
		};
		EType m_Type = EType::Calculation;
		int64 m_Value = 0;
		ECalculationAction m_Action = ECalculationAction::Undefined;
		Array<Calculation> m_Calculations;
	};
}