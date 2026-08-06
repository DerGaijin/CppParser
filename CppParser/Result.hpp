
// ============
// UnitTest.hpp
// ============

namespace TestNamespace1
{
}
namespace 
{
}
inline namespace TestNamespace2
{
}
namespace TestNamespace3
{
	namespace TestNamespace4
	{
	}
}
namespace TestNamespace5::TestNamespace6::TestNamespace7
{
}
namespace TestNamespace8::inline TestNamespace9::inline TestNamespace10
{
}
namespace TestNamespace10 = TestNamespace1;
namespace TestNamespace11 = ::TestNamespace1;
namespace TestNamespace12 = TestNamespace3::TestNamespace4;
class TestClass1
{
public:
protected:
private:
};
class __declspec(dllexport) TestClass2
{
public:
