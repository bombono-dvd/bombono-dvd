//
// mgui/author/script.h
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

#ifndef __MGUI_AUTHOR_SCRIPT_H__
#define __MGUI_AUTHOR_SCRIPT_H__

#include "execute.h"

#include <mbase/project/media.h>
#include <mbase/project/menu.h>
#include <mbase/composite/component.h>

#include <mlib/filesystem.h> // fs::path

#include <boost/function.hpp>

namespace Project
{

typedef boost::function<bool(VideoItem, int)> VideoFnr;
int ForeachVideo(VideoFnr fnr);

typedef boost::function<bool(Menu, int)> MenuFnr;
void ForeachMenu(MenuFnr fnr);

void AuthorMenus(const std::string& out_dir);
// run_all - включая запуск внешшней команды сборки
bool AuthorDVD(const std::string& out_dir);

#define AUTHOR_TAG "Authoring"

std::string MenuAuthorDir(Menu mn, int i, bool cnv_from_utf8 = true);
fs::path SConsAuxDir();

bool HasButtonLink(Comp::MediaObj& m_obj, std::string& targ_str);

class CheckAuthorMode
{
    public:
        static bool Flag; // спец. режим для тестов (доп. проверки)

        CheckAuthorMode(bool turn_on = true);
       ~CheckAuthorMode();
    protected:
        bool origVal;
};

bool IsMotion(Menu mn);
void ClearTaggedData(Menu mn, const char* tag);

} // namespace Project

namespace Author {

void Warning(const std::string& str);
void Info(const std::string& str, bool add_info_sign = true);
void Error(const std::string& str);

void FillSconsOptions(str::stream& scons_options, bool fill_def);
ExitData ExecuteSconsCmd(const std::string& out_dir, OutputFilter& of, 
                         Mode mod, const str::stream& scons_options);

io::pos VideoSizeSum();

} // namespace Author

#endif // #ifndef __MGUI_AUTHOR_SCRIPT_H__


