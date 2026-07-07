#include "String.h"

namespace CE
{
	String::String(const String& Copy)
	{
		*this = std::forward<const String&>(Copy);
	}

	String::String(String&& Move) noexcept
	{
		*this = std::forward<String&&>(Move);
	}

	String::String(const WChar* Raw)
	{
		Insert(0, Raw);
	}

	String::String(const AChar* Raw)
	{
		Insert(0, Raw);
	}

	String::String(const WChar* Raw, size_t RawSize)
	{
		Insert(0, Raw, RawSize);
	}

	String::String(const AChar* Raw, size_t RawSize)
	{
		Insert(0, Raw, RawSize);
	}

	String::String(const std::wstring& Std)
	{
		Insert(0, Std);
	}

	String::String(const std::string& Std)
	{
		Insert(0, Std);
	}

	String::String(const WChar& Char, size_t Count /*= 1*/)
	{
		Insert(0, Char, Count);
	}

	String::String(const AChar& Char, size_t Count /*= 1*/)
	{
		Insert(0, Char, Count);
	}

	String::~String()
	{
		if (m_Reserved >= SmallBuffSize)
		{
			AllocTraits::deallocate(m_Allocator, GetDataPtr(), m_Reserved + 1);
		}
	}

	void String::Clear(size_t Reserve /*= 0*/)
	{
		m_Size = 0;
		if (Reserve != m_Reserved)
		{
			if (m_Reserved >= SmallBuffSize)
			{
				AllocTraits::deallocate(m_Allocator, GetDataPtr(), m_Reserved + 1);
			}
			m_Reserved = Reserve;
			if (m_Reserved >= SmallBuffSize)
			{
				m_Data = AllocTraits::allocate(m_Allocator, m_Reserved + 1);
			}
		}
		GetDataPtr()[0] = 0;
	}

	void String::Reserve(size_t Reserve)
	{
		if (m_Reserved != Reserve && Reserve >= m_Size)
		{
			if (Reserve >= SmallBuffSize)
			{
				WChar* NewData = AllocTraits::allocate(m_Allocator, Reserve + 1);
				if (m_Size > 0)
				{
					wmemmove(NewData, GetDataPtr(), m_Size + 1);
				}
				if (m_Reserved >= SmallBuffSize)
				{
					AllocTraits::deallocate(m_Allocator, GetDataPtr(), m_Reserved + 1);
				}
				m_Data = NewData;
				m_Reserved = Reserve;
				m_Data[m_Size] = 0;
			}
			else
			{
				if (m_Reserved >= SmallBuffSize)
				{
					AllocTraits::deallocate(m_Allocator, GetDataPtr(), m_Reserved + 1);
				}
				m_Reserved = SmallBuffSize - 1;
			}
		}
	}

	void String::Append(const String& Str)
	{
		Insert(m_Size, Str);
	}

	void String::Append(const WChar* Raw)
	{
		Insert(m_Size, Raw);
	}

	void String::Append(const AChar* Raw)
	{
		Insert(m_Size, Raw);
	}

	void String::Append(const WChar* Raw, size_t RawSize)
	{
		Insert(m_Size, Raw, RawSize);
	}

	void String::Append(const AChar* Raw, size_t RawSize)
	{
		Insert(m_Size, Raw, RawSize);
	}

	void String::Append(const std::wstring& Std)
	{
		Insert(m_Size, Std);
	}

	void String::Append(const std::string& Std)
	{
		Insert(m_Size, Std);
	}

	void String::Append(const WChar& Char, size_t Count /*= 1*/)
	{
		Insert(m_Size, Char, Count);
	}

	void String::Append(const AChar& Char, size_t Count /*= 1*/)
	{
		Insert(m_Size, Char, Count);
	}

	void String::Insert(size_t At, const String& Str)
	{
		Insert(At, Str.Data(), Str.Size());
	}

	void String::Insert(size_t At, const WChar* Raw)
	{
		Insert(At, Raw, wcslen(Raw));
	}

	void String::Insert(size_t At, const AChar* Raw)
	{
		Insert(At, Raw, strlen(Raw));
	}

	void String::Insert(size_t At, const WChar* Raw, size_t RawSize)
	{
		CE_CHECK(Raw != nullptr);
		WChar* Added = InsertData(At, RawSize);
		if (Added != nullptr)
		{
			wmemcpy(Added, Raw, RawSize);
		}
	}

