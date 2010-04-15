//
// mbase/composite/composite.cpp
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

#include "component.h"

namespace Composition {

FramedObj::FramedObj(const char* thm_name, Rect plc): pstrLink(this)
{ 
    //Load(thm_path, plc);
    SetPlacement(plc);
    thmName = thm_name;
}

void ListObj::Clear()
{
    for( Itr iter=objArr.begin(), end=objArr.end(); iter != end; ++iter )
        delete *iter;

    objArr.clear();
}

void ListObj::Clear(Object* obj)
{
    for( Itr iter=objArr.begin(), end=objArr.end(); iter != end; ++iter )
        if( *iter == obj )
        {
            objArr.erase(iter);
            delete obj;
            break;
        }
}

void ListObj::Accept(ObjVisitor& vis)
{
    for( Itr iter=objArr.begin(), end=objArr.end(); iter != end; ++iter )
    {
        //vis.Visit(**iter);
        (**iter).Accept(vis);
    }
}

} // namespace Composition


