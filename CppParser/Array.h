#pragma once
#include "Definitions.h"

#include <memory>
#include <initializer_list>

namespace CE
{
	template<typename T, typename Alloc = std::allocator<T>>
	class Array
	{
	public:
		typedef size_t SizeType;
		typedef T ElementType;
		typedef Alloc AllocatorType;
		typedef std::allocator_traits<AllocatorType> AllocatorTraits;

	public:
		// Default Constructor
		Array() = default;

		// Copy Constructor
		Array(const Array& Other)
		{
			*this = std::forward<const Array&>(Other);
		}

		// Move Constructor
		Array(Array&& Other) noexcept
		{
			*this = std::forward<Array&&>(Other);
		}

		// Initializer list Constructor
		Array(const std::initializer_list<ElementType>& InitList)
		{
			*this = std::forward<const std::initializer_list<ElementType>&>(InitList);
		}

		// Copy Operator
		Array& operator=(const Array& Other)
		{
			Clear(Other.m_Reserved);
			if (Other.m_Size > 0)
			{
				AddUninitialized(Other.m_Size);

				ElementType* It1 = begin();
				const ElementType* It2 = Other.begin();
				while (It1 != end() && It2 != Other.end())
				{
					AllocatorTraits::construct(m_Allocator, It1, *It2);

					It1++;
					It2++;
				}
			}

			return *this;
		}

		// Move Operator
		Array& operator=(Array&& Other) noexcept
		{
			std::swap(m_Size, Other.m_Size);
			std::swap(m_Reserved, Other.m_Reserved);
			std::swap(m_Data, Other.m_Data);
			std::swap(m_Allocator, Other.m_Allocator);
			Other.Clear();

			return *this;
		}

		// Initializer list Operator
		Array& operator=(const std::initializer_list<ElementType>& InitList)
		{
			Clear(InitList.size());
			Insert(0, std::forward<const std::initializer_list<ElementType>&>(InitList));
			return *this;
		}

		// Destructor
		~Array()
		{
			if (m_Data != nullptr)
			{
				for (ElementType* Item = m_Data; Item != m_Data + m_Size; Item++)
				{
					AllocatorTraits::destroy(m_Allocator, Item);
				}
				AllocatorTraits::deallocate(m_Allocator, m_Data, m_Reserved);
			}
		}

	public:
		// Clears the Array and reserves memory
		void Clear(SizeType Reserve = 0)
		{
			for (ElementType* Item = m_Data; Item != m_Data + m_Size; Item++)
			{
				AllocatorTraits::destroy(m_Allocator, Item);
			}
			m_Size = 0;
			if (m_Reserved != Reserve)
			{
				AllocatorTraits::deallocate(m_Allocator, m_Data, m_Reserved);
				m_Reserved = Reserve;
				if (m_Reserved > 0)
				{
					m_Data = AllocatorTraits::allocate(m_Allocator, m_Reserved);
				}
				else
				{
					m_Data = nullptr;
				}
			}
		}

		// Reserved memory
		void Reserve(SizeType Reserve)
		{
			if (Reserve > m_Reserved)
			{
				SizeType Size = m_Size;
				InsertData(m_Size, Reserve - m_Size);
				m_Size = Size;
			}
			else if (Reserve < m_Reserved)
			{
				ShrinkTo(Reserve);
			}
		}

		// Shrinks the memory to fit the Size
		void Shrink()
		{
			if (m_Reserved > m_Size)
			{
				ShrinkTo(m_Size);
			}
		}

		// Adds Elements without Initialization
		void AddUninitialized(SizeType Count = 1)
		{
			InsertUninitialized(m_Size, Count);
		}

		// Adds Elements and default constructs
		void AddDefaulted(SizeType Count = 1)
		{
			InsertDefaulted(m_Size, Count);
		}

		// Adds Elements and zeroes the memory
		void AddZeroed(SizeType Count = 1)
		{
			InsertZeroed(m_Size, Count);
		}

		// Adds Item and Copies it
		void Add(const ElementType& Item)
		{
			Insert(m_Size, std::forward<const ElementType&>(Item));
		}

		// Adds Item and Moves it
		void Add(ElementType&& Item)
		{
			Insert(m_Size, std::forward<ElementType&&>(Item));
		}

