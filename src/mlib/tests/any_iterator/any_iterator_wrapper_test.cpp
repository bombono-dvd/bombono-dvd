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

#ifdef _MSC_VER

  // Enable rudimentary memory tracking under Win32.
  #define _CRTDBG_MAP_ALLOC
  #include <stdlib.h>
  #include <crtdbg.h>

  // Disable stupid warning about what Microsoft thinks is deprecated
  #pragma warning( disable : 4996 )

#endif

#include <mlib/any_iterator/detail/any_iterator_wrapper.hpp>
#include<vector>

#define ASSERT_RETURN_FALSE( b ) if( ! (b) ){ return false; }

using namespace IteratorTypeErasure;

bool test_any_iterator_wrapper()
{

  std::vector<int> vect_of_ints;
  vect_of_ints.push_back(42);
  vect_of_ints.push_back(43);
  vect_of_ints.push_back(47);

  //////////////////////////////////////////////////////////////////////
  //
  // Incrementable
  //
  /////////////////////////////////////////////////////////////////////
  
  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::incrementable_traversal_tag,
    int const &,
    ptrdiff_t
  > default_constructed_const_incrementable_wrapper;

  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::incrementable_traversal_tag,
    int const &,
    ptrdiff_t
  > const_incrementable_wrapper(vect_of_ints.begin());

  ASSERT_RETURN_FALSE( 42 == const_incrementable_wrapper.dereference() );
  const_incrementable_wrapper.increment();
  ASSERT_RETURN_FALSE( 43 == const_incrementable_wrapper.dereference() );
  
  detail::any_iterator_abstract_base<
    const int,
    boost::incrementable_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_incrementable_wrapper_clone = const_incrementable_wrapper.clone();

  ASSERT_RETURN_FALSE( 43 == const_incrementable_wrapper_clone->dereference() );

  delete const_incrementable_wrapper_clone;

  detail::any_iterator_wrapper<
    std::vector<int>::iterator,
    int,
    boost::incrementable_traversal_tag,
    int &,
    ptrdiff_t
  > incrementable_wrapper(vect_of_ints.begin());

  incrementable_wrapper.dereference() = 4711;
  ASSERT_RETURN_FALSE( 4711 == incrementable_wrapper.dereference() );
  incrementable_wrapper.dereference() = 42;
  
  const_incrementable_wrapper_clone = incrementable_wrapper.make_const_clone_with_const_value_type();
  ASSERT_RETURN_FALSE( 42 == const_incrementable_wrapper_clone->dereference() );

  detail::any_iterator_abstract_base<
    int,
    boost::incrementable_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_incrementable_wrapper_clone_with_non_const_value_type = incrementable_wrapper.make_const_clone_with_non_const_value_type();
  ASSERT_RETURN_FALSE( 42 == const_incrementable_wrapper_clone_with_non_const_value_type->dereference() );
  
  delete const_incrementable_wrapper_clone;
  delete const_incrementable_wrapper_clone_with_non_const_value_type;

  //////////////////////////////////////////////////////////////////////
  //
  // Single Pass
  //
  /////////////////////////////////////////////////////////////////////
  
  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::single_pass_traversal_tag,
    int const &,
    ptrdiff_t
  > default_constructed_single_pass_wrapper;

  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::single_pass_traversal_tag,
    int const &,
    ptrdiff_t
  > const_single_pass_wrapper(vect_of_ints.begin());

  ASSERT_RETURN_FALSE( 42 == const_single_pass_wrapper.dereference() );
  const_single_pass_wrapper.increment();
  ASSERT_RETURN_FALSE( 43 == const_single_pass_wrapper.dereference() );
  
  detail::any_iterator_abstract_base<
    const int,
    boost::single_pass_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_single_pass_wrapper_clone = const_single_pass_wrapper.clone();

  ASSERT_RETURN_FALSE( 43 == const_single_pass_wrapper_clone->dereference() );

  delete const_single_pass_wrapper_clone;

  detail::any_iterator_wrapper<
    std::vector<int>::iterator,
    int,
    boost::single_pass_traversal_tag,
    int &,
    ptrdiff_t
  > single_pass_wrapper(vect_of_ints.begin());

  single_pass_wrapper.dereference() = 4711;
  ASSERT_RETURN_FALSE( 4711 == single_pass_wrapper.dereference() );
  single_pass_wrapper.dereference() = 42;
  
  const_single_pass_wrapper_clone = single_pass_wrapper.make_const_clone_with_const_value_type();
  ASSERT_RETURN_FALSE( 42 == const_single_pass_wrapper_clone->dereference() );

  detail::any_iterator_abstract_base<
    int,
    boost::single_pass_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_single_pass_wrapper_clone_with_non_const_value_type = const_single_pass_wrapper.make_const_clone_with_non_const_value_type();
  ASSERT_RETURN_FALSE( 43 == const_single_pass_wrapper_clone_with_non_const_value_type->dereference() );
  
  delete const_single_pass_wrapper_clone;
  delete const_single_pass_wrapper_clone_with_non_const_value_type;

  //////////////////////////////////////////////////////////////////////
  //
  // Forward
  //
  /////////////////////////////////////////////////////////////////////
  
  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::forward_traversal_tag,
    int const &,
    ptrdiff_t
  > default_constructed_forward_wrapper;

  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::forward_traversal_tag,
    int const &,
    ptrdiff_t
  > const_forward_wrapper(vect_of_ints.begin());

  ASSERT_RETURN_FALSE( 42 == const_forward_wrapper.dereference() );
  const_forward_wrapper.increment();
  ASSERT_RETURN_FALSE( 43 == const_forward_wrapper.dereference() );
  
  detail::any_iterator_abstract_base<
    const int,
    boost::forward_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_forward_wrapper_clone = const_forward_wrapper.clone();

  ASSERT_RETURN_FALSE( 43 == const_forward_wrapper_clone->dereference() );

  ASSERT_RETURN_FALSE( const_forward_wrapper.equal(*const_forward_wrapper_clone) );
  const_forward_wrapper_clone->increment();
  ASSERT_RETURN_FALSE( ! const_forward_wrapper.equal(*const_forward_wrapper_clone) );

  delete const_forward_wrapper_clone;

  detail::any_iterator_wrapper<
    std::vector<int>::iterator,
    int,
    boost::forward_traversal_tag,
    int &,
    ptrdiff_t
  > forward_wrapper(vect_of_ints.begin());

  forward_wrapper.dereference() = 4711;
  ASSERT_RETURN_FALSE( 4711 == forward_wrapper.dereference() );
  forward_wrapper.dereference() = 42;
  
  const_forward_wrapper_clone = forward_wrapper.make_const_clone_with_const_value_type();
  ASSERT_RETURN_FALSE( 42 == const_forward_wrapper_clone->dereference() );
  detail::any_iterator_abstract_base<
    int,
    boost::forward_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_forward_wrapper_clone_with_non_const_value_type = const_forward_wrapper.make_const_clone_with_non_const_value_type();
  ASSERT_RETURN_FALSE( 43 == const_forward_wrapper_clone_with_non_const_value_type->dereference() );
  
  delete const_forward_wrapper_clone;
  delete const_forward_wrapper_clone_with_non_const_value_type;

  //////////////////////////////////////////////////////////////////////
  //
  // Bidirectional
  //
  /////////////////////////////////////////////////////////////////////
  
  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::bidirectional_traversal_tag,
    int const &,
    ptrdiff_t
  > default_constructed_bidirectional_wrapper;

  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::bidirectional_traversal_tag,
    int const &,
    ptrdiff_t
  > const_bidirectional_wrapper(vect_of_ints.begin());

  ASSERT_RETURN_FALSE( 42 == const_bidirectional_wrapper.dereference() );
  const_bidirectional_wrapper.increment();
  ASSERT_RETURN_FALSE( 43 == const_bidirectional_wrapper.dereference() );
  const_bidirectional_wrapper.decrement();
  ASSERT_RETURN_FALSE( 42 == const_bidirectional_wrapper.dereference() );
  
  detail::any_iterator_abstract_base<
    const int,
    boost::bidirectional_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_bidirectional_wrapper_clone = const_bidirectional_wrapper.clone();

  ASSERT_RETURN_FALSE( 42 == const_bidirectional_wrapper_clone->dereference() );

  ASSERT_RETURN_FALSE( const_bidirectional_wrapper.equal(*const_bidirectional_wrapper_clone) );
  const_bidirectional_wrapper_clone->increment();
  ASSERT_RETURN_FALSE( ! const_bidirectional_wrapper.equal(*const_bidirectional_wrapper_clone) );

  delete const_bidirectional_wrapper_clone;

  detail::any_iterator_wrapper<
    std::vector<int>::iterator,
    int,
    boost::bidirectional_traversal_tag,
    int &,
    ptrdiff_t
  > bidirectional_wrapper(vect_of_ints.begin());

  bidirectional_wrapper.dereference() = 4711;
  ASSERT_RETURN_FALSE( 4711 == bidirectional_wrapper.dereference() );
  bidirectional_wrapper.dereference() = 42;
  
  const_bidirectional_wrapper_clone = bidirectional_wrapper.make_const_clone_with_const_value_type();
  ASSERT_RETURN_FALSE( 42 == const_bidirectional_wrapper_clone->dereference() );

  detail::any_iterator_abstract_base<
    int,
    boost::bidirectional_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_bidirectional_wrapper_clone_with_non_const_value_type = const_bidirectional_wrapper.make_const_clone_with_non_const_value_type();
  ASSERT_RETURN_FALSE( 42 == const_bidirectional_wrapper_clone_with_non_const_value_type->dereference() );
  
  delete const_bidirectional_wrapper_clone;
  delete const_bidirectional_wrapper_clone_with_non_const_value_type;

  //////////////////////////////////////////////////////////////////////
  //
  // Random Access
  //
  /////////////////////////////////////////////////////////////////////
  
  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::random_access_traversal_tag,
    int const &,
    ptrdiff_t
  > default_constructed_random_access_wrapper;

  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::random_access_traversal_tag,
    int const &,
    ptrdiff_t
  > const_random_access_wrapper(vect_of_ints.begin());

  ASSERT_RETURN_FALSE( 42 == const_random_access_wrapper.dereference() );
  const_random_access_wrapper.increment();
  ASSERT_RETURN_FALSE( 43 == const_random_access_wrapper.dereference() );
  const_random_access_wrapper.decrement();
  ASSERT_RETURN_FALSE( 42 == const_random_access_wrapper.dereference() );
  
  const_random_access_wrapper.advance(2);
  ASSERT_RETURN_FALSE( 47 == const_random_access_wrapper.dereference() );
  const_random_access_wrapper.advance(-2);
  ASSERT_RETURN_FALSE( 42 == const_random_access_wrapper.dereference() );
  
  detail::any_iterator_wrapper<
    std::vector<int>::const_iterator,
    const int,
    boost::random_access_traversal_tag,
    int const &,
    ptrdiff_t
  > other_const_random_access_wrapper = const_random_access_wrapper;
  other_const_random_access_wrapper.advance(3);
  ASSERT_RETURN_FALSE( 3 == const_random_access_wrapper.distance_to(other_const_random_access_wrapper) );

  detail::any_iterator_abstract_base<
    const int,
    boost::random_access_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_random_access_wrapper_clone = const_random_access_wrapper.clone();

  ASSERT_RETURN_FALSE( 42 == const_random_access_wrapper_clone->dereference() );

  ASSERT_RETURN_FALSE( const_random_access_wrapper.equal(*const_random_access_wrapper_clone) );
  const_random_access_wrapper_clone->increment();
  ASSERT_RETURN_FALSE( ! const_random_access_wrapper.equal(*const_random_access_wrapper_clone) );

  delete const_random_access_wrapper_clone;

  detail::any_iterator_wrapper<
    std::vector<int>::iterator,
    int,
    boost::random_access_traversal_tag,
    int &,
    ptrdiff_t
  > random_access_wrapper(vect_of_ints.begin());

  random_access_wrapper.dereference() = 4711;
  ASSERT_RETURN_FALSE( 4711 == random_access_wrapper.dereference() );
  random_access_wrapper.dereference() = 42;
  
  const_random_access_wrapper_clone = random_access_wrapper.make_const_clone_with_const_value_type();
  ASSERT_RETURN_FALSE( 42 == const_random_access_wrapper_clone->dereference() );
  detail::any_iterator_abstract_base<
    int,
    boost::random_access_traversal_tag,
    int const &,
    ptrdiff_t
  >* const_random_access_wrapper_clone_with_non_const_value_type = const_random_access_wrapper.make_const_clone_with_non_const_value_type();
  ASSERT_RETURN_FALSE( 42 == const_random_access_wrapper_clone_with_non_const_value_type->dereference() );
  
  delete const_random_access_wrapper_clone;
  delete const_random_access_wrapper_clone_with_non_const_value_type;

  return true;
}
