
#include <mlib/tests/_pc_.h>

#include <mlib/format.h>

BOOST_AUTO_TEST_CASE( TestFormat )
{
    // Boost.Format
    std::string f_str = (boost::format("writing %2%,  x=%1% : %3%-th try") % "toto" % 40.23 % 50).str();
    BOOST_CHECK( strcmp(f_str.c_str(), "writing 40.23,  x=toto : 50-th try") == 0 );
}

