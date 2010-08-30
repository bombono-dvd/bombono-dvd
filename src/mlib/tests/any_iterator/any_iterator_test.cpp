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

#include <mlib/any_iterator/any_iterator.hpp>

#include<boost/iterator/transform_iterator.hpp>
#include<boost/function.hpp>

#include<iostream>
#include<vector>
#include<set>
#include<deque>
#include<sstream>

#define ASSERT_RETURN_FALSE( b ) if( ! (b) ){ return false; }

using namespace IteratorTypeErasure;

////////////////////////////////////////////////////////////////////////
//
// An iterator type with const value type.
//
class my_iterator
{
public:
  typedef const int value_type;
  typedef int const * pointer;
  typedef int const & reference;
  typedef std::forward_iterator_tag iterator_category;
  typedef ptrdiff_t difference_type;

  reference operator*() const
  {
    return m_i;
  }

  my_iterator& operator++()
  {
    return *this;
  }

  int m_i;

};
//
bool operator==(my_iterator const & /*lhs*/, my_iterator const & /*rhs*/)
{
  return true;
}

////////////////////////////////////////////////////////////////////////
//
// Simple hierarchy
//
struct base
{
  base() : i(42)
  {}
  int i;
};
//
struct derived : public  base
{
};

struct base_less : public std::binary_function<base const &, base const &, bool>
{
  bool operator()(base const & lhs, base const & rhs)
  {
    return lhs.i < rhs.i;
  }
};


////////////////////////////////////////////////////////////////////////
//
// Mimick iterator_facade's operator_brackets_proxy. If the tests
// involving this class work, then operator[] should work properly.
// These tests are provided so that in case of failure, the error
// messages are easier to interpret.
//
template<class Iterator>
class iterator_proxy
{
public:
  iterator_proxy(Iterator const & it) : m_it(it) {}
  Iterator m_it;
};

