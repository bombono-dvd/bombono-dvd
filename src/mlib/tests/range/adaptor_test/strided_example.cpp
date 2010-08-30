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

#include <mlib/range/adaptor/strided.hpp>
#include <mlib/range/algorithm/copy.hpp>
#include <mlib/range/algorithm_ext/push_back.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

namespace 
{
    void strided_example_test()
    {
        using namespace boost::adaptors;
        using namespace boost::assign;

        std::vector<int> input;
        input += 1,2,3,4,5,6,7,8,9,10;
        
        //boost::copy(
        //    input | strided(2),
        //    std::ostream_iterator<int>(std::cout, ","));


        std::vector<int> reference;
        reference += 1,3,5,7,9;

        std::vector<int> test;
        boost::push_back(test, input | strided(2));

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }
}

BOOST_AUTO_TEST_CASE( test_range_strided_example_test )
{
    strided_example_test();
}

