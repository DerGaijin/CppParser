#pragma once


#if defined(TEST_INCLUDES) || defined(TEST_ALL)
#include <string>
#include <vector>
#endif


#if defined(TEST_NAMESPACES) || defined(TEST_ALL)
namespace TestNamespace1 {}
namespace {}
inline namespace TestNamespace2 {}
inline namespace TestNamespace2 {}
namespace TestNamespace3 { namespace TestNamespace4 {} }
namespace TestNamespace5::TestNamespace6::TestNamespace7 {}
namespace TestNamespace8::inline TestNamespace9::inline TestNamespace10 {}
#endif


#if defined(TEST_NAMESPACE_ALIAS) || defined(TEST_ALL)
namespace TestNamespace10 = TestNamespace1;
namespace TestNamespace11 = ::TestNamespace1;
namespace TestNamespace12 = TestNamespace3::TestNamespace4;
#endif


#if defined(TEST_CLASS_STRUCTS) || defined(TEST_ALL)
class TestClass1
{
public:
protected:
private:
};
class __declspec(dllexport) TestClass2
{
public:
	friend class TestClass1;
};
class TestClass3;
struct
{

};
struct TestStruct1 final {};
struct alignas(2) TestStruct2 final {};
struct TestStruct3
{
protected:
};
struct TestStruct4 : public TestStruct3, private virtual TestClass2
{
};
#endif


#if defined(TEST_ENUMS) || defined(TEST_ALL)
enum class TestEnum1 : int
{
	Value01,
	Value02 = 1,
	Value03 = 1UL + 2 << 23,
};
enum TestEnum2 : short;
enum struct TestEnum3 : short {
	low = -1,
	high = 1,
};
enum OpaqueEnum : int;
enum class OpaqueScopedEnum : unsigned;
enum { AValue };
#endif


#if defined(TEST_VARIABLES) || defined(TEST_ALL)
class MemberTestClass1
{
public:
	const volatile std::basic_string<wchar_t> MemberVariable1;
	const volatile std::basic_string<wchar_t>* MemberVariable2;
	const volatile std::basic_string<wchar_t>& MemberVariable3;
	volatile const std::basic_string<wchar_t, int>** MemberVariable4;
	volatile const volatile std::basic_string<wchar_t, std::char_traits<wchar_t>>**& MemberVariable5;
	const const volatile std::basic_string<wchar_t>&& MemberVariable6;
	volatile const const volatile std::basic_string<wchar_t>**&& MemberVariable7;
	static volatile const const volatile std::basic_string<wchar_t>**&& MemberVariable8;
	volatile const static const volatile std::basic_string<wchar_t>**&& MemberVariable9;
	thread_local volatile const static const volatile std::basic_string<wchar_t> const* const volatile** const&& MemberVariable10;
	std::basic_string<wchar_t>::pointer MemberVariable11;
	class std::basic_string<wchar_t> MemberVariable12;
	class Unknown const* MemberVariable13;
	typename std::basic_string<wchar_t>::pointer MemberVariable14;
	constexpr static double MemberVariable15 = 0.0f;
	const class __declspec(dllexport) Testing : public TestClass2 {} MemberVariable16;
	int MemberVariable17 = 0, MemberVariable18 = 1, MemberVariable19 = 2;
	const const enum { AValue1, AValue2 } MemberVariable20 = AValue1;
	decltype(MemberTestClass1::MemberVariable17) MemberVariable21;
	static class Testing2 final
	{
	public:
		float Value;
	} MemberVariable22;
};
#endif


