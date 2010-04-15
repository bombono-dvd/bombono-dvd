//
// mgui/project/menu-actions.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2010 Ilya Murav'jov
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

#ifndef __MGUI_PROJECT_MENU_ACTIONS_H__
#define __MGUI_PROJECT_MENU_ACTIONS_H__

#include "menu-browser.h"

#include <mbase/project/table.h>
#include <mgui/rectlist.h>
#include <mgui/menu-rgn.h>

namespace Project
{

RefPtr<MenuStore> CreateMenuStore(MenuList& ml);
RefPtr<MenuStore> CreateEmptyMenuStore();

void PublishMenu(const Gtk::TreeIter& itr, RefPtr<MenuStore> ms, Menu mn);
void PublishMenuStore(RefPtr<MenuStore> ms, MenuList& ml);

// меню не должно зависеть от неоткрытых меню
void OpenPublishMenu(const Gtk::TreeIter& itr, RefPtr<MenuStore> ms, Menu mn);

// перерисовать все меню в ответ на изменения в меню changed_mn
void RenderMenuSystem(Menu changed_mn, RectListRgn& rct_lst);

bool IsMenuToBe4_3();

} // namespace Project

bool ReDivideRects(RectListRgn& rct_lst, MenuRegion& menu_rgn);

#endif // #ifndef __MGUI_PROJECT_MENU_ACTIONS_H__

