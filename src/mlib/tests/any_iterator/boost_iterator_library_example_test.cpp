//  (C) Copyright Thomas Becker 2005. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Revision History
// ================
//
// 15 Mar 2008 (Thomas Becker) Created

// Includes
// ========

#include <mlib/tests/_pc_.h>

#include <mlib/any_iterator/any_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <iostream>
#include <sstream>

#define ASSERT_RETURN_FALSE( b ) if( ! (b) ){ return false; }

using namespace IteratorTypeErasure;

///////////////////////////////////////////////////////////////////////////////////////////
//
// The example that the boost iterator library documentation uses is
// interesting insofar as the value type of the iterator they present
// is an abstract base class. When this iterator is assigned to an
// any_iterator instantiation with the same value type, a flaw in 
// boost::is_convertible is revealed: boost::is_convertible<X, X>::value
// is false when X is an abstract base class. (Doug Gregor assures me that
// this will be fixed in the Convertible concept.) I have now fixed the
// any_iterator so that it works around that problem. This file tests that
// fix.
//
///////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
//
struct node_base
{
  node_base() : m_next(0) {}

  // Each node manages all of its tail nodes
  virtual ~node_base() { delete m_next; }

  // Access the rest of the list
  virtual node_base* next() const { return m_next; }

  // print to the stream
  virtual void print(std::ostream& s) const = 0;

  void append(node_base* p)
  {
    if (m_next)
      m_next->append(p);
    else
      m_next = p;
  }

private:
  node_base* m_next;
};

struct gratuitous_node_base : public node_base
{
public:
  gratuitous_node_base() : node_base() {}
  virtual gratuitous_node_base* next() const { return dynamic_cast<gratuitous_node_base*>(node_base::next()); }
};

struct another_gratuitous_node_base : public gratuitous_node_base
{
public:
  another_gratuitous_node_base() : gratuitous_node_base() {}
  virtual another_gratuitous_node_base* next() const { return dynamic_cast<another_gratuitous_node_base*>(gratuitous_node_base::next()); }
};

///////////////////////////////////////////////////////////////////////////////////////////
//
template <class T>
struct node : gratuitous_node_base
{
  node(T x)
    : m_value(x)
  {}

  void print(std::ostream& s) const 
  { 
    s << this->m_value; 
    if(next())
      next()->print(s);
  }

private:
  T m_value;
};

///////////////////////////////////////////////////////////////////////////////////////////
//
inline std::ostream& operator<<(std::ostream& s, node_base const& n)
{
  n.print(s);
  return s;
}

template <class Value>
class node_iter_impl : 
  public boost::iterator_facade<
    node_iter_impl<Value>
    , Value
    , boost::forward_traversal_tag
  >
{
public:
  node_iter_impl()
    : m_node(0) {}

  explicit node_iter_impl(Value* p)
    : m_node(p) {}

  template <class OtherValue>
  node_iter_impl(node_iter_impl<OtherValue> const& other)
    : m_node(other.m_node) {}

private:
  friend class boost::iterator_core_access;
  template <class> friend class node_iter_impl;

  template <class OtherValue>
  bool equal(node_iter_impl<OtherValue> const& other) const
  {
    return this->m_node == other.m_node;
  }

  void increment()
  { m_node = m_node->next(); }

  Value& dereference() const
  { return *m_node; }

  Value* m_node;
};
//
typedef node_iter_impl<gratuitous_node_base> node_iterator;
typedef node_iter_impl<gratuitous_node_base const> node_const_iterator;

template<class Iterator>
static std::string print_range_to_str(Iterator& run, Iterator& end)
{
    std::stringstream strm;
    while(true)
    {
        strm << *run << ";" << std::flush;
        if(++run == end)
            break;
        if(++run == end)
            break;
    }
    return strm.str();
}

template<class Iterator>
static void check_range(Iterator& run, Iterator& end)
{
    //std::cout << *run << "\n";
    //std::cout << "!?" << print_range_to_str(run, end) << "!?" << std::endl;
    BOOST_CHECK_EQUAL( print_range_to_str(run, end), "42 43 44;43 44;44;" );
}

