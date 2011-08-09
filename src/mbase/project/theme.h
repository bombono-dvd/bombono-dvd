//
// mbase/project/theme.h
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

#ifndef __MBASE_PROJECT_THEME_H__
#define __MBASE_PROJECT_THEME_H__

//#include <vector>

#include "const.h"

#include <mlib/string.h>
#include <mlib/filesystem.h>

namespace Project
{

struct ThemeDirList: public Str::List, public Singleton<ThemeDirList>
{
    ThemeDirList();
};

void AddSrcDirs(Str::List& dirs, const char* dname);

// по имени темы выдать ее путь
fs::path FindThemePath(const std::string& theme_name);
inline std::string ThemeOrDef(const std::string& theme_name)
{
    // :TODO!!!: упростить/вынести на клиентский уровень
    return theme_name.empty() ? std::string("rect") : theme_name ;
}
// вернуть список тем
void GetThemeList(Str::List& t_lst);

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_THEME_H__