		// Adds Elements
		void Add(const std::initializer_list<ElementType>& InitList)
		{
			Insert(m_Size, std::forward<const std::initializer_list<ElementType>&>(InitList));
		}

		// Adds Elements
		void Add(const Array<ElementType>& Elements)
		{
			Insert(m_Size, std::forward<const Array<ElementType>&>(Elements));
		}

		// Emplaces Element
		template<typename... ARGS>
		void Emplace(ARGS&&... Args)
		{
			EmplaceAtRef(m_Size, std::forward<ARGS&&>(Args)...);
		}

		// Emplaces Element at Index
		template<typename... ARGS>
		void EmplaceAt(SizeType Index, ARGS&&... Args)
		{
			EmplaceAtRef(Index, std::forward<ARGS&&>(Args)...);
		}

		// Emplaces Element and returns a Reference
		template<typename... ARGS>
		ElementType& EmplaceRef(ARGS&&... Args)
		{
			return EmplaceAtRef(m_Size, std::forward<ARGS&&>(Args)...);
		}

		// Emplaces Element at Index and returns a Reference
		template<typename... ARGS>
		ElementType& EmplaceAtRef(SizeType Index, ARGS&&... Args)
		{
			ElementType* Added = InsertData(Index, 1);
			CE_CHECK(Added != nullptr);
			AllocatorTraits::construct(m_Allocator, Added, std::forward<ARGS&&>(Args)...);
			return *Added;
		}

		// Adds a Element if it is not found
		bool AddUnique(const ElementType& Item)
		{
			if (!Contains(Item))
			{
				Add(Item);
				return true;
			}
			return false;
		}

		// Adds a Element if it is not found
		bool AddUnique(ElementType&& Item)
		{
			if (!Contains(Item))
			{
				Add(Item);
				return true;
			}
			return false;
		}

		// Inserts Elements at Index without initialization and returns a Reference
		ElementType& InsertUninitializedRef(SizeType Index, SizeType Count = 1)
		{
			ElementType* Added = InsertData(Index, Count);
			CE_CHECK(Added != nullptr);
			return *Added;
		}

		// Inserts Element and default constructs at Index and returns a Reference
		ElementType& InsertDefaultedRef(SizeType Index, SizeType Count = 1)
		{
			ElementType* Added = InsertData(Index, Count);
			CE_CHECK(Added != nullptr);
			for (ElementType* Item = Added; Item != Added + Count; Item++)
			{
				AllocatorTraits::construct(m_Allocator, Item);
			}
			return *Added;
		}

		// Inserts Element and zeroes the memory at Index and returns a Reference
		ElementType& InsertZeroedRef(SizeType Index, SizeType Count = 1)
		{
			ElementType* Added = InsertData(Index, Count);
			CE_CHECK(Added != nullptr);
			memset(Added, 0, Count * sizeof(ElementType));
			return *Added;
		}

		// Inserts Element and copies it at Index and returns a Reference
		ElementType& InsertRef(SizeType Index, const ElementType& Item)
		{
			ElementType* Added = InsertData(Index, 1);
			CE_CHECK(Added != nullptr);
			AllocatorTraits::construct(m_Allocator, Added, Item);
			return *Added;
		}

		// Inserts Element and moves it at Index and returns a Reference
		ElementType& InsertRef(SizeType Index, ElementType&& Item)
		{
			ElementType* Added = InsertData(Index, 1);
			CE_CHECK(Added != nullptr);
			AllocatorTraits::construct(m_Allocator, Added, std::move(Item));
			return *Added;
		}

		// Inserts Elements at Index without initialization
		void InsertUninitialized(SizeType Index, SizeType Count = 1)
		{
			InsertUninitializedRef(Index, Count);
		}

		// Inserts Element and default constructs at Index
		void InsertDefaulted(SizeType Index, SizeType Count = 1)
		{
			InsertDefaultedRef(Index, Count);
		}

		// Inserts Element and zeroes the memory at Index
		void InsertZeroed(SizeType Index, SizeType Count = 1)
		{
			InsertZeroedRef(Index, Count);
		}

		// Inserts Element and copies it at Index
		void Insert(SizeType Index, const ElementType& Item)
		{
			InsertRef(Index, std::forward<const ElementType&>(Item));
		}

