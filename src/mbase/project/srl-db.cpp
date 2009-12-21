//
// mbase/project/srl-db.cpp
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

#include <mbase/_pc_.h>

#include "table.h"
#include "archieve.h"
#include "srl-common.h"

#include <mlib/sdk/logger.h>

namespace Project
{

static void Save(Archieve& ar, MenuList& ml)
{
    for( MenuList::Itr itr = ml.Beg(), end = ml.End(); itr != end; ++itr )
        ar << NameValue("Menu", **itr);
}

NameValueT<MenuMD> LoadMenu(MenuList& ml)
{
    Menu mn(new MenuMD);
    ml.Insert(mn);

    return NameValue("Menu", *mn);
}

static void Load(Archieve& ar, MenuList& ml)
{
    using namespace boost;
    ArchieveFunctor<MenuMD> fnr =
        MakeArchieveFunctor<MenuMD>( lambda::bind(&LoadMenu, boost::ref(ml)) );
    LoadArray(ar, fnr);
}

APROJECT_SRL_SPLIT_FREE(MenuList)

class DelayedRefLoading
{
    public:

    DelayedRefLoading(): arr(AData().GetData<MediaRefArr>())
    {
        arr.clear();
    }

   ~DelayedRefLoading()
    {
        if( std::uncaught_exception() )
            arr.clear();

        if( !arr.empty() ) // только в случае загрузки будет непустым
        {
            for( MediaRefArr::iterator itr = arr.begin(), end = arr.end(); itr != end; ++itr )
            {
                MediaItem& mi = *(itr->first);
                mi = Ref2Media(itr->second);
            }
            arr.clear();
        }
    }

    protected:
        MediaRefArr& arr;
};

void DbSerializeProjectImpl(Archieve& ar)
{
    DelayedRefLoading drl;

    // секция "Globals"
    {
        ArchieveStackFrame asf(ar, "Globals");

        ar & NameValue("PAL", AData().PalTvSystem());
        ar & NameValue("DefMenuParams", AData().GetDefMP());
        SerializeReference(ar, "First-Play", AData().FirstPlayItem());
    }

    ar & NameValue("Medias", AData().GetML());
    ar & NameValue("Menus",  AData().GetMN());
}

void ADatabase::Load(const std::string& fname,
                     const std::string& cur_dir) throw (std::exception)
{
    try
    {
        LoadWithFnr(fname, DbSerializeProjectImpl, cur_dir);
    }
    catch (const std::exception& err)
    {
        LOG_ERR << "Project loading (ADatabase::Load) is failed (" << fname << "): " << err.what() << " !" << io::endl;
        AData().Clear();
    }
}

bool ADatabase::Save()
{
    return AData().SaveWithFnr(DbSerializeProjectImpl);
}

bool ADatabase::SaveAs(const std::string& fname, const std::string& cur_dir)
{
    SetProjectFName(fname, cur_dir);
    return Save();
}

} // namespace Project

