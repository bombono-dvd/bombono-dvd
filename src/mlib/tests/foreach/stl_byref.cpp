//  (C) Copyright Eric Niebler 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 Revision history:
   25 August 2005: Initial version.
*/

#include <mlib/tests/_pc_.h>
//#include <boost/test/minimal.hpp>
#include <list>
#include <boost/foreach.hpp>

///////////////////////////////////////////////////////////////////////////////
// define the container types, used by utility.hpp to generate the helper functions
typedef std::list<int> foreach_container_type;
typedef std::list<int> const foreach_const_container_type;
typedef int foreach_value_type;
typedef int &foreach_reference_type;
typedef int const &foreach_const_reference_type;

#include "./utility.hpp"

///////////////////////////////////////////////////////////////////////////////
// initialize a std::list<int>
std::list<int> make_list()
{
    std::list<int> l;
    l.push_back(1);
    l.push_back(2);
    l.push_back(3);
    l.push_back(4);
    l.push_back(5);
    return l;
}

///////////////////////////////////////////////////////////////////////////////
// define some containers
//
std::list<int> my_list = make_list();
std::list<int> const &my_const_list = my_list;

///////////////////////////////////////////////////////////////////////////////
// test_main
//   
BOOST_AUTO_TEST_CASE( Test )
{
    // non-const containers by reference
    BOOST_CHECK(sequence_equal_byref_n(my_list, "\1\2\3\4\5"));

    // const containers by reference
    BOOST_CHECK(sequence_equal_byref_c(my_const_list, "\1\2\3\4\5"));

    // mutate the mutable collections
    mutate_foreach_byref(my_list);

    // compare the mutated collections to the actual results
    BOOST_CHECK(sequence_equal_byref_n(my_list, "\2\3\4\5\6"));
}
