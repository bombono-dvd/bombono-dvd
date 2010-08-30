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

#include <mlib/range/adaptor/adjacent_filtered.hpp>
#include <mlib/range/algorithm/copy.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include <mlib/range/algorithm_ext/push_back.hpp>

namespace
{
    void adjacent_filtered_example_test()
    {
        using namespace boost::assign;
        using namespace boost::adaptors;

        std::vector<int> input;
        input += 1,1,2,2,2,3,4,5,6;

        //boost::copy(
        //    input | adjacent_filtered(std::not_equal_to<int>()),
        //    std::ostream_iterator<int>(std::cout, ","));

        std::vector<int> reference;
        reference += 1,2,3,4,5,6;

        std::vector<int> test;
        boost::push_back(test, input | adjacent_filtered(std::not_equal_to<int>()));

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }
}


BOOST_AUTO_TEST_CASE( test_range_adjacent_filtered_example )
{
    adjacent_filtered_example_test();
}

