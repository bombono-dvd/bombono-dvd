//
// mgui/project/mconstructor.h
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

#ifndef __MGUI_PROJECT_MCONSTRUCTOR_H__
#define __MGUI_PROJECT_MCONSTRUCTOR_H__

#include <mgui/init.h>
#include <mlib/dataware.h>
#include <mlib/resingleton.h>

namespace Project
{

struct SizeBar
{
           Gtk::Label szLbl;
    Gtk::ComboBoxText dvdTypes;
};

struct ConstructorApp: public ReSingleton<ConstructorApp>
{
      Gtk::Window  win;
             bool  askSaveOnExit;
             bool  isProjectChanged;

                  ConstructorApp();

  Gtk::Notebook&  BookContent() { return bookCont; }
  Gtk::Notebook&  BookTabs()    { return bookTabs; }
      Gtk::Menu&  GoMenu()      { return goMenu;   }
        SizeBar&  SB()          { return szBar;    }

           void   SetTabName(const std::string& name, int pos);

    protected:

        Gtk::VBox  vBox;
    Gtk::Notebook  bookCont; // рабочая область программы
    Gtk::Notebook  bookTabs; // закладки в области меню
        Gtk::Menu  goMenu;

          SizeBar  szBar;
};

inline ConstructorApp& Application() { return ConstructorApp::Instance(); }

#define APROJECT_NAME "Bombono DVD"

// насчет того, что возвращает - см. PackOutput()
ActionFunctor BuildConstructor(ConstructorApp& app, const std::string& prj_file_name);
void RunConstructor(const std::string& prj_file_name, bool ask_save_on_exit = true);

void PackMediasWindow(Gtk::Container& contr, RefPtr<MediaStore> md_store);
typedef boost::function<void(Gtk::Container&, MediaBrowser&)> MediasWindowPacker;
void PackMediasWindow(Gtk::Container& contr, RefPtr<MediaStore> ms, MediasWindowPacker pack_fnr);

void PackMenusWindow(Gtk::Container& contr, RefPtr<MenuStore> ms, RefPtr<MediaStore> md_store);

//
//
// 

void PackFullMBrowser(Gtk::Container& contr, MediaBrowser& brw);

} // namespace Project

void InitI18n();

#endif // #ifndef __MGUI_PROJECT_MCONSTRUCTOR_H__