	void String::Insert(size_t At, const AChar* Raw, size_t RawSize)
	{
		CE_CHECK(Raw != nullptr);
		WChar* Added = InsertData(At, RawSize);
		size_t Idx = 0;
		while (Idx < RawSize)
		{
			(*Added) = (WChar)Raw[Idx];
			Added++;
			Idx++;
		}
	}

	void String::Insert(size_t At, const std::wstring& Std)
	{
		Insert(At, Std.data(), Std.size());
	}

	void String::Insert(size_t At, const std::string& Std)
	{
		Insert(At, Std.data(), Std.size());
	}

	void String::Insert(size_t At, const WChar& Char, size_t Count /*= 1*/)
	{
		WChar* Added = InsertData(At, Count);
		if (Added != nullptr)
		{
			wmemset(Added, Char, Count);
		}
	}

	void String::Insert(size_t At, const AChar& Char, size_t Count /*= 1*/)
	{
		Insert(At, (WChar)Char, Count);
	}

	void String::RemoveAt(size_t At, size_t Count /*= 1*/)
	{
		if (IsValidIndex(At) && Count > 0)
		{
			if (Count > m_Size - At)
			{
				Count = m_Size - At;
			}
			wmemmove(GetDataPtr() + At, GetDataPtr() + At + Count, (m_Size - (At + Count)) + 1);
			m_Size -= Count;
		}
	}

	size_t String::Replace(const String& ToReplace, const String& With, size_t Count /*= 1*/)
	{
		if (Count == 0 || ToReplace.Size() == 0)
		{
			return 0;
		}

		size_t Replaced = 0;

		size_t Index = 0;
		while (Index < m_Size && Replaced < Count)
		{
			size_t FoundAt = Find(ToReplace, Index);
			if (FoundAt == NPos)
			{
				break;
			}

			if (ToReplace.Size() == With.Size())
			{
				wmemcpy(GetDataPtr() + FoundAt, With.GetDataPtr(), With.Size());
			}
			else if (ToReplace.Size() > With.Size())
			{
				RemoveAt(FoundAt, ToReplace.Size() - With.Size());
				wmemcpy(GetDataPtr() + FoundAt, With.GetDataPtr(), With.Size());
			}
			else
			{
				size_t Inserted = With.Size() - ToReplace.Size();
				Insert(FoundAt, With.Data(), Inserted);
				wmemcpy(GetDataPtr() + FoundAt + Inserted, With.GetDataPtr() + Inserted, With.Size() - Inserted);
			}

			Index = FoundAt + With.Size();
			Replaced++;
		}

		return Replaced;
	}

	void String::ToUpper()
	{
		for (auto& C : *this)
		{
			C = std::toupper(C);
		}
	}

	void String::ToLower()
	{
		for (auto& C : *this)
		{
			C = std::tolower(C);
		}
	}

	void String::LTrim()
	{
		size_t Index = 0;
		while (Index < m_Size)
		{
			if (!std::isspace(GetDataPtr()[Index]))
			{
				break;
			}
			Index++;
		}

		if (Index > 0)
		{
			RemoveAt(0, Index);
		}
	}

	void String::RTrim()
	{
		size_t Index = m_Size;
		while (Index > 0)
		{
			if (!std::isspace(GetDataPtr()[Index - 1]))
			{
				break;
			}
			Index--;
		}

		if (Index != m_Size)
		{
			RemoveAt(Index, m_Size - Index);
		}
	}

	size_t String::Size() const
	{
		return m_Size;
	}

	size_t String::Reserved() const
	{
		return m_Reserved;
	}

	WChar* String::Data()
	{
		return GetDataPtr();
	}

	const WChar* String::Data() const
	{
		return GetDataPtr();
	}

	bool String::IsValidIndex(size_t Index) const
	{
		return Index >= 0 && Index < m_Size;
	}

	String String::SubString(size_t Offset) const
	{
		if (!IsValidIndex(Offset))
		{
			return String();
		}
		return String(GetDataPtr() + Offset, m_Size - Offset);
	}

	String String::SubString(size_t Offset, size_t Count) const
	{
		if (!IsValidIndex(Offset))
		{
			return String();
		}
		return String(GetDataPtr() + Offset, Count > m_Size - Offset ? m_Size - Offset : Count);
	}

