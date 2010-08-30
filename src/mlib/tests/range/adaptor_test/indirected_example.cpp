// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <mlib/tests/_pc_.h>

#include <mlib/range/adaptor/indirected.hpp>
#include <mlib/range/algorithm/copy.hpp>
#include <mlib/range/algorithm_ext/push_back.hpp>

#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <iostream>
#include <vector>


namespace 
{
    void indirected_example_test()
    {
        using namespace boost::adaptors;

        std::vector<boost::shared_ptr<int> > input;

        for (int i = 0; i < 10; ++i)
            input.push_back(boost::shared_ptr<int>(new int(i)));
        
        //boost::copy(
        //    input | indirected,
        //    std::ostream_iterator<int>(std::cout, ","));


        std::vector<int> reference;
        for (int i = 0; i < 10; ++i)
            reference.push_back(i);

        std::vector<int> test;
        boost::push_back(test, input | indirected);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }
}

BOOST_AUTO_TEST_CASE( test_range_indirected_example_test )
{
    indirected_example_test();
}

