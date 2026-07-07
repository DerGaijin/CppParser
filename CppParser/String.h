#pragma once
#include "Definitions.h"
#include "Array.h"

#include <string>

namespace CE
{
	typedef char AChar;
	typedef wchar_t WChar;

	class CE_API String
	{
		using Allocator = std::allocator<WChar>;
		using AllocTraits = std::allocator_traits<Allocator>;

	public:
		static constexpr size_t NPos = -1;

		template<typename... FmtArgs>
		static String Format(const WChar* Fmt, FmtArgs... Args)
		{
			size_t RequiredSize = (size_t)swprintf(nullptr, 0, Fmt, Args...) + 1;
			if (RequiredSize <= 0)
			{
				return Fmt;
			}

			String Result;
			Result.Clear(RequiredSize);
			swprintf(Result.Data(), RequiredSize, Fmt, Args...);
			Result.m_Size = RequiredSize - 1;	// Remove the null terminator added by sPrintf
			return Result;
		}


	public:
		String() = default;

		String(const String& Copy);

		String(String&& Move) noexcept;

		String(const WChar* Raw);

		String(const AChar* Raw);

		String(const WChar* Raw, size_t RawSize);

		String(const AChar* Raw, size_t RawSize);

		String(const std::wstring& Std);

		String(const std::string& Std);

		String(const WChar& Char, size_t Count = 1);

		String(const AChar& Char, size_t Count = 1);

		~String();

	public:
		void Clear(size_t Reserve = 0);

		void Reserve(size_t Reserve);

		inline void Shrink()
		{
			Reserve(m_Size);
		}


		void Append(const String& Str);

		void Append(const WChar* Raw);

		void Append(const AChar* Raw);

		void Append(const WChar* Raw, size_t RawSize);

		void Append(const AChar* Raw, size_t RawSize);

		void Append(const std::wstring& Std);

		void Append(const std::string& Std);

		void Append(const WChar& Char, size_t Count = 1);

		void Append(const AChar& Char, size_t Count = 1);


		void Insert(size_t At, const String& Str);

		void Insert(size_t At, const WChar* Raw);

		void Insert(size_t At, const AChar* Raw);

		void Insert(size_t At, const WChar* Raw, size_t RawSize);

		void Insert(size_t At, const AChar* Raw, size_t RawSize);

		void Insert(size_t At, const std::wstring& Std);

		void Insert(size_t At, const std::string& Std);

		void Insert(size_t At, const WChar& Char, size_t Count = 1);

		void Insert(size_t At, const AChar& Char, size_t Count = 1);


		void RemoveAt(size_t At, size_t Count = 1);

		size_t Replace(const String& ToReplace, const String& With, size_t Count = 1);

		inline size_t ReplaceAll(const String& ToReplace, const String& With)
		{
			return Replace(std::forward<const String&>(ToReplace), std::forward<const String&>(With), m_Size);
		}

		void ToUpper();

		void ToLower();

		void LTrim();

		void RTrim();

		inline void Trim()
		{
			LTrim();
			RTrim();
		}


	public:
		size_t Size() const;

		size_t Reserved() const;

		WChar* Data();

		const WChar* Data() const;

		const WChar* begin() const;

		WChar* begin();

		const WChar* end() const;

		WChar* end();

		bool IsValidIndex(size_t Index) const;

		String SubString(size_t Offset) const;

		String SubString(size_t Offset, size_t Count) const;

		size_t Find(const String& Str, size_t Offset = 0) const;

		size_t Find(const String& Str, size_t Offset, size_t Count) const;

		size_t Find(const WChar* Raw, size_t Offset = 0) const;

		size_t Find(const WChar* Raw, size_t Offset, size_t Count) const;

		size_t Find(const WChar* Raw, size_t RawSize, size_t Offset, size_t Count) const;

		bool StartsWith(const String& Str) const;

		bool StartsWith(const WChar* Raw) const;

		bool StartsWith(const WChar* Raw, size_t RawSize) const;

		bool StartsWith(const WChar& Char) const;

		bool EndsWith(const String& Str) const;

		bool EndsWith(const WChar* Raw) const;

		bool EndsWith(const WChar* Raw, size_t RawSize) const;

		bool EndsWith(const WChar& Char) const;

		// As Bool
		// As Integral
		// As Unsigned Integral
		// As Float


	public:
		const WChar& operator[](size_t Index) const;

		WChar& operator[](size_t Index);