///////////////////////////////////////////////////////////////////////////////////////////
//
void boost_iterator_library_example_test()
{
  gratuitous_node_base* root = new node<int>(42);
  gratuitous_node_base* n1 = root;
  gratuitous_node_base* n2 = new node<char>(' ');
  n1->append(n2);
  n1 = new node<int>(43);
  n2->append(n1);
  n2 = new node<char>(' ');
  n1->append(n2);
  n1 = new node<int>(44);
  n2->append(n1);

  node_iterator run(root);
  node_iterator end;
  check_range(run, end);

  // First any_iterator: same value type gratuitous_node_base as the node iterator
  //
  {
    node_iterator run(root);
    any_iterator<gratuitous_node_base, boost::forward_traversal_tag> any_run(run);

    node_iterator end;
    any_iterator<gratuitous_node_base, boost::forward_traversal_tag> any_end(end);

    check_range(any_run, any_end);
  }

  // Second any_iterator: value type is node_iterator, an abstract base class of the
  // abstract gratuitous_node_base.
  //
  {
    node_iterator run(root);
    any_iterator<node_base, boost::forward_traversal_tag> any_run(run);

    node_iterator end;
    any_iterator<node_base, boost::forward_traversal_tag> any_end(end);

    check_range(any_run, any_end);
  }

  // Third any_iterator: same value type "gratuitous_node_base const" as the node iterator
  //
  // Also test the conversion from non-const to const any_iterator here.
  {
    node_const_iterator run(root);
    any_iterator<gratuitous_node_base const, boost::forward_traversal_tag> any_run(run);

    node_const_iterator end;
    any_iterator<gratuitous_node_base const, boost::forward_traversal_tag> any_end(end);
    check_range(any_run, any_end);

    node_iterator run_non_const(root);
    any_iterator<gratuitous_node_base, boost::forward_traversal_tag> any_run_non_const(run_non_const);

    node_iterator end_non_const;
    any_iterator<gratuitous_node_base, boost::forward_traversal_tag> any_end_non_const(end_non_const);

    any_run = any_run_non_const;
    // any_run_non_const = any_run; // error: converse not allowed
    any_end = any_end_non_const;
    check_range(any_run, any_end);
  }

  // Fourth any_iterator: value type "node_base const", where node_base is an abstract base class of the
  // abstract gratuitous_node_base
  //
  {
    node_const_iterator run(root);
    any_iterator<node_base const, boost::forward_traversal_tag> any_run(run);

    node_const_iterator end;
    any_iterator<node_base const, boost::forward_traversal_tag> any_end(end);
    check_range(any_run, any_end);
  }

  // Fifth any_iterator: value type is "gratuitous_node_base const", and the node iterator
  // is the non-const version of that (value type gratuitous_node_base).
  {
    node_iterator run(root);
    any_iterator<gratuitous_node_base const, boost::forward_traversal_tag> any_run(run);

    node_iterator end;
    any_iterator<gratuitous_node_base const, boost::forward_traversal_tag> any_end(end);
    check_range(any_run, any_end);
  }

  // Sixth any_iterator: value type "node_base const", where node_base is an abstract base class of the
  // abstract gratuitous_node_base, and the concrete iterator is non-const.
  //
  // Also, test conversion from const any to non-const any again.
  {
    node_iterator run(root);
    any_iterator<node_base const, boost::forward_traversal_tag> any_run(run);

    node_iterator end;
    any_iterator<node_base const, boost::forward_traversal_tag> any_end(end);
    check_range(any_run, any_end);

    node_iterator run_non_const(root);
    any_iterator<node_base, boost::forward_traversal_tag> any_run_non_const(run_non_const);

    node_iterator end_non_const;
    any_iterator<node_base, boost::forward_traversal_tag> any_end_non_const(end_non_const);

    any_run = any_run_non_const;
    // any_run_non_const = any_run; // error: converse not allowed
    any_end = any_end_non_const;
    check_range(any_run, any_end);
  }

  // This does not work. Value types inherit the wrong way round.
  //
  {
    node_iterator run(root);
//    any_iterator<another_gratuitous_node_base, boost::forward_traversal_tag> any_run(run);
  }

  delete root;
}
