//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
Revision history:
25 August 2005 : Initial version.
*/

#include <mlib/tests/_pc_.h>
//#include <boost/test/minimal.hpp>
#include <boost/foreach.hpp>

///////////////////////////////////////////////////////////////////////////////
// define a user-defined collection type and teach BOOST_FOREACH how to enumerate it
//
namespace mine
{
    struct dummy {};
}

namespace mine
{
    char * range_begin(mine::dummy&) {return 0;}
    char const * range_begin(mine::dummy const&) {return 0;}
    char * range_end(mine::dummy&) {return 0;}
    char const * range_end(mine::dummy const&) {return 0;}
}

namespace boost
{
    template<>
    struct range_mutable_iterator<mine::dummy>
    {
        typedef char * type;
    };
    template<>
    struct range_const_iterator<mine::dummy>
    {
        typedef char const * type;
    };
}

///////////////////////////////////////////////////////////////////////////////
// test_main
//   
BOOST_AUTO_TEST_CASE( Test )
{
    // loop over a user-defined type (just make sure this compiles)
    mine::dummy d;
    BOOST_FOREACH( char c, d )
    {
        ((void)c); // no-op
    }
}
