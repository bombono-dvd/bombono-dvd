//
// mbase/project/tests/menu-utils.cpp
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

#include <mbase/tests/_pc_.h>

#include "menu-utils.h"
#include <mbase/project/archieve.h>
#include <mbase/project/srl-common.h>

#include <mlib/sdk/logger.h>

namespace Project
{


//
// SerializeMedias()
// 

void DbSerializeMediasImpl(Archieve& ar)
{
    ar & NameValue("Medias", AData().GetML());
}

void LoadProjectMedias(const std::string& fname, const std::string& cur_dir)
{
    try
    {
        AData().LoadWithFnr(fname, DbSerializeMediasImpl, cur_dir);
    }
    catch (const std::exception& err)
    {
        LOG_ERR << "Project loading is failed (" << fname << "): " << err.what() << " !" << io::endl;
        AData().Clear();
    }
}

bool SaveProjectMedias(const std::string& fname, const std::string& cur_dir)
{
    AData().SetProjectFName(fname, cur_dir);
    return AData().SaveWithFnr(DbSerializeMediasImpl);
}

} // namespace Project

