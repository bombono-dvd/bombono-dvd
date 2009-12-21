//
// mlib/tests/test_dataware.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#include <mlib/dataware.h>
#include <mlib/tests/test_common.h>

template<int i>
struct MyData: public DWConstructorTag, public set_bool
{
    public:
                        MyData(DataWare&) {}

    static const int val = i;
};

BOOST_AUTO_TEST_CASE( DataWareTest )
{
    bool b  = false;
    bool b_wo_ctor = false;
    {
        DataWare dware;
        BOOST_CHECK_EQUAL( dware.Size(), 0 );

        // *
        MyData<1>& dat = dware.GetData<MyData<1> >();
        BOOST_CHECK_EQUAL( dware.Size(), 1 );

        dat.set_new_bool(b);
        BOOST_CHECK(b);

        // **, без конструктора
        set_bool& sb = dware.GetData<set_bool>();
        sb.set_new_bool(b_wo_ctor);

        MyData<2>& dat_2 = dware.GetData<MyData<2> >();
        BOOST_CHECK_EQUAL( dware.Size(), 3 );
        BOOST_CHECK( (void*)&dat != (void*)&dat_2 );

        // *
        MyData<1>& dat_ = dware.GetData<MyData<1> >();
        BOOST_CHECK_EQUAL( &dat, &dat_ );
        BOOST_CHECK_EQUAL( dware.Size(), 3 );

        // **
        set_bool& sb_ = dware.GetData<set_bool>();
        BOOST_CHECK_EQUAL( &sb, &sb_ );

        dware.GetData<int>();
        dware.Clear();
        BOOST_CHECK_EQUAL( dware.Size(), 0 );
    }
    BOOST_CHECK(!b);
    BOOST_CHECK(!b_wo_ctor);

    // именованные локальные данные
    b = true;
    bool b2 = true;
    {
        DataWare dware;
        set_bool& sb = dware.GetData<set_bool>("tag");
        sb.set_new_bool(b);

        BOOST_CHECK_EQUAL( dware.Size(), 0 );
        dware.Clear();
        BOOST_CHECK_EQUAL( dware.Size(), 0 );
        BOOST_CHECK_EQUAL( dware.Size("tag2"), 0 );
        BOOST_CHECK( b );

        // * повторяемость
        DataWare& tag_dware = dware.GetTaggedDW("tag");
        BOOST_CHECK_EQUAL( &sb, &tag_dware.GetData<set_bool>() );

        // * очистка
        BOOST_CHECK_EQUAL( tag_dware.Size(), 1 );
        dware.Clear("tag");
        BOOST_CHECK_EQUAL( tag_dware.Size(),  0 );
        BOOST_CHECK_EQUAL( dware.Size("tag"), 0 );

        // * деструктор
        set_bool& sb2 = dware.GetData<set_bool>("tag2");
        sb2.set_new_bool(b2);
    }
    BOOST_CHECK( !b );
    BOOST_CHECK( !b2 );

    // проверка инициализации скалярных типов по умолчанию
    // (int, float/double, bool, enum, указатели)
    // C++98, par. 8.5 (zero-initialization)
    {
        DataWare dware;
        bool b_f = dware.GetData<bool>(); // = false
        BOOST_CHECK( !b_f );

        int i = dware.GetData<int>(); // = 0
        BOOST_CHECK_EQUAL( i, 0 );

        double d = dware.GetData<double>(); // = 0
        BOOST_CHECK_EQUAL( d, 0 );

        int* p = dware.GetData<int*>(); // = 0
        BOOST_CHECK( !p );
    }

    // проверка DefFunctor
    {
        DataWare dware;
        bool& b_f = dware.GetData<bool, DefBoolean<false> >("0");
        BOOST_CHECK( !b_f );

        bool& b_t = dware.GetData<bool, DefBoolean<true> >("1");
        BOOST_CHECK( b_t );
        BOOST_CHECK( &b_t != &b_f );

        int i = dware.GetData<int, DefInteger<777> >();
        BOOST_CHECK_EQUAL( i, 777 );

        int* p = dware.GetData<int*, DefNull>();
        BOOST_CHECK( !p );
    }
}

