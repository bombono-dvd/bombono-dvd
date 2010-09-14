
#include <boost/bind.hpp>
#include <boost/function.hpp>

typedef boost::function<void()> TestFunctor;

template<int value>
struct TestStruct
{
};

#define IMPL_DEF_(Idx, Type) void Impl ## Idx(Type) {} \
void TestImpl ## Idx()                        \
{                                             \
    TestFunctor fnr;                          \
    fnr = boost::bind(&Impl ## Idx, Type());  \
} \
/**/

#define IMPL_DEF(Idx) IMPL_DEF_(Idx, TestStruct<Idx>)

#define IMPL_DEF_10_(Idx) \
IMPL_DEF(Idx ## 0) \
IMPL_DEF(Idx ## 1) \
IMPL_DEF(Idx ## 2) \
IMPL_DEF(Idx ## 3) \
IMPL_DEF(Idx ## 4) \
IMPL_DEF(Idx ## 5) \
IMPL_DEF(Idx ## 6) \
IMPL_DEF(Idx ## 7) \
IMPL_DEF(Idx ## 8) \
IMPL_DEF(Idx ## 9) \
/**/

#define IMPL_DEF_10(Idx) IMPL_DEF_10_(Idx)

IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)
IMPL_DEF_10(__LINE__)


