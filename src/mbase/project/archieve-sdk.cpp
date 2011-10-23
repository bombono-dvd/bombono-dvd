//
// mbase/project/archieve-sdk.cpp
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

#include <mbase/_pc_.h>

#include "archieve.h"
#include "archieve-sdk.h"
#include "table.h" // SaveFormattedUTF8Xml()

#include <mlib/string.h>

namespace Project
{

XMLSave::XMLSave(const char* root_tag): rootNode(doc.create_root_node(root_tag)) {}

void SaveXS(XMLSave& xs, const ArchieveFnr& afnr, int save_ver, const Glib::ustring& filename)
{
    Archieve ar(xs.rootNode, false);
    ar("Version", save_ver);
    afnr(ar);

    SaveFormattedUTF8Xml(xs.doc, filename);
}

void DoLoadArchieve(const std::string& fname, const ArchieveFnr& afnr, const char* root_tag)
{
    xmlpp::DomParser parser;
    try
    {
        parser.parse_file(fname);
    }
    catch(const std::exception& err)
    {
        // заменяем, потому что из сообщения непонятно, что произошло
        throw std::runtime_error(std::string(fname) + " is not existed or corrupted");
    }

    xmlpp::Element* root_node = parser.get_document()->get_root_node();
    if( root_node->get_name() != root_tag ) 
        throw std::runtime_error("The file is not " APROGRAM_PRINTABLE_NAME " document.");

    Archieve ar(root_node, true);

    std::string ver_str;
    ar("Version", ver_str);
    if( !Str::GetType(ar.loadVer, ver_str.c_str()) )
        ar.loadVer = 0;

    afnr(ar);
}

} // namespace Project

