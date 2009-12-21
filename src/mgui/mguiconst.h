//
// mgui/mguiconst.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#ifndef __MGUI_MGUICONST_H__
#define __MGUI_MGUICONST_H__

#include <glibmm/refptr.h>

#include <mbase/project/const.h>
#include <mlib/function.h> // ActionFunctor

// "аббревиатура" для Glib/Gtk
using Glib::RefPtr;

// "аббревиатура" для Cairo
namespace CR
{
using Cairo::RefPtr;
using Cairo::ImageSurface;
using Cairo::Context;
};

// обнулить RefPtr прямо запрещается, потому так
template<typename T, template<typename R> class RefPtrT>
void ClearRefPtr(RefPtrT<T>& p)
{
    p = RefPtrT<T>(0);
}

template<typename T, template<typename R> class RefPtrT>
T* UnRefPtr(RefPtrT<T> p)
{
    return p.operator->();
}

// основное использование - когда надо передать как аргумент RefPtr, а
// у нас указатель (вроде this)
template<class GlibObj>
RefPtr<GlibObj> MakeRefPtr(GlibObj* obj)
{
    obj->reference();
    return RefPtr<GlibObj>(obj);
}

#endif // __MGUI_MGUICONST_H__