	size_t String::Find(const String& Str, size_t Offset /*= 0*/) const
	{
		return Find(Str.Data(), Str.Size(), Offset, m_Size);
	}

	size_t String::Find(const String& Str, size_t Offset, size_t Count) const
	{
		return Find(Str.Data(), Str.Size(), Offset, Count);
	}

	size_t String::Find(const WChar* Raw, size_t Offset /*= 0*/) const
	{
		return Find(Raw, wcslen(Raw), Offset, m_Size);
	}

	size_t String::Find(const WChar* Raw, size_t Offset, size_t Count) const
	{
		return Find(Raw, wcslen(Raw), Offset, Count);
	}

	size_t String::Find(const WChar* Raw, size_t RawSize, size_t Offset, size_t Count) const
	{
		if (Offset >= m_Size || RawSize == 0 || RawSize >= m_Size)
		{
			return NPos;
		}

		const WChar* Found = wcsstr(GetDataPtr() + Offset, Raw);
		if (Found != nullptr)
		{
			size_t FoundIdx = Found - GetDataPtr();
			return FoundIdx < Offset + Count ? FoundIdx : NPos;
		}

		return NPos;
	}

	bool String::StartsWith(const String& Str) const
	{
		return StartsWith(Str.Data(), Str.Size());
	}

	bool String::StartsWith(const WChar* Raw) const
	{
		return StartsWith(Raw, wcslen(Raw));
	}

	bool String::StartsWith(const WChar* Raw, size_t RawSize) const
	{
		return m_Size >= RawSize && wmemcmp(GetDataPtr(), Raw, RawSize) == 0;
	}

	bool String::StartsWith(const WChar& Char) const
	{
		return m_Size > 0 && GetDataPtr()[0] == Char;
	}

	bool String::EndsWith(const String& Str) const
	{
		return EndsWith(Str.Data(), Str.Size());
	}

	bool String::EndsWith(const WChar* Raw) const
	{
		return EndsWith(Raw, wcslen(Raw));
	}

	bool String::EndsWith(const WChar* Raw, size_t RawSize) const
	{
		size_t Diff = m_Size - RawSize;
		return m_Size >= RawSize && wmemcmp(GetDataPtr() + Diff, Raw, RawSize) == 0;
	}

	bool String::EndsWith(const WChar& Char) const
	{
		return m_Size > 0 && GetDataPtr()[m_Size - 1] == Char;
	}

	const WChar& String::operator[](size_t Index) const
	{
		CE_ASSERT(IsValidIndex(Index), "String Index out of bounds");
		return GetDataPtr()[Index];
	}

	WChar& String::operator[](size_t Index)
	{
		CE_ASSERT(IsValidIndex(Index), "String Index out of bounds");
		return GetDataPtr()[Index];
	}

	const WChar* String::begin() const
	{
		return GetDataPtr();
	}

	WChar* String::begin()
	{
		return GetDataPtr();
	}

	const WChar* String::end() const
	{
		return GetDataPtr() + m_Size;
	}

	WChar* String::end()
	{
		return GetDataPtr() + m_Size;
	}

	String& String::operator=(const String& Copy)
	{
		Clear(Copy.Size());
		m_Allocator = Copy.m_Allocator;
		InsertData(0, Copy.Size());
		wmemcpy(GetDataPtr(), Copy.GetDataPtr(), Copy.m_Size);
		return *this;
	}

	String& String::operator=(String&& Move) noexcept
	{
		WChar Temp[SmallBuffSize] = { 0 };	// Do we need to swap it or can we copy it?
		wmemmove(Temp, m_Small, SmallBuffSize);
		wmemmove(m_Small, Move.m_Small, SmallBuffSize);
		wmemmove(Move.m_Small, Temp, SmallBuffSize);
		std::swap(m_Size, Move.m_Size);
		std::swap(m_Reserved, Move.m_Reserved);
		std::swap(m_Allocator, Move.m_Allocator);
		Move.Clear();	//@TODO: Is this necessary?
		return *this;
	}

	String& String::operator=(const WChar* Raw)
	{
		size_t RawLength = wcslen(Raw);
		Clear(RawLength);
		Insert(0, Raw, RawLength);
		return *this;
	}

