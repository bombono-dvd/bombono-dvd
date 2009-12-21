//
// mgui/author/output.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009 Ilya Murav'jov
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

#ifndef __MGUI_AUTHOR_OUTPUT_H__
#define __MGUI_AUTHOR_OUTPUT_H__

#include "execute.h"

#include <mgui/project/mconstructor.h>

namespace Project
{
// возвращает функтор, который надо выполнить после RunWindow() (но перед удалением окна)
ActionFunctor PackOutput(ConstructorApp& app, const std::string& prj_fname);

//Gtk::Button& FillBuildButton(Gtk::Button& btn, bool not_started);
void PackProgressBar(Gtk::VBox& vbox, Author::ExecState& es);
Gtk::Label& FillAuthorLabel(Gtk::Label& lbl, const std::string& name, bool is_left = true);

} // namespace Project

namespace Author
{

void OnDVDBuild(Gtk::FileChooserButton& ch_btn);

const int DVD5Size = 4490; // MB, размер в 1024^3; = 4,7*1000^3

class ConsoleOF: public OutputFilter
{
    public:

    virtual void  SetStage(Stage stg)   { GetES().SetStatus(StageToStr(stg)); }
    virtual  int  GetDVDSize()          { return DVD5Size; }
    virtual void  SetProgress(double p) { GetES().SetIndicator(p); }
    protected:
    virtual void  OnSetParser()         { SetProgress(0); }
};

} // namespace Author

#endif // #ifndef __MGUI_AUTHOR_OUTPUT_H__

