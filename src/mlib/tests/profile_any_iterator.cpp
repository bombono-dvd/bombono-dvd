#include <mlib/any_iterator.h>

#ifdef ADOBE_AIT
#   define AIT_INSTANCE(ts_t) adobe::bidirectional_iter<ts_t> it((ts_t*)arr)
#else
#   define AIT_INSTANCE(ts_t) IteratorTypeErasure::any_iterator<ts_t, boost::bidirectional_traversal_tag> it((ts_t*)arr);
#endif


template<int value>
struct TestStruct
{
};

#define IMPL_DEF(Idx)  \
    {                  \
        typedef TestStruct<Idx> ts_t; \
        ts_t arr[1];   \
        AIT_INSTANCE(ts_t); \
    }         \
/**/


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


void TestImpl()
{
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
}