	String& String::operator=(const AChar* Raw)
	{
		size_t RawLength = strlen(Raw);
		Clear(RawLength);
		Insert(0, Raw, RawLength);
		return *this;
	}

	String& String::operator=(const std::wstring& Std)
	{
		Clear(Std.size());
		Insert(0, Std);
		return *this;
	}

	String& String::operator=(const std::string& Std)
	{
		Clear(Std.size());
		Insert(0, Std);
		return *this;
	}

	String& String::operator=(const WChar& Char)
	{
		Clear(1);
		Insert(0, Char);
		return *this;
	}

	String& String::operator=(const AChar& Char)
	{
		Clear(1);
		Insert(0, Char);
		return *this;
	}

	String& String::operator+=(const String& Str)
	{
		Append(std::forward<const String&>(Str));
		return *this;
	}

	String& String::operator+=(const WChar* Raw)
	{
		Append(std::forward<const WChar*>(Raw));
		return *this;
	}

	String& String::operator+=(const AChar* Raw)
	{
		Append(std::forward<const AChar*>(Raw));
		return *this;
	}

	String& String::operator+=(const std::wstring& Std)
	{
		Append(std::forward<const std::wstring&>(Std));
		return *this;
	}

	String& String::operator+=(const std::string& Std)
	{
		Append(std::forward<const std::string&>(Std));
		return *this;
	}

	String& String::operator+=(const WChar& Char)
	{
		Append(std::forward<const WChar&>(Char));
		return *this;
	}

	String& String::operator+=(const AChar& Char)
	{
		Append(std::forward<const AChar&>(Char));
		return *this;
	}

	String String::operator+(const String& Str) const
	{
		return String(*this) += std::forward<const String&>(Str);
	}

	String String::operator+(const WChar* Raw) const
	{
		return String(*this) += std::forward<const WChar*>(Raw);
	}

	String String::operator+(const AChar* Raw) const
	{
		return String(*this) += std::forward<const AChar*>(Raw);
	}

	String String::operator+(const std::wstring& Std) const
	{
		return String(*this) += std::forward<const std::wstring&>(Std);
	}

	String String::operator+(const std::string& Std) const
	{
		return String(*this) += std::forward<const std::string&>(Std);
	}

	String String::operator+(const WChar& Char) const
	{
		return String(*this) += std::forward<const WChar&>(Char);
	}

	String String::operator+(const AChar& Char) const
	{
		return String(*this) += std::forward<const AChar&>(Char);
	}

	bool String::operator==(const String& Str) const
	{
		return Str.Size() == m_Size && wmemcmp(GetDataPtr(), Str.Data(), m_Size) == 0;
	}

	bool String::operator==(const WChar* Raw) const
	{
		return wcslen(Raw) == m_Size && wmemcmp(GetDataPtr(), Raw, m_Size) == 0;
	}

	bool String::operator==(const AChar* Raw) const
	{
		if (strlen(Raw) != m_Size)
		{
			return false;
		}
		for (auto& C : *this)
		{
			if (C != (*Raw))
			{
				return false;
			}
			Raw++;
		}
		return true;
	}

	bool String::operator==(const std::wstring& Std) const
	{
		return Std.size() == m_Size && wmemcmp(GetDataPtr(), Std.data(), m_Size) == 0;
	}

	bool String::operator==(const std::string& Std) const
	{
		if (Std.size() != m_Size)
		{
			return false;
		}
		const AChar* Data = Std.data();
		for (auto& C : *this)
		{
			if (C != (*Data))
			{
				return false;
			}
			Data++;
		}
		return true;
	}

	bool String::operator==(const WChar& Char) const
	{
		return m_Size == 1 && GetDataPtr()[0] == Char;
	}

	bool String::operator==(const AChar& Char) const
	{
		return m_Size == 1 && GetDataPtr()[0] == Char;
	}

	bool String::operator<(const String& Str) const
	{
		if (Str.m_Size == m_Size)
		{
			return wmemcmp(GetDataPtr(), Str.GetDataPtr(), m_Size) < 0;
		}
		return Str.m_Size < m_Size;
	}

	bool String::operator<(const WChar* Raw) const
	{
		const size_t Size = wcslen(Raw);
		if (Size == m_Size)
		{
			return wmemcmp(GetDataPtr(), Raw, Size) < 0;
		}
		return Size < m_Size;
	}