#if defined(TEST_FUNCTIONS) || defined(TEST_ALL)
class FunctionTestClass1
{
public:
	const volatile static std::basic_string<char, std::char_traits<char>>***&& MemberFunction1(const int**& Param1, const int**& Param2);
	const volatile virtual std::basic_string<char, std::char_traits<char>>***&& MemberFunction2(const int**& Param1, const int**& Param2) = 0;
	const volatile virtual virtual std::basic_string<char, std::char_traits<char>>***&& MemberFunction3(const int**& Param1, const int**& Param2) = 0;
	static const volatile std::basic_string<char, std::char_traits<char>>***&& MemberFunction4() noexcept;
	const volatile static std::basic_string<char, std::char_traits<char>>***&& MemberFunction5() noexcept {}
	const static volatile std::basic_string<char, std::char_traits<char>>***&& MemberFunction6() {}
	inline const static volatile std::basic_string<char, std::char_traits<char>>***&& MemberFunction7() {}
	void MemberFunction8(int Param1 = 0, float Param2 = 0.0f, std::vector<char> Param3 = { 1,2,3 }) {}
	void MemberFunction9(std::vector<char> = { 1,2,3,4,5 }) {}
	consteval auto MemberFunction10() const { return -1; }
	constexpr int MemberFunction11(const char* format, ...);
	void MemberFunction12(int values[3], char text[]);
	auto MemberFunction13(int value) -> decltype(value);
	int MemberFunction14(int value) try { return value; }
	catch (...) { return 0; }
};
#endif


#if defined(TEST_CONSTRUCTORS_DESTRUCTORS) || defined(TEST_ALL)
class ConDestructorTestClass1
{
public:
	ConDestructorTestClass1() = default;
	explicit ConDestructorTestClass1(float Value) noexcept : Value(Value) {}
	explicit(false) ConDestructorTestClass1(double value);
	virtual compl ConDestructorTestClass1() = default;
private:
	float Value;
};
class ConDestructorTestClass2
{
public:
	ConDestructorTestClass2() = delete;
	virtual ~ConDestructorTestClass2() {}
};
#endif


#if defined(TEST_FUNCTION_POINTERS) || defined(TEST_ALL)
inline int function_declaration(double, char = 'x') { return 0; }
int (*function_pointer)(double, char) = nullptr;
int(&function_reference)(double, char) = function_declaration;
int (*array_of_function_pointers[2])(double, char) = { nullptr, nullptr };
int (*(*function_returning_pointer_to_array)(double))[3] = nullptr;
auto (*trailing_return_function_pointer)(double) -> int (*)[3] = nullptr;
#endif


#if defined(TEST_TYPE_ALIAS) || defined(TEST_ALL)
using namespace TestNamespace3::TestNamespace4;
using std::string;
using TestClass04 = std::string;
using using_function_pointer = int (*)(int);
using using_array_reference = int(&)[3];
typedef int value_type;
typedef int (*typedef_function_pointer)(int);
typedef struct { int a; int b; } S, * pS;
typedef struct { float a; float b; } *pSS, SS;
#endif


#if defined(TEST_UNIONS) || defined(TEST_ALL)
union UnionTest1;
union UnionTest2 {
	int integer;
	double floating;
	char bytes[sizeof(double)];
};
struct UnionTest3 {
	int tag;
	union {
		int integer;
		float floating;
	};
};
#endif


#if defined(TEST_LINKAGE) || defined(TEST_ALL)
extern "C" int c_linkage_function(int);
extern "C++" int cpp_linkage_function(int);
#endif


#if defined(TEST_OPERATORS) || defined(TEST_ALL)
class OperatorTestClass1
{
public:
	OperatorTestClass1& operator=(const OperatorTestClass1&) = default;
	OperatorTestClass1& operator=(OperatorTestClass1&&) noexcept = default;
	int operator()(int value) const;
	int operator[](int index) const;
	int operator+(const OperatorTestClass1&) const;
	OperatorTestClass1& operator++();
	OperatorTestClass1 operator++(int);
	explicit operator bool() const noexcept;
	operator int() const;
	auto operator<=>(const OperatorTestClass1&) const = default;
};
#endif


#if defined(TEST_TEMPLATE) || defined(TEST_ALL)
template<typename T>
struct TemplateDefaultContainer
{
	using ValueType = T;
};

template<typename T, int Count = 4, template<typename> class Container = TemplateDefaultContainer>
class TemplateTestClass1;

template<typename T, int Count, template<typename> class Container>
class TemplateTestClass1
{
public:
	using ValueType = T;
	static constexpr int Size = Count;
	Container<T> Storage;

