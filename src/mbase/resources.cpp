//
// mbase/resources.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

//#include <mbase/_pc_.h>
#include "resources.h"

#include <mlib/filesystem.h>
#include <mlib/gettext.h>
#include <mlib/sdk/logger.h>

#include <glibmm/miscutils.h> // get_user_config_dir()

const char* GetInstallPrefix()
{
#ifdef INSTALL_PREFIX
    return INSTALL_PREFIX;
#else
    // работаем локально
    return 0;
#endif

}

#define DATADIR  "resources"

const std::string& GetDataDir()
{
    static std::string res_dir;
    if( res_dir.empty() )
    {
        const char* prefix = GetInstallPrefix();
        res_dir = prefix ? (fs::path(prefix) / "share" / "bombono" / DATADIR).string() : DATADIR ;
    }
    return res_dir;
}

bool CreateDirsQuiet(const fs::path& dir)
{
    bool res = true;
    std::string err_str;
    if( !CreateDirs(dir, err_str) )
    {
        res = false;
        LOG_ERR << "CreateDirsQuiet(): " << err_str << io::endl;
    }
    return res;
}

const std::string& GetConfigDir()
{
    static std::string cfg_dir;
    if( cfg_dir.empty() )
    {
        fs::path dir = fs::path(Glib::get_user_config_dir()) / "bombono-dvd";
        CreateDirsQuiet(dir);

        cfg_dir = dir.string();
    }
    return cfg_dir;
}

// :REFACTOR:
std::string DataDirPath(const std::string& fpath)
{
    return AppendPath(GetDataDir(), fpath);
}

