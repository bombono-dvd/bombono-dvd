
#include <mlib/tests/_pc_.h>

#include <mlib/format.h>
#include <mlib/regex.h>
#include <mlib/stream.h>
#include <mlib/tech.h>

BOOST_AUTO_TEST_CASE( TestFormat )
{
    // Boost.Format
    std::string f_str = (boost::format("writing %2%,  x=%1% : %3%-th try") % "toto" % 40.23 % 50).str();
    BOOST_CHECK( strcmp(f_str.c_str(), "writing 40.23,  x=toto : 50-th try") == 0 );
}

BOOST_AUTO_TEST_CASE( TestRegexWrapper )
{
    // re::search()
    re::pattern pat("([0-9]+)");
    std::string str = "x111xxx222dd333x";

    re::match_results what;
    int matches = 0;
    for( re::const_iterator it = str.begin(), end = str.end(); 
         re::search(it, end, what, pat);  
         it = what[1].second )
    {
        BOOST_CHECK_EQUAL( (int)what.size(), 2 );
        matches++;
        BOOST_CHECK_EQUAL( what.str(1), boost::format("%1%%1%%1%") % matches % bf::stop );
    }
    BOOST_CHECK_EQUAL( matches, 3 );

    // re::match()
    BOOST_CHECK( re::match("12345",  pat) );
    BOOST_CHECK( !re::match("12x45", pat) );
}

//typedef union
//{
//    uint16_t* sp;
//    uint32_t* wp;
//} U32P;
//
//uint32_t sa_swap_words(uint32_t arg)
//{
//    U32P in; // = { .wp = &arg };
//    in.wp = &arg;
//
//    const uint16_t hi = in.sp[0];
//    const uint16_t lo = in.sp[1];
//
//    in.sp[0] = lo;
//    in.sp[1] = hi;
//
//    return arg;
//}
//
//bool sa_is_swaped_vs_sa()
//{
//    return sa_swap_words(1) != 1;
//}

//BOOST_AUTO_TEST_CASE( TestStrictAliasing )
//{
//    BOOST_CHECK( sa_is_swaped_vs_sa() );
//}

UNUSED_FUNCTION
static bool SetTrue(bool& b)
{
    b = true;
    return true;
}

BOOST_AUTO_TEST_CASE( TestASSERT_UNUSED )
{
    int var1;
    UNUSED_VAR(var1); // без предупреждения!

    bool b1 = false;
    bool b2 = false;

#ifdef NDEBUG
    // ASSERT_OR_UNUSED
    // Внимание: выполняет выражение в скобках в любом случае, поэтому
    // подходит только для проверок переменных простых типов в отладке и
    // исключения неиспользуемости в релизе
    ASSERT_OR_UNUSED( b1 ); // не должно быть ошибки и предупреждения

    // ASSERT_OR_UNUSED_VAR
    // не выполняет первый аргумент в релизе
    ASSERT_OR_UNUSED_VAR( SetTrue(b2), b2 );
    ASSERT_RTL( !b2 );
#else
    // ASSERT_OR_UNUSED
    ASSERT_OR_UNUSED( !b1 ); // = ASSERT()

    // ASSERT_OR_UNUSED_VAR
    ASSERT_OR_UNUSED_VAR( SetTrue(b2), b2 );
    ASSERT_RTL( b2 );
#endif // NDEBUG

}