		// Inserts Element and copies it at Index
		void Insert(SizeType Index, const ElementType& Item, SizeType Count)
		{
			ElementType* Added = InsertData(Index, Count);
			CE_CHECK(Added != nullptr);
			for (ElementType* AddedItem = Added; AddedItem != Added + Count; AddedItem++)
			{
				AllocatorTraits::construct(m_Allocator, AddedItem, Item);
			}
		}

		// Inserts Element and moves it at Index
		void Insert(SizeType Index, ElementType&& Item)
		{
			InsertRef(Index, std::forward<ElementType&&>(Item));
		}

		// Inserts Elements at Index
		void Insert(SizeType Index, const Array<ElementType>& Items)
		{
			if (Items.Size() > 0)
			{
				ElementType* Added = InsertData(Index, Items.Size());
				CE_CHECK(Added != nullptr);
				for (const ElementType& Item : Items)
				{
					AllocatorTraits::construct(m_Allocator, Added, Item);
					Added++;
				}
			}
		}

		// Inserts Elements at Index
		void Insert(SizeType Index, const std::initializer_list<ElementType>& InitList)
		{
			if (InitList.size() > 0)
			{
				ElementType* Added = InsertData(Index, InitList.size());
				CE_CHECK(Added != nullptr);
				for (const ElementType& Item : InitList)
				{
					AllocatorTraits::construct(m_Allocator, Added, Item);
					Added++;
				}
			}
		}

		// Removes Elements at Index
		void RemoveAt(SizeType Index, SizeType Count = 1)
		{
			if (IsValidIndex(Index))
			{
				if (Count > m_Size - Index)
				{
					Count = m_Size - Index;
				}
				if (Count > 0)
				{
					for (ElementType* Item = m_Data + Index; Item != m_Data + Index + Count; Item++)
					{
						AllocatorTraits::destroy(m_Allocator, Item);
					}

					ElementType* From = m_Data + Index + Count;
					ElementType* To = m_Data + Index;
					while (To != m_Data + Index + (m_Size - Index - Count))
					{
						AllocatorTraits::construct(m_Allocator, To, std::move(*From));
						AllocatorTraits::destroy(m_Allocator, From);
						From++;
						To++;
					}
					m_Size -= Count;
				}
			}
		}

		// Removes Elements that matches by Predicate
		template<typename Predicate = bool(*)(const ElementType&)>
		SizeType RemoveByPredicate(Predicate Pred)
		{
			return RemoveByPredicate(Pred, m_Size, 0, m_Size);
		}

		// Removes Elements that matches by Predicate with Count
		template<typename Predicate = bool(*)(const ElementType&)>
		SizeType RemoveByPredicate(Predicate Pred, SizeType RemoveCount)
		{
			return RemoveByPredicate(Pred, RemoveCount, 0, m_Size);
		}

		// Removes Elements that matches by Predicate with Count and Offset
		template<typename Predicate = bool(*)(const ElementType&)>
		SizeType RemoveByPredicate(Predicate Pred, SizeType RemoveCount, SizeType Offset)
		{
			return RemoveByPredicate(Pred, RemoveCount, Offset, m_Size);
		}

		// Removes Elements that matches by Predicate with RemoveCount and Offset
		template<typename Predicate = bool(*)(const ElementType&)>
		SizeType RemoveByPredicate(Predicate Pred, SizeType RemoveCount, SizeType Offset, SizeType Count)
		{
			if (IsValidIndex(Offset))
			{
				if (Count >= m_Size - Offset)
				{
					Count = (m_Size - 1) - Offset;
				}

				SizeType Idx = Offset + Count;
				SizeType ToRemove = 0;
				SizeType Removed = 0;
				while (true)
				{
					if (Pred((const ElementType&)*(m_Data + Idx)))
					{
						ToRemove++;
					}
					else if (ToRemove > 0)
					{
						RemoveAt(Idx + 1, ToRemove);
						Removed += ToRemove;
						ToRemove = 0;
					}
					if (Idx == Offset || Removed + ToRemove >= RemoveCount)
					{
						break;
					}
					Idx--;
				}
				if (ToRemove > 0)
				{
					RemoveAt(Idx, ToRemove);
					Removed += ToRemove;
				}
				return Removed;
			}
			return 0;
		}

