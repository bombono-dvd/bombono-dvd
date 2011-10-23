//
// mbase/project/archieve-sdk.h
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

#ifndef __MBASE_PROJECT_ARCHIEVE_SDK_H__
#define __MBASE_PROJECT_ARCHIEVE_SDK_H__

#include "archieve-fwd.h"

namespace Project
{

void DoLoadArchieve(const std::string& fname, const ArchieveFnr& afnr, const char* root_tag);

struct XMLSave
{
    xmlpp::Document doc;
    xmlpp::Element* rootNode;
    
    XMLSave(const char* root_tag);
};

void SaveXS(XMLSave& xs, const ArchieveFnr& afnr, int save_ver, const Glib::ustring& filename);

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_ARCHIEVE_SDK_H__


