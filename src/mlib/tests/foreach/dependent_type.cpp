//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
  Revision history:
  26 August 2005 : Initial version.
*/

#include <mlib/tests/_pc_.h>
//#include <boost/test/minimal.hpp>
#include <boost/foreach.hpp>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// use FOREACH to iterate over a sequence with a dependent type
template<typename Vector>
void do_test(Vector const & vect)
{
    typedef BOOST_DEDUCED_TYPENAME Vector::value_type value_type;
    BOOST_FOREACH(value_type i, vect)
    {
        // no-op, just make sure this compiles
        ((void)i);
    }
}

///////////////////////////////////////////////////////////////////////////////
// test_main
//   
BOOST_AUTO_TEST_CASE( Test )
{
    std::vector<int> vect;
    do_test(vect);
}
