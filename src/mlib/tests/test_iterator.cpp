//
// mlib/tests/test_iterator.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

#include <mlib/tests/_pc_.h>

#include <mlib/foreach.h>

#include <mlib/range/any_range.h>
#include <mlib/range/filter.h>
#include <mlib/range/transform.h>
#include <mlib/range/irange.h>
#include <mlib/range/enumerate.h>

#include <mlib/string.h>
#include <mlib/stream.h>
#include <mlib/lambda.h>
#include <mlib/function.h>
#include <mlib/bind.h>

#include <boost/lexical_cast.hpp>
#include <boost/iterator/iterator_concepts.hpp> // RandomAccessTraversal

#include <vector>

struct is_positive_number {
    bool operator()(int x) { return x > 0; }
};

static int numbers_[] = { 0, -1, 4, -3, 5, 8, -2 };
static const int N = sizeof(numbers_)/sizeof(int);

BOOST_AUTO_TEST_CASE( test_iterator )
{
    typedef boost::filter_iterator<is_positive_number, int*> IterT;
    IterT beg = boost::make_filter_iterator(is_positive_number(), numbers_,     numbers_ + N);
    IterT end = boost::make_filter_iterator(is_positive_number(), numbers_ + N, numbers_ + N);

    str::stream strm;
    //for( IterT it = beg; it != end; ++it)
    //    io::cout << *it << " " << io::endl;
    BOOST_FOREACH(int i, boost::make_iterator_range(beg, end))
        strm << i;
    BOOST_CHECK_EQUAL( strm.str(), "458" );
}

///////////////////////////////////////////////////////////////////////////////

namespace {

typedef std::vector<int> int_vector;
int_vector IntArr(numbers_, numbers_ + N);
std::string IntArrStr("0;-1;4;-3;5;8;-2;");
std::string IntArrStr2("0;-1;8;-3;10;16;-2;");

}

template <class Type, class Range>
static void CheckRange(const Range& rng, const std::string& str)
{
    str::stream strm;
    boost_foreach( Type i, rng )
        strm << i << ";";
    BOOST_CHECK_EQUAL( strm.str(), str);
}

//
// transform
//

//static Rect Int2Rect(int& x) { return Rect(0, 0, x, x); }
static std::string Int2Str(int& x) { return boost::lexical_cast<std::string>(x); }

int MPTransform(int x, int mult)
{
    return x > 0 ? x*mult : x ;
}

struct MyltiplePositiveTransform {
    int mult;

    MyltiplePositiveTransform(int mult_): mult(mult_) {}

    // Два требования собираемости:
    // 1) определен результирующий тип через result_type
    // 2) оператор () должен быть константным (вот зачем такая ерунда, David Abrahams?)
    typedef int result_type;
    int operator()(int x) const 
    {
        return MPTransform(x, mult);
    }

    private:
    MyltiplePositiveTransform& operator=(const MyltiplePositiveTransform&);
};

template <class UnaryFunctor>
void CheckIntRange(UnaryFunctor fnr)
{
    fe::range<int> a_rng = fe::make_any( IntArr | fe::transformed(fnr) );
    CheckRange<int>(a_rng, IntArrStr2);
}


BOOST_AUTO_TEST_CASE( test_iterator_transform )
{
    CheckRange<int>(IntArr, IntArrStr);

    CheckRange<std::string>(fe::transform(IntArr, &Int2Str), IntArrStr);

    CheckIntRange(MyltiplePositiveTransform(2));

    boost::function<int(int)> fnr = bl::bind(&MPTransform, bl::_1, 2);
    CheckIntRange(fnr);

    // в отличие от Boost.Lambda (BBL) BB устанавливает result_type,
    // потому что в концепции библиотеки явно заложено определение типа
    // по первому аргументу
    CheckIntRange(bb::bind(&MPTransform, _1, 2));
}

// пример превращения категории итератора в гораздо более низкую по STL
typedef std::vector<int>::iterator int_iterator;
typedef boost::transform_iterator<int(*)(int), int_iterator> tr_iterator;
//BOOST_CONCEPT_ASSERT((boost::RandomAccessIterator<tr_iterator>));
BOOST_CONCEPT_ASSERT((boost_concepts::RandomAccessTraversal<tr_iterator>));

//
// filter
//

static bool IsPositive(int x) { return x > 0; }

static fe::range<int&> FilterPositive(int_vector& arr)
{
    return fe::make_any(fe::filter(arr, &IsPositive));
}

static fe::range<const int&> FilterPositive(const int_vector& arr)
{
    return fe::make_any(fe::filter(arr, &IsPositive));
}

BOOST_AUTO_TEST_CASE( test_iterator_filter )
{
    CheckRange<int>(FilterPositive(IntArr), "4;5;8;");

    int_vector arr2(IntArr);
    boost_foreach( int& i, FilterPositive(arr2) )
        i *= 2;

    CheckRange<int>(arr2, IntArrStr2);

    const int_vector& arr_cref = IntArr;
    CheckRange<int>(FilterPositive(arr_cref), "4;5;8;");
}

// 
// irange & enumerate
// 

BOOST_AUTO_TEST_CASE( test_irange )
{
    // irange
    int_vector arr;
    boost_foreach( int i, fe::irange(0, 5) )
        arr.push_back(i);
    for( int i=0; i<5 ; i++ )
        BOOST_CHECK( i == arr[i] );

    // index_value
    int val = 5;
    fe::index_value<int&> iv(1, val);
    BOOST_CHECK_EQUAL( iv.index(), 1   );
    BOOST_CHECK_EQUAL( iv.value(), val );
    int& r_val = iv.value();
    BOOST_CHECK_EQUAL( &r_val, &val );
    iv.index() = 2;
    iv.value() = 7;
    BOOST_CHECK_EQUAL( iv.index(), 2 );
    BOOST_CHECK_EQUAL( iv.value(), 7 );

    // enumerate
    boost_foreach( fe::index_value<int&> iv, arr | fe::enumerated(1) )
        BOOST_CHECK_EQUAL( iv.index()-1, iv.value() );
}

