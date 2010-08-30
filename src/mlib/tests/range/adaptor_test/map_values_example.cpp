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

#include <mlib/range/adaptor/map.hpp>
#include <mlib/range/algorithm/copy.hpp>
#include <mlib/range/algorithm_ext/push_back.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

namespace 
{
    void map_values_example_test()
    {
        using namespace boost::assign;
        using namespace boost::adaptors;

        std::map<int,int> input;
        for (int i = 0; i < 10; ++i)
            input.insert(std::make_pair(i, i * 10));

        //boost::copy(
        //    input | map_values,
        //    std::ostream_iterator<int>(std::cout, ","));

        
        std::vector<int> reference;
        reference += 0,10,20,30,40,50,60,70,80,90;

        std::vector<int> test;
        boost::push_back(test, input | map_values);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }
}

BOOST_AUTO_TEST_CASE( test_range_map_values_example_test )
{
    map_values_example_test();
}

