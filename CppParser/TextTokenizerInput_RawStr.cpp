#include "TextTokenizerInput_RawStr.h"


namespace CE
{
	WChar TextTokenizerInput_RawStr::GetChar()
	{
		WChar C = m_Pos < RawStrSize ? RawStr[m_Pos] : '\0';
		m_Pos++;
		return C;
	}

	WChar TextTokenizerInput_RawStr::PeekChar(size_t Offset /*= 0*/)
	{
		size_t Index = m_Pos + Offset;
		return Index < RawStrSize ? RawStr[Index] : '\0';
	}
}