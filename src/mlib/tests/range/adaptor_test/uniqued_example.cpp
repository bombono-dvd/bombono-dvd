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

#include <mlib/range/adaptor/uniqued.hpp>
#include <mlib/range/algorithm/copy.hpp>
#include <mlib/range/algorithm_ext/push_back.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

namespace
{
    void uniqued_example_test()
    {
        using namespace boost::assign;
        using namespace boost::adaptors;

        std::vector<int> input;
        input += 1,1,2,2,2,3,4,5,6;

        //boost::copy(
        //    input | uniqued,
        //    std::ostream_iterator<int>(std::cout, ","));

        std::vector<int> reference;
        reference += 1,2,3,4,5,6;

        std::vector<int> test;
        boost::push_back( test, input | uniqued );

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }
}

BOOST_AUTO_TEST_CASE( test_range_uniqued_example_test )
{
    uniqued_example_test();
}