bool test_any_iterator()
{

  // Make a vector of three elements with a const iterator.
  //
  std::vector<int> vector_of_ints;
  vector_of_ints.push_back(42);
  vector_of_ints.push_back(43);
  vector_of_ints.push_back(44);
  //
  std::vector<int>::const_iterator a_vector_const_iterator = vector_of_ints.begin();

  // Make a set of three elements with a const iterator.
  //
  std::set<int> set_of_ints;
  set_of_ints.insert(1);
  set_of_ints.insert(2);
  set_of_ints.insert(3);
  set_of_ints.insert(4);
  std::set<int>::const_iterator a_set_const_iterator = set_of_ints.begin();
  std::set<int>::iterator a_set_iterator = set_of_ints.begin();

  //
  // Play with random access iterators with int value type.
  //
  
  typedef make_any_iterator_type<std::vector<int>::iterator>::type 
    any_random_access_iterator_to_int;

  any_random_access_iterator_to_int a_random_access_iterator_to_int;
  any_random_access_iterator_to_int a_null_random_access_iterator_to_int;
  ASSERT_RETURN_FALSE( a_random_access_iterator_to_int == a_null_random_access_iterator_to_int );

  // See comments preceding the declaration of class iterator_proxy.
  iterator_proxy<any_random_access_iterator_to_int> proxy1(a_random_access_iterator_to_int);
  iterator_proxy<any_random_access_iterator_to_int> proxy2(proxy1);
  ASSERT_RETURN_FALSE( proxy1.m_it == proxy2.m_it );

  a_random_access_iterator_to_int = vector_of_ints.begin();
  ASSERT_RETURN_FALSE( a_random_access_iterator_to_int != a_null_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( 42 == *a_random_access_iterator_to_int );
  ++a_random_access_iterator_to_int;
  ASSERT_RETURN_FALSE( 43 == *a_random_access_iterator_to_int );
  any_random_access_iterator_to_int another_random_access_iterator_to_int = a_random_access_iterator_to_int++;

  ASSERT_RETURN_FALSE( !(another_random_access_iterator_to_int == a_random_access_iterator_to_int) );
  ASSERT_RETURN_FALSE( another_random_access_iterator_to_int != a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( another_random_access_iterator_to_int <= a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( !(another_random_access_iterator_to_int >= a_random_access_iterator_to_int) );
  ASSERT_RETURN_FALSE( another_random_access_iterator_to_int < a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( !(another_random_access_iterator_to_int > a_random_access_iterator_to_int) );

  ASSERT_RETURN_FALSE( 44 == *a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( 43 == *another_random_access_iterator_to_int );
  a_random_access_iterator_to_int -= 2;
  ASSERT_RETURN_FALSE( 42 == *a_random_access_iterator_to_int );
  another_random_access_iterator_to_int = a_random_access_iterator_to_int + 2;
  ASSERT_RETURN_FALSE( 44 == *another_random_access_iterator_to_int );

  ASSERT_RETURN_FALSE( 42 == a_random_access_iterator_to_int[0] );
  ASSERT_RETURN_FALSE( 43 == a_random_access_iterator_to_int[1] );
  ASSERT_RETURN_FALSE( 44 == a_random_access_iterator_to_int[2] );

  a_random_access_iterator_to_int = another_random_access_iterator_to_int - 1;
  ASSERT_RETURN_FALSE( 43 == *a_random_access_iterator_to_int );
  a_random_access_iterator_to_int = another_random_access_iterator_to_int - 2;
  ASSERT_RETURN_FALSE( 42 == *a_random_access_iterator_to_int );

  a_random_access_iterator_to_int = --another_random_access_iterator_to_int;
  ASSERT_RETURN_FALSE( 43 == *a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( 43 == *another_random_access_iterator_to_int );

  a_random_access_iterator_to_int = another_random_access_iterator_to_int--;
  ASSERT_RETURN_FALSE( 43 == *a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( 42 == *another_random_access_iterator_to_int );

  ++another_random_access_iterator_to_int;
  ASSERT_RETURN_FALSE( another_random_access_iterator_to_int == a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( !(another_random_access_iterator_to_int != a_random_access_iterator_to_int) );
  ASSERT_RETURN_FALSE( another_random_access_iterator_to_int <= a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( another_random_access_iterator_to_int >= a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( !( another_random_access_iterator_to_int < a_random_access_iterator_to_int) );
  ASSERT_RETURN_FALSE( !( another_random_access_iterator_to_int > a_random_access_iterator_to_int) );

  std::vector<int>::const_iterator a_const_vector_iterator_of_ints = vector_of_ints.begin();
  std::vector<int>::iterator a_non_const_vector_iterator_of_ints = vector_of_ints.begin() + 2;

  typedef make_any_iterator_type<std::vector<int>::const_iterator>::type 
    any_random_access_const_iterator_to_int;

  any_random_access_const_iterator_to_int a_random_access_const_iterator_to_int(a_const_vector_iterator_of_ints);
  ASSERT_RETURN_FALSE( 42 == *a_random_access_const_iterator_to_int );
  ++a_const_vector_iterator_of_ints;
  a_random_access_const_iterator_to_int = a_const_vector_iterator_of_ints;
  ASSERT_RETURN_FALSE( 43 == *a_random_access_const_iterator_to_int );

  any_random_access_const_iterator_to_int another_random_access_const_iterator_to_int;
  another_random_access_const_iterator_to_int = a_non_const_vector_iterator_of_ints;
  ASSERT_RETURN_FALSE( 44 == *another_random_access_const_iterator_to_int );
  --a_const_vector_iterator_of_ints;
  another_random_access_const_iterator_to_int = a_non_const_vector_iterator_of_ints;
  ASSERT_RETURN_FALSE( 44 == *another_random_access_const_iterator_to_int );
  //
  // any_random_access_iterator_to_int yet_another_random_access_iterator_to_int(a_const_vector_iterator_of_ints); // error: cannot construct a non-const any-iterator from a const iterator
  any_random_access_iterator_to_int yet_another_random_access_iterator_to_int;
  // yet_another_random_access_iterator_to_int = a_const_vector_iterator_of_ints; // error: cannot assign a const iterator to a non-const any-iterator

  *a_random_access_iterator_to_int = 45;
  ASSERT_RETURN_FALSE( 45 == *a_random_access_const_iterator_to_int );
  // *a_random_access_const_iterator_to_int = 46; // error: cannot assign to dereferenced const iterator
  // a_random_access_iterator_to_int = a_random_access_const_iterator_to_int; // error: cannot assign const any-iterator to non-const any-iterator
  another_random_access_const_iterator_to_int = a_random_access_iterator_to_int;
  any_random_access_const_iterator_to_int yet_another_random_access_const_iterator_to_int = a_random_access_iterator_to_int;
  ASSERT_RETURN_FALSE( 45 == *a_random_access_iterator_to_int );
  ASSERT_RETURN_FALSE( 45 == *a_random_access_const_iterator_to_int );
  ASSERT_RETURN_FALSE( 45 == *another_random_access_const_iterator_to_int );
  ASSERT_RETURN_FALSE( 45 == *yet_another_random_access_const_iterator_to_int );

  // a_random_access_iterator_to_int = a_set_iterator; // error: cannot assign a set iterator to an any-random-access iterator

  // No convertibilities: constructor from wrapped iterator is explicit
  ASSERT_RETURN_FALSE( ! static_cast<bool>(boost::is_convertible<std::vector<int>::iterator, any_random_access_const_iterator_to_int>::value) );
  ASSERT_RETURN_FALSE( ! static_cast<bool>(boost::is_convertible<std::set<int>::iterator, any_random_access_iterator_to_int>::value) );

  typedef make_any_iterator_type<std::deque<int>::iterator>::type 
    any_deque_iterator_to_int;
  std::deque<int> dq;
  any_deque_iterator_to_int a_deque_iterator_to_int(dq.begin());
  any_deque_iterator_to_int another_deque_iterator_to_int(dq.end());
  ASSERT_RETURN_FALSE( a_deque_iterator_to_int == another_deque_iterator_to_int );
  
  a_deque_iterator_to_int = a_random_access_iterator_to_int;
  ASSERT_RETURN_FALSE( 45 == *a_deque_iterator_to_int );
  // a_deque_iterator_to_int = a_random_access_const_iterator_to_int; // error: cannot assign const to non-const

  //
  // Play with bidirectional iterators with int value type.
  //
  
  typedef make_any_iterator_type<std::set<int>::iterator>::type 
    bidirectional_iterator_to_int;
  bidirectional_iterator_to_int a_bidirectional_iterator_to_int(set_of_ints.begin());
  ASSERT_RETURN_FALSE( 1 == *a_bidirectional_iterator_to_int );
  
  // a_random_access_iterator_to_int = a_bidirectional_iterator_to_int; // error: cannot assign unrelated any-iterators

  // a_bidirectional_iterator_to_int = a_random_access_iterator_to_int; // depending on your STL implementation, this can be
                                                                        // a legitimate conversion to weaker traversal tag,
                                                                        // in which case the line compiles, or the iterators
                                                                        // can be unrelated, in which case it doesn't

  a_bidirectional_iterator_to_int = vector_of_ints.begin() + 1;
  ASSERT_RETURN_FALSE( 45 == *a_bidirectional_iterator_to_int );
  a_bidirectional_iterator_to_int = set_of_ints.begin();

  ASSERT_RETURN_FALSE( 1 == *a_bidirectional_iterator_to_int );
  // a_bidirectional_iterator_to_int[0]; // error: bidirectional iterator does not define op[]

  a_bidirectional_iterator_to_int = a_non_const_vector_iterator_of_ints;
  ASSERT_RETURN_FALSE( 44 == *a_bidirectional_iterator_to_int );

  // a_bidirectional_iterator_to_int = a_set_const_iterator; // error: cannot assign a const iterator to a non-const any-iterator
  // a_bidirectional_iterator_to_int = a_const_vector_iterator_of_ints; // error: cannot assign a const iterator to a non-const any-iterator

  a_bidirectional_iterator_to_int = a_set_iterator;
  ASSERT_RETURN_FALSE( 1 == *a_bidirectional_iterator_to_int );

//  a_bidirectional_iterator_to_int <= a_bidirectional_iterator_to_int;  // error: bidirectional iterator does not define this operator
//  a_bidirectional_iterator_to_int >= a_bidirectional_iterator_to_int;  // error: bidirectional iterator does not define this operator
//  a_bidirectional_iterator_to_int < a_bidirectional_iterator_to_int;  // error: bidirectional iterator does not define this operator
//  a_bidirectional_iterator_to_int > a_bidirectional_iterator_to_int;  // error: bidirectional iterator does not define this operator

  //
  // Test operator->
  //

  typedef std::pair<int, int> my_pair;
  std::vector<my_pair> vect;
  vect.push_back(std::make_pair(142, 143));
  //
  make_any_iterator_type<std::vector<my_pair>::iterator>::type it;
  it = vect.begin();
  ASSERT_RETURN_FALSE( 143 == it->second );

  //
  // Check the const subtleness (value type!)
  //

  typedef any_iterator<
    const int,
    std::forward_iterator_tag,
    int&,
    ptrdiff_t
  > any_iterator_with_const_value_type;
  //
  typedef any_iterator<
    int,
    std::forward_iterator_tag,
    int&,
    ptrdiff_t
  > any_iterator_with_non_const_value_type;
  //
  typedef any_iterator<
    const int,
    std::forward_iterator_tag,
    int const &,
    ptrdiff_t
  > const_any_iterator_with_const_value_type;
  //
  typedef any_iterator<
    int,
    std::forward_iterator_tag,
    int const &,
    ptrdiff_t
    > const_any_iterator_with_non_const_value_type;

  // Constness of value type is irrelevant
  my_iterator mit;
  const_any_iterator_with_const_value_type const_with_const_1(mit);
  const_any_iterator_with_const_value_type const_with_const_2;
  const_with_const_2 = mit;
  const_any_iterator_with_non_const_value_type const_with_non_const_1(mit);
  const_any_iterator_with_non_const_value_type const_with_non_const_2;
  const_with_non_const_2 = mit;
  //
  const_with_const_1 = const_with_non_const_1;
  const_with_non_const_1 = const_with_const_1;
  
  any_iterator_with_const_value_type non_const_with_const;
  any_iterator_with_non_const_value_type non_const_with_non_const;

  // non_const_with_const = non_const_with_non_const; // these are considered unrelated (perhaps allow that one exception?)
  // non_const_with_non_const = non_const_with_const; // these are considered unrelated (perhaps allow that one exception?)

//  non_const_with_const = const_with_non_const_1; // error: cannot convert from const to non-const.
//  non_const_with_const = const_with_const_1; // error: cannot convert from const to non-const.
//  non_const_with_non_const = const_with_non_const_1; // error: cannot convert from const to non-const.
//  non_const_with_non_const = const_with_const_1; // error: cannot convert from const to non-const.

  // Constness of value type is irrelevant
  const_with_const_1 = non_const_with_non_const;
  const_with_const_1 = non_const_with_const;
  const_with_non_const_1 = non_const_with_non_const;
  const_with_non_const_1 = non_const_with_const;
  
  std::vector<derived> vector_of_derived(3);
  vector_of_derived[0].i = 41;
  vector_of_derived[2].i = 43;

  std::set<base, base_less> set_of_base(vector_of_derived.begin(), vector_of_derived.end());
  typedef make_any_iterator_type<std::set<base, base_less>::iterator>::type 
    any_bidirectional_base_iterator;

  any_bidirectional_base_iterator a_bidirectional_base_iterator(set_of_base.begin());
  base a_base = *a_bidirectional_base_iterator;
  ASSERT_RETURN_FALSE( 41 == a_base.i );
  a_bidirectional_base_iterator = vector_of_derived.begin() + 1;
  ++a_bidirectional_base_iterator;
  a_base = *a_bidirectional_base_iterator;
  ASSERT_RETURN_FALSE( 43 == a_base.i );

  std::vector<base> vector_of_base;
  typedef make_any_iterator_type<std::vector<derived>::iterator>::type 
    any_bidirectional_derived_iterator;
  // any_bidirectional_derived_iterator a_bidirectional_derived_iterator(vector_of_base.begin()); // error: base does not convert to derived

  //
  // Using boost traversal tags instead of STL iterator categories
  //

  typedef boost::transform_iterator<
    boost::function<int (int)>,
    std::vector<int>::const_iterator
  > some_transform_iterator;

  typedef IteratorTypeErasure::any_iterator<
    int const,
    boost::bidirectional_traversal_tag,
    int const
  > bidirectional_traversal_any_iterator;

  bidirectional_traversal_any_iterator bt_ait(vector_of_ints.begin());
  bt_ait = vector_of_ints.begin();
  
  some_transform_iterator tr_it;
  bt_ait = tr_it;
  
  //
  // Conversions between any_iterator types, other than non-const to const.
  //

  any_random_access_iterator_to_int arit(vector_of_ints.begin());
  arit = vector_of_ints.begin();
  any_iterator<int, std::bidirectional_iterator_tag> abit;
  abit = arit;
  ASSERT_RETURN_FALSE( *abit == *arit );
  //  arit = abit; // error: cannot convert from bidirectional to random access iterator
  
  any_iterator<int, std::forward_iterator_tag> afit = abit;
  afit = arit;
  
  any_iterator<int, boost::single_pass_traversal_tag> asit = afit;
  asit = abit;
  asit = arit;

  any_iterator<int, boost::incrementable_traversal_tag> aniit = asit;
  aniit = afit;
  aniit = abit;
  aniit = arit;

  any_iterator<double, std::random_access_iterator_tag> arit_to_double;
  any_iterator<int, std::bidirectional_iterator_tag> abit_to_int;
//  abit_to_int = arit_to_double; // error: everything other than traversal must be the exact same.

  //
  // Conversions between any_iterator types (other than non-const to const), this time with the variables being const.
  //

  any_random_access_iterator_to_int const carit(vector_of_ints.begin());
  any_iterator<int, std::bidirectional_iterator_tag> abit2(carit);
  abit2 = carit;
  ASSERT_RETURN_FALSE(abit2 == carit);
  ASSERT_RETURN_FALSE( *abit2 == *carit );
  // any_random_access_iterator_to_int const carit2(cabit);  // error: cannot convert from bidirectional to random access iterator

  any_iterator<int, std::bidirectional_iterator_tag> const cabit(carit);
  any_iterator<int, std::forward_iterator_tag> afit2 = cabit;
  afit2 = carit;
  
  any_iterator<int, std::forward_iterator_tag> const cafit = cabit;
  any_iterator<int, boost::single_pass_traversal_tag> asit2 = cafit;
  asit2 = cabit;
  asit2 = carit;

  any_iterator<int, boost::single_pass_traversal_tag> const casit(cafit);
  any_iterator<int, boost::incrementable_traversal_tag> aniit2 = casit;
  aniit2 = cafit;
  aniit2 = cabit;
  aniit2 = carit;

  any_iterator<double, std::random_access_iterator_tag> const carit_to_double;
//  any_iterator<int, std::bidirectional_iterator_tag> const cabit_to_int = carit_to_double;  // error: everything other than traversal must be the exact same.

  //
  // Input and output iterators
  //
  
  typedef any_iterator<
    char,
    boost::single_pass_traversal_tag,
    char const,
    ptrdiff_t
  >
  any_input_iterator_to_char;

  std::istreambuf_iterator<char> std_in_it(std::cin.rdbuf());
  any_input_iterator_to_char an_input_iterator_to_char(std_in_it);

  std::istreambuf_iterator<char> eos;
  any_input_iterator_to_char eos_iterator(eos);

  // Uncomment the next two lines for an interactive test. Enter char's, followed by ENTER.
  // Enter ^Z ENTER to quit.
//   std::ostreambuf_iterator<char> std_out_it_1(std::cout.rdbuf());
//   std::copy(an_input_iterator_to_char, eos_iterator, std_out_it_1);

//  --an_input_iterator_to_char; // error: input iterators do not define this operator
//  an_input_iterator_to_char--; // error: input iterators do not define this operator
//  char c = an_input_iterator_to_char[42]; // error: input iterators do not define this operator
//  an_input_iterator_to_char = an_input_iterator_to_char + 42; // error: input iterators do not define this operator
//  an_input_iterator_to_char += 42; // error: input iterators do not define this operator

  typedef any_iterator<
    char,
    boost::incrementable_traversal_tag,
    std::ostreambuf_iterator<char, std::char_traits<char> > &,
    ptrdiff_t
  >
  any_output_iterator_to_char;

  std::ostringstream o_strm;
  std::ostreambuf_iterator<char> std_out_it_2(o_strm.rdbuf());
  any_output_iterator_to_char an_output_iterator_to_char(std_out_it_2);

  *an_output_iterator_to_char = 'B';
  *an_output_iterator_to_char = 'i';
  *an_output_iterator_to_char = 'n';
  *an_output_iterator_to_char = 'g';
  *an_output_iterator_to_char = 'o';
  *an_output_iterator_to_char = '!';
  *an_output_iterator_to_char = '\n';
  BOOST_CHECK_EQUAL( o_strm.str() , "Bingo!\n" );

  // Uncomment the next line for an interactive test. Enter char's, followed by ENTER.
  // Enter ^Z ENTER to quit.
//   std::copy(an_input_iterator_to_char, eos_iterator, an_output_iterator_to_char);

  // Make sure that the bug reported by Sergei Politov is fixed.
  //
  typedef any_iterator<const int, std::forward_iterator_tag> sergei_const_iterator;
  typedef any_iterator<int, std::forward_iterator_tag> sergei_iterator;
  std::vector<int> v(1);
  const sergei_const_iterator i = sergei_const_iterator(v.begin());
  const sergei_iterator j = sergei_iterator(v.end());
  ASSERT_RETURN_FALSE( i != j );
  
  // Demo the evil pitfall with type erasure: type erasure erases interoperability!
  //
  {    
    std::vector<int> int_vector;
    std::vector<int>::iterator it = int_vector.begin();
    std::vector<int>::const_iterator cit = int_vector.begin();
    ASSERT_RETURN_FALSE(it == cit);

    typedef any_iterator<int, boost::random_access_traversal_tag,  int const &> random_access_const_iterator_to_int;
    random_access_const_iterator_to_int ait_1(it);
    random_access_const_iterator_to_int ait_2(cit);
    
    // Bad comparison! Behaves like comparing completely unrelated iterators!
//    ait_1 == ait_2; 
  }

  return true;
}
