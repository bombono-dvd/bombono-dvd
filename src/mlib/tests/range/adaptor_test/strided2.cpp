// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//

// This test was added due to a report that the Range Adaptors:
// 1. Caused havoc when using namespace boost::adaptors was used
// 2. Did not work with non-member functions
// 3. The strided adaptor could not be composed with sliced
//
// None of these issues could be reproduced on GCC 4.4, but this
// work makes for useful additional test coverage since this
// uses chaining of adaptors and non-member functions whereas
// most of the tests avoid this use case.

#include <mlib/tests/_pc_.h>

#include <mlib/range/adaptor/strided.hpp>
#include <mlib/range/adaptor/sliced.hpp>
#include <mlib/range/adaptor/transformed.hpp>
#include <mlib/range/irange.hpp>
#include <mlib/range/algorithm_ext.hpp>

#include <boost/assign.hpp>

#include <algorithm>
#include <vector>

namespace boost
{
    namespace
    {
        int times_two(int x) { return x * 2; }

        void strided_test2()
        {
            using namespace boost::adaptors;
            using namespace boost::assign;
            std::vector<int> v;
            boost::push_back(v, boost::irange(0,10));
            std::vector<int> z;
            boost::push_back(z, v | sliced(2,6) | strided(2) | transformed(&times_two));
            std::vector<int> reference;
            reference += 4,8;
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                z.begin(), z.end() );
        }
    }
}

BOOST_AUTO_TEST_CASE( test_range_strided_test2 )
{
    boost::strided_test2();
}

