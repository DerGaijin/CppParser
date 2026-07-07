#pragma once
#include "Core.h"
#include "TextTokenizerInput.h"


namespace CE
{
#if 0	// Currently Disabled because the PeekChar function will sometimes reset the position too far in front
	class CE_API TextTokenizerInput_Stream : public TextTokenizerInput
	{
	public:
		TextTokenizerInput_Stream(std::wistream& Stream) : Stream(Stream) {}

		virtual WChar GetChar() override;

		virtual WChar PeekChar(size_t Offset = 0) override;


	public:
		std::wistream& Stream;
	};
#endif
}