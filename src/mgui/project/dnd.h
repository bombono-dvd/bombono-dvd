//
// mgui/project/dnd.h
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

#ifndef __MGUI_PROJECT_DND_H__
#define __MGUI_PROJECT_DND_H__

#include <mgui/sdk/dndtreeview.h>

#include <mlib/string.h>

inline void CheckSelFormat(const Gtk::SelectionData& selection_data)
{
    ASSERT_OR_UNUSED_VAR( selection_data.get_format() == 8, selection_data );
}

template<class DataType>
Gtkmm2ext::SerializedObjectPointers<DataType>& GetSOP(const Gtk::SelectionData& selection_data)
{
    typedef Gtkmm2ext::SerializedObjectPointers<DataType> SOPType;
    ASSERT( selection_data.get_length() == sizeof(SOPType*) );

    return **(SOPType**)selection_data.get_data();
}

namespace Project
{

#define DND_MI_NAME "AP/Project::MediaItem"
// тип DND для MediaItem из DnDTreeView
std::string MediaItemDnDTVType();
inline Gtk::TargetEntry MediaItemDnDTVTargetEntry()
{
    return Gtk::TargetEntry(MediaItemDnDTVType(), Gtk::TARGET_SAME_APP);
}

}  // namespace Project

std::string UriListDnDType();
typedef Glib::StringArrayHandle URIList;

void AddUriListTarget(std::list<Gtk::TargetEntry>& targets);

//////////////////////////////////////////////////////////////////////////
// DnD для URI

typedef std::list<Gtk::TargetEntry> DnDTargetList;
inline DnDTargetList& TextUriList()
{
    static DnDTargetList list;
    if( list.empty() )
    {
        // не нужен - похоже "text/uri-list" везде есть (и в Nautilus)
        //dropTargets.push_back (Gtk::TargetEntry ("text/plain"));
        AddUriListTarget(list);
        // не нужен - это только для бросания на рабочий стол
        //dropTargets.push_back (Gtk::TargetEntry ("application/x-rootwin-drop"));
    }
    return list;
}

typedef Str::List StringList;

typedef boost::function<void(const StringList&, const Point&)> UriDropFunctor;
void ConnectOnDropUris(Gtk::Widget& dnd_wdg, const UriDropFunctor& fnr);

inline void SetupURIDrop(Gtkmm2ext::DnDTreeViewBase& brw, const UriDropFunctor& fnr)
{
    brw.add_drop_targets(TextUriList());
    ConnectOnDropUris(brw, fnr);
}

std::string Uri2LocalPath(const std::string& uri_fname, bool& is_new);

#endif // #ifndef __MGUI_PROJECT_DND_H__

