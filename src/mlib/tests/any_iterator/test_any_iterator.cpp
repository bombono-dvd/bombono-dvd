//  (C) Copyright Thomas Becker 2005. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Revision History
// ================
//
// 27 Dec 2006 (Thomas Becker) Created

// Includes
// ========

#include <mlib/tests/_pc_.h>

void any_iterator_demo();
bool test_any_iterator_wrapper();
bool test_any_iterator();
void boost_iterator_library_example_test();

BOOST_AUTO_TEST_CASE( test_any_iterator2 )
{
  any_iterator_demo();

  BOOST_CHECK_MESSAGE( test_any_iterator_wrapper(), "Problem in test_any_iterator_wrapper." );

  BOOST_CHECK_MESSAGE( test_any_iterator(), "Problem in test_any_iterator." );

  boost_iterator_library_example_test();
}



