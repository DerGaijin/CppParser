#include "TextTokenizerInput_String.h"


namespace CE
{
	WChar TextTokenizerInput_String::GetChar()
	{
		WChar C = m_Pos < String.Size() ? String[m_Pos] : '\0';
		m_Pos++;
		return C;
	}

	WChar TextTokenizerInput_String::PeekChar(size_t Offset /*= 0*/)
	{
		size_t Index = m_Pos + Offset;
		return Index < String.Size() ? String[Index] : '\0';
	}
}