		operator std::wstring() const
		{
			return std::wstring(begin(), end());
		}

		String& operator=(const String& Copy);

		String& operator=(String&& Move) noexcept;

		String& operator=(const WChar* Raw);

		String& operator=(const AChar* Raw);

		String& operator=(const std::wstring& Std);

		String& operator=(const std::string& Std);

		String& operator=(const WChar& Char);

		String& operator=(const AChar& Char);

		String& operator+=(const String& Str);

		String& operator+=(const WChar* Raw);

		String& operator+=(const AChar* Raw);

		String& operator+=(const std::wstring& Std);

		String& operator+=(const std::string& Std);

		String& operator+=(const WChar& Char);

		String& operator+=(const AChar& Char);

		String operator+(const String& Str) const;

		String operator+(const WChar* Raw) const;

		String operator+(const AChar* Raw) const;

		String operator+(const std::wstring& Std) const;

		String operator+(const std::string& Std) const;

		String operator+(const WChar& Char) const;

		String operator+(const AChar& Char) const;

		bool operator==(const String& Str) const;

		bool operator==(const WChar* Raw) const;

		bool operator==(const AChar* Raw) const;

		bool operator==(const std::wstring& Std) const;

		bool operator==(const std::string& Std) const;

		bool operator==(const WChar& Char) const;

		bool operator==(const AChar& Char) const;

		inline bool operator!=(const String& Str) const
		{
			return !(*this == Str);
		}

		inline bool operator!=(const WChar* Raw) const
		{
			return !(*this == Raw);
		}

		inline bool operator!=(const AChar* Raw) const
		{
			return !(*this == Raw);
		}

		inline bool operator!=(const std::wstring& Std) const
		{
			return !(*this == Std);
		}

		inline bool operator!=(const std::string& Std) const
		{
			return !(*this == Std);
		}

		inline bool operator!=(const WChar& Char) const
		{
			return !(*this == Char);
		}

		inline bool operator!=(const AChar& Char) const
		{
			return !(*this == Char);
		}

		bool operator<(const String& Str) const;

		bool operator<(const WChar* Raw) const;

		operator std::string() const
		{
			std::string Result;
			Result.reserve(Size());
			for (auto& C : *this)
			{
				Result += (AChar)C;
			}
			return Result;
		}

	private:
		size_t CalculateReserve(size_t MinReserved) const;

		WChar* InsertData(size_t At, size_t Count);

		WChar* GetDataPtr() const;

	private:
		static constexpr size_t SmallBuffSize = 16 / sizeof(WChar) < 1 ? 1 : 16 / sizeof(WChar);

		union
		{
			WChar m_Small[SmallBuffSize] = { 0 };
			WChar* m_Data;
		};
		size_t m_Size = 0;
		size_t m_Reserved = SmallBuffSize - 1;
		Allocator m_Allocator;
	};


	CE_API String operator+(const WChar* Raw, const String& StrRight);
	CE_API String operator+(const AChar* Raw, const String& StrRight);
	CE_API String operator+(const std::wstring& Std, const String& StrRight);
	CE_API String operator+(const std::string& Std, const String& StrRight);
	CE_API String operator+(const WChar& Char, const String& StrRight);
	CE_API String operator+(const AChar& Char, const String& StrRight);
	CE_API bool operator==(const WChar* Raw, const String& StrRight);
	CE_API bool operator==(const AChar* Raw, const String& StrRight);
	CE_API bool operator==(const std::wstring& Std, const String& StrRight);
	CE_API bool operator==(const std::string& Std, const String& StrRight);
	CE_API bool operator==(const WChar& Char, const String& StrRight);
	CE_API bool operator==(const AChar& Char, const String& StrRight);
	CE_API bool operator!=(const WChar* Raw, const String& StrRight);
	CE_API bool operator!=(const AChar* Raw, const String& StrRight);
	CE_API bool operator!=(const std::wstring& Std, const String& StrRight);
	CE_API bool operator!=(const std::string& Std, const String& StrRight);
	CE_API bool operator!=(const WChar& Char, const String& StrRight);
	CE_API bool operator!=(const AChar& Char, const String& StrRight);
}


namespace std
{
	template<>
	struct hash<CE::String>
	{
		[[nodiscard]] size_t operator()(const CE::String& _Val) const noexcept
		{
			return _Hash_array_representation(_Val.Data(), _Val.Size());
		}
	};
}

#define TEXT(X) L##X