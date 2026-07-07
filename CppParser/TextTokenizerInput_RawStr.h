#pragma once
#include "Core.h"
#include "TextTokenizerInput.h"


namespace CE
{
	class CE_API TextTokenizerInput_RawStr : public TextTokenizerInput
	{
	public:
		TextTokenizerInput_RawStr(const WChar* RawStr, size_t RawStrSize) : RawStr(RawStr), RawStrSize(RawStrSize) {}

		virtual WChar GetChar() override;

		virtual WChar PeekChar(size_t Offset = 0) override;


	public:
		const WChar* RawStr;
		const size_t RawStrSize;


	private:
		size_t m_Pos = 0;
	};
}