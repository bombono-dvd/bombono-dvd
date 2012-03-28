/*
    Copyright 2005-2007 Adobe Systems Incorporated
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html)
*/

/*************************************************************************************************/

#include <mlib/tests/_pc_.h>

#include <mlib/sdk/asl_any_iter.h>

//#include <cassert>
#include <algorithm>
#include <list>
#include <vector>
#include <deque>

#include <iostream>

#include <boost/range/functions.hpp>

/*************************************************************************************************/

typedef adobe::poly<adobe::bidirectional_iter<int> > any_bd_iterator_to_int;
typedef adobe::poly<adobe::random_access_iter<int> > any_ra_iterator_to_int;

// Concrete (non-template) function
void reverse(any_bd_iterator_to_int first, any_bd_iterator_to_int last)
{
    any_ra_iterator_to_int* first_ra = adobe::poly_cast<any_ra_iterator_to_int*>(&first);
    any_ra_iterator_to_int* last_ra  = adobe::poly_cast<any_ra_iterator_to_int*>(&last);

    if(first_ra && last_ra) 
        std::reverse(*first_ra, *last_ra);
    else 
        std::reverse(first, last);
}

BOOST_AUTO_TEST_CASE( test_adobe_any_iterator )
{
    const int a[] = { 0, 1, 2, 3, 4, 5 };
    
    std::list<int> l(boost::begin(a), boost::end(a));
    reverse(any_bd_iterator_to_int(boost::begin(l)), any_bd_iterator_to_int(boost::end(l)));
    
    std::vector<int> v(boost::begin(a), boost::end(a));
    any_ra_iterator_to_int begin_ra(boost::begin(v));
    any_ra_iterator_to_int end_ra(boost::end(v));
    reverse(any_bd_iterator_to_int(begin_ra), any_bd_iterator_to_int(end_ra));
    
    std::deque<int> d(boost::begin(a), boost::end(a));
    begin_ra = any_ra_iterator_to_int(boost::begin(d));
    end_ra = any_ra_iterator_to_int(boost::end(d));
    reverse(any_bd_iterator_to_int(begin_ra), any_bd_iterator_to_int(end_ra));
    
    any_bd_iterator_to_int i1((int*)NULL); // default constructor
    any_bd_iterator_to_int i2(i1); // copy empty constructor
    BOOST_CHECK(i1 == i2);
    i1 = any_bd_iterator_to_int(boost::begin(l)); // assignment to empty
    BOOST_CHECK(i1 == any_bd_iterator_to_int(boost::begin(l)));
    i2 = any_bd_iterator_to_int(boost::end(l));
    i1 = i2;
    BOOST_CHECK(i1 == i2);
    BOOST_CHECK(i1.cast<std::list<int>::iterator>() == boost::end(l));
}

typedef adobe::bidirectional_iter<int> any_iterator_to_int;

// Concrete (non-template) function
void reverse_and_print(any_iterator_to_int first, any_iterator_to_int last)
{
    std::reverse(first, last);

    bool PrintRange = false;
    if( PrintRange )
    {
        while (first != last)
        {
            std::cout << *first << " ";
            ++first;
        }
        std::cout << std::endl;
    }
    else
    {
        for( int i=0; i<6 ; ++i, ++first )
        {
            BOOST_CHECK( first != last );
            BOOST_CHECK_EQUAL( *first, 5-i );
        }
        BOOST_CHECK( first == last );
    }
}

BOOST_AUTO_TEST_CASE( test_adobe_any_iterator_example )
{
    const int a[] = { 0, 1, 2, 3, 4, 5 };
    
    std::list<int> l(boost::begin(a), boost::end(a));
    reverse_and_print(any_iterator_to_int(boost::begin(l)), any_iterator_to_int(boost::end(l)));
    
    std::vector<int> v(boost::begin(a), boost::end(a));
    reverse_and_print(any_iterator_to_int(boost::begin(v)), any_iterator_to_int(boost::end(v)));
    
    std::deque<int> d(boost::begin(a), boost::end(a));
    reverse_and_print(any_iterator_to_int(boost::begin(d)), any_iterator_to_int(boost::end(d)));
}