		// Removes Elements that matches Compare
		template<typename TCompare>
		SizeType Remove(TCompare Compare, SizeType RemoveCount, SizeType Offset, SizeType Count)
		{
			return RemoveByPredicate([&](const ElementType& It) { return It == Compare; }, RemoveCount, Offset, Count);
		}

		// Removes Elements that matches Compare
		template<typename TCompare>
		SizeType Remove(TCompare Compare, SizeType RemoveCount, SizeType Offset = 0)
		{
			return RemoveByPredicate([&](const ElementType& It) { return It == Compare; }, RemoveCount, Offset, m_Size);
		}

		// Removes Elements that matches Compare
		template<typename TCompare>
		SizeType Remove(TCompare Compare)
		{
			return RemoveByPredicate([&](const ElementType& It) { return It == Compare; }, m_Size, 0, m_Size);
		}

	public:
		// Finds Element by Predicate
		template<typename Predicate = bool(*)(const ElementType&)>
		bool FindByPredicate(Predicate Pred, SizeType& FoundAt, SizeType BeginAt, SizeType EndAt) const
		{
			BeginAt = BeginAt < 0 ? 0 : (BeginAt > m_Size ? m_Size : BeginAt);
			EndAt = EndAt < 0 ? 0 : (EndAt > m_Size ? m_Size : EndAt);

			ElementType* Begin = m_Data + BeginAt;
			ElementType* End = m_Data + EndAt;

			for (const ElementType* Curr = m_Data + BeginAt; Curr != m_Data + EndAt;)
			{
				if (Pred(*Curr))
				{
					FoundAt = static_cast<SizeType>(Curr - m_Data);
					return true;
				}

				if (BeginAt < EndAt)
				{
					Curr++;
				}
				else
				{
					Curr--;
				}
			}
			return false;
		}

		// Finds Element by Predicate
		template<typename Predicate = bool(*)(const ElementType&)>
		bool FindByPredicate(Predicate Pred, SizeType& FoundAt, SizeType BeginAt) const
		{
			return FindByPredicate(Pred, FoundAt, BeginAt, m_Size);
		}

		// Finds Element by Predicate
		template<typename Predicate = bool(*)(const ElementType&)>
		bool FindByPredicate(Predicate Pred, SizeType& FoundAt) const
		{
			return FindByPredicate(Pred, FoundAt, 0, m_Size);
		}

		// Finds Element by CompareItem
		template<typename Comp>
		bool Find(const Comp& CompareItem, SizeType& FoundAt) const
		{
			return FindByPredicate([&](const ElementType& It) { return It == CompareItem; }, FoundAt, 0, m_Size);
		}

		// Returns true if Pred is contained
		template<typename Predicate = bool(*)(const ElementType&)>
		bool ContainsByPredicate(Predicate Pred) const
		{
			SizeType FoundAt = 0;
			return FindByPredicate(Pred, FoundAt);
		}

		// Returns true if CompareItem is contained
		template<typename Comp>
		bool Contains(const Comp& CompareItem) const
		{
			return ContainsByPredicate([&](const ElementType& It) { return It == CompareItem; });
		}

		// Filters Elements into a new Array
		template<typename Alloc = AllocatorType, typename Predicate = bool(*)(const ElementType&)>
		Array<ElementType, Alloc> FilterByPredicate(Predicate Pred) const
		{
			Array<ElementType, Alloc> FilterResults;
			for (const ElementType* Data = m_Data, *DataEnd = Data + m_Size; Data != DataEnd; ++Data)
			{
				if (Pred(*Data))
				{
					FilterResults.Add(*Data);
				}
			}
			return FilterResults;
		}

	public:
		// Returns the Size
		SizeType Size() const
		{
			return m_Size;
		}

		// Returns the Reserved Size
		SizeType Reserved() const
		{
			return m_Reserved;
		}

		// Returns the Data Pointer as const
		const ElementType* Data() const
		{
			return m_Data;
		}

		// Returns the Data Pointer
		ElementType* Data()
		{
			return m_Data;
		}

