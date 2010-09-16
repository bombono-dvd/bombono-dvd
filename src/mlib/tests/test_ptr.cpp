//
// mlib/tests/test_ptr.cpp
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

#include <utility> // std::pair

#include <mlib/ptr.h>
#include <mlib/tests/test_common.h>

// общая функция для проверки ptr::one, ptr::shared
template<template<class> class Ptr>
void TestPtr()
{
    bool b = false;

    // конструкторы
    {
        Ptr<set_bool> ptr1;
        Ptr<set_bool> ptr2((set_bool*)0);

        Ptr<set_bool> ptr3 = new set_bool(b);
        BOOST_CHECK( b );
    }
    BOOST_CHECK( !b );

    // get(), operator =
    {
        Ptr<set_bool> ptr;
        BOOST_CHECK( !ptr.get() );

        set_bool* p_sb = new set_bool(b); 
        ptr = p_sb;

        BOOST_CHECK( ptr.get() == p_sb );
        BOOST_CHECK( b );

        Ptr<set_bool> new_ptr(ptr);
        BOOST_CHECK( ptr.get() == 0 );
        BOOST_CHECK( new_ptr.get() == p_sb );
        BOOST_CHECK( b );
    }
    BOOST_CHECK( !b );

    // reset()
    {
        Ptr<set_bool> ptr = new set_bool(b); 

        ptr.reset((set_bool*)0);
        BOOST_CHECK( !b );

        ptr.reset( new set_bool(b) );
        BOOST_CHECK( b );
    }

    // if( ptr )
    // operator *
    {
        Ptr<int> ptr;
        if( ptr )
            BOOST_CHECK( false );

        int* p_int = new int(5);
        ptr = p_int;

        if( ptr )
            BOOST_CHECK( true );
        BOOST_CHECK( 5 == *ptr );
    }

    // operator ->
    {
        typedef std::pair<int, int> iint;
        Ptr<iint> pair_ptr = new iint(1,2);
        BOOST_CHECK( pair_ptr->first  == 1 );
        BOOST_CHECK( pair_ptr->second == 2 );
    }
}

// ptr::one
BOOST_AUTO_TEST_CASE( ptr_one )
{
    TestPtr<ptr::one>();

    bool b = false;

    // конструктор ptr::one(ptr::one&)
    {
        set_bool* p_sb = new set_bool(b);
        ptr::one<set_bool> ptr = p_sb;

        ptr::one<set_bool> new_ptr(ptr);
        BOOST_CHECK( !ptr.get() );
        BOOST_CHECK( new_ptr.get() == p_sb );
        BOOST_CHECK( b );
    }

    // release()
    {
        ptr::one<set_bool> ptr = new set_bool(b);

        set_bool* p_sb = ptr.release();
        BOOST_CHECK( p_sb && !ptr.get() );
        BOOST_CHECK( b );

        ptr = p_sb;
    }
}

// ptr::shared
BOOST_AUTO_TEST_CASE( ptr_shared )
{
    //TestPtr<boost::shared_ptr>();
    TestPtr<ptr::shared>();

    bool b = false;

    // конструктор ptr::shared(const ptr::shared&)
    {
        set_bool* p_sb = new set_bool(b);
        ptr::shared<set_bool> ptr = p_sb;

        ptr::shared<set_bool> new_ptr(ptr);
        BOOST_CHECK( ptr.get()     == p_sb );
        BOOST_CHECK( new_ptr.get() == p_sb );
        BOOST_CHECK( ptr.use_count() == 2 );
        BOOST_CHECK( b );

        ptr.reset();
        BOOST_CHECK( b );
        new_ptr.reset();
        BOOST_CHECK( !b );
    }
}

class intr_set_bool: public ptr::base, public set_bool {};

BOOST_AUTO_TEST_CASE( TestIntrusivePtr )
{
    typedef boost::intrusive_ptr<intr_set_bool> ptr_set_bool;
    typedef ptr::weak_intrusive<intr_set_bool>  wptr_set_bool;

    bool b = false;
    wptr_set_bool w_p;
    BOOST_CHECK( w_p.expired() );
    {
        //ptr_set_bool p_x(new intr_set_bool);
        ptr_set_bool p_x = new intr_set_bool;

        w_p = p_x;
        BOOST_CHECK( !w_p.expired() && (w_p.lock() == p_x) );

        wptr_set_bool w_p2 = p_x;
        BOOST_CHECK( w_p == w_p2 );

        p_x->set_new_bool(b);
        BOOST_CHECK( b );
        BOOST_CHECK_EQUAL( 1 , p_x->use_count() );

        {
            ptr_set_bool p_x2(p_x);
            BOOST_CHECK_EQUAL( 2 , p_x->use_count() );
        }

        BOOST_CHECK( b );
        p_x = ptr_set_bool();
        BOOST_CHECK( !b );
    }
    BOOST_CHECK( w_p.expired() );
    BOOST_CHECK( !b );
}


