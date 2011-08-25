//
// mgui/project/menu-browser.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008, 2010 Ilya Murav'jov
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

#ifndef __MGUI_PROJECT_MENU_BROWSER_H__
#define __MGUI_PROJECT_MENU_BROWSER_H__

#include "browser.h"

#include <mbase/project/menu.h>

namespace Project
{

class MenuStore: public ObjectStore
{
    typedef ObjectStore MyParent;
    public:

            struct TrackFields : public Gtk::TreeModelColumnRecord
            {
                Gtk::TreeModelColumn<RefPtr<Gdk::Pixbuf> > thumbnail;
                Gtk::TreeModelColumn<Menu> menu;
    
                TrackFields() 
                { 
                    add(thumbnail);
                    add(menu);
                }
            };
            const TrackFields  columns;

                         MenuStore() { set_column_types(columns); }

  virtual     MediaItem  GetMedia(const Gtk::TreeIter& itr) const;

    protected:

        virtual    bool  row_drop_possible_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& data) const;
};

Menu GetMenu(RefPtr<MenuStore> ms, const Gtk::TreeIter& itr);

// обозреватель меню
class MenuBrowser : public ObjectBrowser
{
    typedef ObjectBrowser MyParent;
    public:
                       MenuBrowser(RefPtr<MenuStore> m_lst);

     RefPtr<MenuStore> GetMenuStore()
                       { return RefPtr<MenuStore>::cast_static(get_model()); }
    virtual      void  DeleteMedia();
};

Menu MakeMenu(const std::string& name, AspectFormat af);
void InsertMenuIntoBrowser(MenuBrowser& brw, Menu mn);
int MenusCnt();

fe::range<Menu> AllMenus();

} // namespace Project

#endif // #ifndef __MGUI_PROJECT_MENU_BROWSER_H__