	size_t String::CalculateReserve(size_t MinReserved) const
	{
		static constexpr size_t AllocMask = sizeof(WChar) <= 1 ? 15 : sizeof(WChar) <= 2 ? 7 : sizeof(WChar) <= 4 ? 3 : sizeof(WChar) <= 8 ? 1 : 0;
		const size_t Mask = MinReserved | AllocMask;

		const size_t Geometric = std::max(Mask, m_Reserved + m_Reserved / 2);
		if (Geometric > MinReserved)
		{
			return Geometric;
		}
		return MinReserved;
	}

	WChar* String::InsertData(size_t At, size_t Count)
	{
		if (IsValidIndex(At) || At == m_Size)
		{
			if ((m_Size + Count) + 1 > SmallBuffSize)
			{
				if (m_Reserved < m_Size + Count)
				{
					size_t NewReserved = CalculateReserve(m_Size + Count);
					WChar* NewData = AllocTraits::allocate(m_Allocator, NewReserved + 1);
					if (m_Size > 0)
					{
						wmemmove(NewData, GetDataPtr(), At);
						wmemmove(NewData + At + Count, GetDataPtr() + At, (m_Size - At) + 1);
					}
					if (m_Reserved >= SmallBuffSize)
					{
						AllocTraits::deallocate(m_Allocator, GetDataPtr(), m_Reserved + 1);
					}
					m_Data = NewData;
					m_Reserved = NewReserved;
					m_Size += Count;
					m_Data[m_Size] = 0;
				}
				else
				{
					wmemmove(GetDataPtr() + At + Count, GetDataPtr() + At, (m_Size - At) + 1);
					m_Size += Count;
				}
			}
			else
			{
				wmemmove(GetDataPtr() + At + Count, GetDataPtr() + At, (m_Size - At) + 1);
				m_Size += Count;
			}
			return GetDataPtr() + At;
		}
		return nullptr;
	}

	WChar* String::GetDataPtr() const
	{
		if (m_Reserved < SmallBuffSize)
		{
			return (WChar*)m_Small;
		}
		else
		{
			return m_Data;
		}
	}

	CE_API String operator+(const WChar* Raw, const String& StrRight)
	{
		return String(Raw) += StrRight;
	}

	CE_API String operator+(const AChar* Raw, const String& StrRight)
	{
		return String(Raw) += StrRight;
	}

	CE_API String operator+(const std::wstring& Std, const String& StrRight)
	{
		return String(Std) += StrRight;
	}

	CE_API String operator+(const std::string& Std, const String& StrRight)
	{
		return String(Std) += StrRight;
	}

	CE_API String operator+(const WChar& Char, const String& StrRight)
	{
		return String(Char) += StrRight;
	}

	CE_API String operator+(const AChar& Char, const String& StrRight)
	{
		return String(Char) += StrRight;
	}

	CE_API bool operator==(const WChar* Raw, const String& StrRight)
	{
		return StrRight == Raw;
	}

	CE_API bool operator==(const AChar* Raw, const String& StrRight)
	{
		return StrRight == Raw;
	}

	CE_API bool operator==(const std::wstring& Std, const String& StrRight)
	{
		return StrRight == Std;
	}

	CE_API bool operator==(const std::string& Std, const String& StrRight)
	{
		return StrRight == Std;
	}

	CE_API bool operator==(const WChar& Char, const String& StrRight)
	{
		return StrRight == Char;
	}

	CE_API bool operator==(const AChar& Char, const String& StrRight)
	{
		return StrRight == Char;
	}

	CE_API bool operator!=(const WChar* Raw, const String& StrRight)
	{
		return StrRight != Raw;
	}

	CE_API bool operator!=(const AChar* Raw, const String& StrRight)
	{
		return StrRight != Raw;
	}

	CE_API bool operator!=(const std::wstring& Std, const String& StrRight)
	{
		return StrRight != Std;
	}

	CE_API bool operator!=(const std::string& Std, const String& StrRight)
	{
		return StrRight != Std;
	}

	CE_API bool operator!=(const WChar& Char, const String& StrRight)
	{
		return StrRight != Char;
	}

	CE_API bool operator!=(const AChar& Char, const String& StrRight)
	{
		return StrRight != Char;
	}
}