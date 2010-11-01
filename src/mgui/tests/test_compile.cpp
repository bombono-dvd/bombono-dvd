//
// mgui/tests/test_compile.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008, 2010 Ilya Murav'jov
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

#include <mgui/tests/_pc_.h>

#include <mgui/render/rgba.h>
#include <mlib/lambda.h>
#include <boost/lambda/if.hpp>     // if_then

BOOST_AUTO_TEST_CASE( test_lambda )
{
    typedef RGBA::RgnPixelDrawer::DrwFunctor DrwFunctor;
    DrwFunctor drw_fnr;
    const RectListRgn r_lst;
    Rect cut_rct, plc_rct;

    // проверка компиляции
    // Особенности использования: 
    // - lambda::var - это шаблонная функция Lambda, специально сделанная для того, чтобы
    //   использовать обычные переменные в lambda-выражении (lambda::constant - для констант)
    // - (boost::)ref - это класс общего назначения (вне lambda), имитация ссылки на объект,
    //   но с возможностью присваивания; Lambda "знает" это класс, и во многих случаях для
    //   использования в lambda-выражении достаточно использовать ее (если не помогает,- тогда
    //   lambda::var)
    using namespace boost;
    std::for_each(r_lst.begin(), r_lst.end(), (
        lambda::var(cut_rct) = lambda::bind(Intersection<int>, lambda::_1, ref(plc_rct)),
        lambda::if_then( !lambda::bind(&RectT<int>::IsNull, lambda::var(cut_rct)),
            lambda::bind(&DrwFunctor::operator(), ref(drw_fnr), lambda::var(cut_rct)) )
                                              ) );
}


