//
// mgui/render/common.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#include <mgui/_pc_.h>

#include "common.h"

#include <mlib/lambda.h>

typedef boost::function<RefPtr<Gdk::Pixbuf>(bool)> ShotBoolFunctor;

RefPtr<Gdk::Pixbuf> GetLRShotImpl(ShotFunctor s_fnr, bool is_exceed)
{
    if( is_exceed )
        return MakeColor11Image(TRANS_CLR);
    return s_fnr();
}

RefPtr<Gdk::Pixbuf> GetLimitedRecursedShot(ShotFunctor s_fnr, int& rd)
{
    using namespace boost;
    return LimitedRecursiveCall<RefPtr<Gdk::Pixbuf> >( lambda::bind(&GetLRShotImpl, s_fnr, lambda::_1),
                                                       rd );
}

RefPtr<Gdk::Pixbuf> GetInteractiveLRS(ShotFunctor s_fnr)
{
    static int RecurseDepth = 0;
    return GetLimitedRecursedShot(s_fnr, RecurseDepth);
}

RefPtr<Gdk::Pixbuf> MakeColor11Image(int clr)
{
    RefPtr<Gdk::Pixbuf> pix = Make11Image();
    pix->fill(clr);
    return pix;
}