		// Returns true if Index is a valid Index
		bool IsValidIndex(SizeType Index) const
		{
			return Index >= 0 && Index < m_Size;
		}

	public:
		// Returns the Element at Index
		const ElementType& operator[](SizeType Index) const
		{
			CE_ASSERT(IsValidIndex(Index), L"Array Index out of bounds");
			return m_Data[Index];
		}

		// Returns the Element at Index
		ElementType& operator[](SizeType Index)
		{
			CE_ASSERT(IsValidIndex(Index), L"Array Index out of bounds");
			return m_Data[Index];
		}

		// Returns the begin Iterator
		ElementType* begin()
		{
			return m_Data;
		}

		// Returns the begin Iterator as const
		const ElementType* begin() const
		{
			return m_Data;
		}

		// Returns the end Iterator
		ElementType* end()
		{
			return m_Data + m_Size;
		}

		// Returns the end Iterator as const
		const ElementType* end() const
		{
			return m_Data + m_Size;
		}

		// Compares the Array Elements
		bool operator==(const Array& Other) const
		{
			if (m_Size != Other.m_Size)
			{
				return false;
			}

			ElementType* It1 = m_Data;
			ElementType* It2 = Other.m_Data;
			while (It1 != m_Data + m_Size && It2 != Other.m_Data + Other.m_Size)
			{
				if (!((*It1) == (*It2)))
				{
					return false;
				}
				It1++;
				It2++;
			}

			return true;
		}

	private:
		// Calculates the new Reserved Size
		SizeType CalcReserved(SizeType MinReserved) const
		{
			const SizeType Geometric = m_Reserved + m_Reserved / 2;
			if (Geometric > MinReserved)
			{
				return Geometric;
			}
			return MinReserved;
		}

		void ShrinkTo(SizeType NewReserved)
		{
			if (NewReserved < m_Size)
			{
				NewReserved = m_Size;
			}

			ElementType* NewData = AllocatorTraits::allocate(m_Allocator, NewReserved);
			for (SizeType Index = 0; Index < m_Size; Index++)
			{
				AllocatorTraits::construct(m_Allocator, NewData + Index, std::move(*(m_Data + Index)));
				AllocatorTraits::destroy(m_Allocator, m_Data + Index);
			}
			AllocatorTraits::deallocate(m_Allocator, m_Data, m_Reserved);

			m_Data = NewData;
			m_Reserved = NewReserved;
		}

		// Inserts data without initializing it
		ElementType* InsertData(SizeType Index, SizeType Count)
		{
			if (IsValidIndex(Index) || Index == m_Size)
			{
				if (m_Size + Count > m_Reserved)
				{
					SizeType NewReserved = CalcReserved(m_Size + Count);
					ElementType* NewData = AllocatorTraits::allocate(m_Allocator, NewReserved);
					if (m_Data != nullptr)
					{
						ElementType* From = m_Data;
						ElementType* To = NewData;
						while (From != m_Data + Index && To != NewData + Index)
						{
							AllocatorTraits::construct(m_Allocator, To, std::move(*From));
							AllocatorTraits::destroy(m_Allocator, From);
							From++;
							To++;
						}
						From = m_Data + Index;
						To = NewData + Index + Count;
						while (From != m_Data + m_Size && To != NewData + m_Size + Count)
						{
							AllocatorTraits::construct(m_Allocator, To, std::move(*From));
							AllocatorTraits::destroy(m_Allocator, From);
							From++;
							To++;
						}
						AllocatorTraits::deallocate(m_Allocator, m_Data, m_Reserved);
					}
					m_Data = NewData;
					m_Reserved = NewReserved;
				}
				else
				{
					ElementType* From = m_Data + (m_Size - 1);
					ElementType* To = m_Data + (m_Size - 1) + Count;
					while (From >= m_Data + Index && To >= m_Data + Index + Count)
					{
						AllocatorTraits::construct(m_Allocator, To, std::move(*From));
						AllocatorTraits::destroy(m_Allocator, From);
						From--;
						To--;
					}
				}
				m_Size += Count;
				return m_Data + Index;
			}
			return nullptr;
		}

	private:
		SizeType m_Size = 0;
		SizeType m_Reserved = 0;
		ElementType* m_Data = nullptr;
		AllocatorType m_Allocator;
	};
}