	template<typename U>
	U MemberTemplateFunction(U Value);
};

template<typename... Types>
struct TemplateTypeList {};

template<typename T, T Value>
struct TemplateNonTypeParameter
{
	static constexpr T Constant = Value;
};

template<auto Value>
struct TemplateAutoNonTypeParameter
{
	static constexpr auto Constant = Value;
};

template<typename T>
T TemplateFunction(T Value);

template<typename... Values>
auto TemplateFoldExpression(Values... values) -> decltype((values + ...));

template<typename T>
using TemplateAlias = TemplateTestClass1<T, 1>;

template<typename T>
inline constexpr bool TemplateVariable = sizeof(T) > 1;

template<typename T>
struct TemplateSpecialization;

template<>
struct TemplateSpecialization<int>
{
	using Type = int;
};

template<typename T>
struct TemplateSpecialization<T*>
{
	using Type = T*;
};
#endif


#if defined(TEST_CONCEPT) || defined(TEST_ALL)
template<typename T>
concept ConceptHasSize = requires(T Value)
{
	typename T::size_type;
	Value.size();
};

template<typename T>
concept ConceptSmall = sizeof(T) <= sizeof(void*);

template<typename T, typename U>
concept ConceptSameSize = sizeof(T) == sizeof(U);

template<typename T>
concept ConceptAddable = requires(T Left, T Right)
{
	Left + Right;
	{ Left += Right };
	requires sizeof(T) > 0;
};
#endif


#if defined(TEST_REQUIRES) || defined(TEST_ALL)
template<ConceptSmall T>
struct RequiresConstrainedClass
{
	T Value;
};

template<typename T>
	requires ConceptHasSize<T>
struct RequiresClauseClass
{
	T Value;
};

template<typename T>
T RequiresFunction(T Value) requires ConceptAddable<T>;

template<ConceptAddable T>
T RequiresConstrainedParameter(T Value);

template<typename T>
auto RequiresExpressionFunction(T Value) -> decltype(Value.size()) requires requires(T Object)
{
	Object.size();
	{ Object.size() };
};
#endif


#if defined(TEST_ATTRIBUTES) || defined(TEST_ALL)
[[maybe_unused]] int AttributeVariable1 = 0;
[[deprecated]] int AttributeFunction1();
[[deprecated("Use AttributeFunction3 instead")]] int AttributeFunction2();
[[nodiscard]] int AttributeFunction3();
[[nodiscard("Check the return value")]] int AttributeFunction4();
[[maybe_unused, nodiscard]] int AttributeFunction5();
[[using gnu: unused]] int AttributeVariable2;

alignas(16) int AttributeAlignedVariable1;
alignas(32) char AttributeAlignedVariable2[32];
__declspec(dllexport) int AttributeDeclspecVariable1;
[[maybe_unused]] int AttributeGnuVariable1;

namespace [[deprecated("Use AttributeNamespace2 instead")]] AttributeNamespace1
{
}

namespace AttributeNamespace2
{
}
namespace AttributeNamespaceAlias1 = AttributeNamespace2;

class [[nodiscard]] AttributeClass1
{
public:
	[[maybe_unused]] int MemberVariable1;
	[[no_unique_address]] TestStruct1 MemberVariable2;
	[[nodiscard]] int MemberFunction1() const;
};

struct alignas(64) AttributeStruct1
{
	int Value;
};

enum class [[nodiscard]] AttributeEnum1
{
	Value1,
	Value2 [[deprecated]],
};

using AttributeUsing1 [[maybe_unused]] = AttributeClass1;
[[maybe_unused]] typedef AttributeClass1 AttributeTypedef1;

template<typename T>
[[nodiscard]] T AttributeTemplateFunction1(T Value);

template<typename T>
concept AttributeConcept1 = requires(T Value)
{
	{ Value + Value };
};
#endif


#if defined(TEST_STATIC_ASSERT) || defined(TEST_ALL)
static_assert(true);
static_assert(sizeof(int) > 0, "int must exist");
static_assert(sizeof(char) == 1, "char size is one byte");
static_assert(1 + 1 == 2);
#endif
