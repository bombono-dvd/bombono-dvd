
#include <mlib/tests/_pc_.h>

#include <mlib/format.h>
#include <mlib/regex.h>

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

