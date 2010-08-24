//  (C) Copyright Eric Niebler 2004.
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
// define the container types, used by utility.hpp to generate the helper functions
typedef char *foreach_container_type;
typedef char const *foreach_const_container_type;
typedef char foreach_value_type;
typedef char &foreach_reference_type;
typedef char const &foreach_const_reference_type;

#include "./utility.hpp"

///////////////////////////////////////////////////////////////////////////////
// define some containers
//
static char my_ntcs_buffer[] = "\1\2\3\4\5";
static char *my_ntcs  = my_ntcs_buffer;
static char const *my_const_ntcs  = my_ntcs;

///////////////////////////////////////////////////////////////////////////////
// test_main
//   
BOOST_AUTO_TEST_CASE( cstr_byref )
{
    // non-const containers by reference
    BOOST_CHECK(sequence_equal_byref_n(my_ntcs, "\1\2\3\4\5"));

    // const containers by reference
    BOOST_CHECK(sequence_equal_byref_c(my_const_ntcs, "\1\2\3\4\5"));

    // mutate the mutable collections
    mutate_foreach_byref(my_ntcs);

    // compare the mutated collections to the actual results
    BOOST_CHECK(sequence_equal_byref_n(my_ntcs, "\2\3\4\5\6"));
}
