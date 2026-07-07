#pragma once
#include "Core.h"


namespace CE
{
	class CE_API TextTokenizerInput
	{
	public:
		virtual ~TextTokenizerInput() = default;

		virtual WChar GetChar() = 0;

		virtual WChar PeekChar(size_t Offset = 0) = 0;
	};
}
