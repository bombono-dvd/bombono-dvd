//
// mgui/menu-rgn.cpp
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

#include "menu-rgn.h"

RefPtr<Gdk::Pixbuf> CanvasBuf::FramePixbuf()
{
    Point sz(Size());
    return MakeSubPixbuf(Canvas(), sz);
}

// храним в именнованных данных, чтобы можно было делать очистку неименнованых
Project::MenuMD* GetOwnerMenu(Comp::Object* obj)
{
    return obj->GetData<Project::MenuMD*>("Project");
}

void SetOwnerMenu(Comp::Object* obj, Project::MenuMD* owner)
{
    obj->GetData<Project::MenuMD*>("Project") = owner;
}

void AcceptOnlyObject(Comp::Object* obj, GuiObjVisitor& g_vis)
{
    if( MenuRegion* mr = dynamic_cast<MenuRegion*>(obj) )
        // :KLUDGE: посещаем так, чтобы не обходить подобъекты
        g_vis.Visit(*mr);
    else
        obj->Accept(g_vis);
}

RectListRgn& GetRenderList(MenuRegion& m_rgn)
{
    return m_rgn.GetCanvasBuf().RenderList();
}


