#include "TextTokenizerInput_Stream.h"


namespace CE
{
#if 0	// Currently Disabled because the PeekChar function will sometimes reset the position too far in front
	WChar TextTokenizerInput_Stream::GetChar()
	{
		WChar Result = '\0';
		Stream.read(&Result, 1);
		return Stream.bad() ? '\0' : Result;
	}

	WChar TextTokenizerInput_Stream::PeekChar(size_t Offset /*= 0*/)
	{
		WChar Result = '\0';
		size_t Current = Stream.tellg();
		Stream.seekg(0, std::ios::end);
		size_t Last = Stream.tellg();
		if (Current + Offset < Last)
		{
			Stream.seekg(Current + Offset);
			Stream.read(&Result, 1);
		}
		Stream.seekg(Current);
		return Stream.bad() ? '\0' : Result;
	}
#endif
}