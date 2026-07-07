#pragma once
#include "Core.h"
#include "TextTokenizerInput.h"


namespace CE
{
	class CE_API TextTokenizerInput_String : public TextTokenizerInput
	{
	public:
		TextTokenizerInput_String(const String& String) : String(String) {}

		virtual WChar GetChar() override;

		virtual WChar PeekChar(size_t Offset = 0) override;


	public:
		const String& String;

	private:
		size_t m_Pos = 0;
	};
}