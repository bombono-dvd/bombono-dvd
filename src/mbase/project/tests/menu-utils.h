//
// mbase/project/tests/menu-utils.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#ifndef __MBASE_PROJECT_TESTS_MENU_UTILS_H__
#define __MBASE_PROJECT_TESTS_MENU_UTILS_H__

#include <mbase/project/menu.h>
#include <mbase/project/table.h>

namespace Project {

//
// SerializeMedias()
// 

void LoadProjectMedias(const std::string& fname,
                       const std::string& cur_dir = std::string());
bool SaveProjectMedias(const std::string& fname,
                       const std::string& cur_dir = std::string());

// загрузчик базы
class AMediasLoader
{
    public:
                AMediasLoader(const std::string& path = std::string())
                { 
                    if( !path.empty() )
                        LoadProjectMedias(path.c_str());
                    else
                        AData().Clear();
                }
               ~AMediasLoader()
                { AData().Clear(); }
    protected:
};

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_TESTS_MENU_UTILS_H__

