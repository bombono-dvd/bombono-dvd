//
// mgui/project/media-browser.h
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

#ifndef __MGUI_PROJECT_MEDIA_BROWSER_H__
#define __MGUI_PROJECT_MEDIA_BROWSER_H__

#include "browser.h"

#include <mgui/sdk/treemodel.h>

namespace Project
{

class MediaStore: public ObjectStore
{
    typedef ObjectStore MyParent;
    public:

            struct TrackFields
            {
                //Gtk::TreeModelColumn<Glib::ustring>        title;
                Gtk::TreeModelColumn<RefPtr<Gdk::Pixbuf> > thumbnail;
                Gtk::TreeModelColumn<MediaItem>   media;
    
                TrackFields(Gtk::TreeModelColumnRecord& rec) 
                { 
                    //add(title); 
                    rec.add(thumbnail);
                    rec.add(media);
                }
            };
            //const TrackFields  columns;

                             MediaStore();

      static    TrackFields& Fields();
      static      MediaItem  Get(const Gtk::TreeRow& row);

      virtual     MediaItem  GetMedia(const Gtk::TreeIter& itr) const;

    protected:

        //virtual bool  row_draggable_vfunc(const TreeModel::Path& path) const;
        virtual bool  row_drop_possible_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& data) const;
};

// обозреватель дорожек
class MediaBrowser : public ObjectBrowser
{
    typedef ObjectBrowser MyParent;
    public:
                       MediaBrowser(RefPtr<MediaStore> a_lst);

    RefPtr<MediaStore> GetMediaStore()
                       { return RefPtr<MediaStore>::cast_static(get_model()); }
    virtual      void  DeleteMedia();

    protected:

            void  BuildStructure();
};


void PackMediaBrowser(Gtk::Container& contr, MediaBrowser& brw);
void PackMediaBrowserAll(Gtk::Container& contr, MediaBrowser& brw, ActionFunctor add_media_fnr, 
                         ActionFunctor remove_media_fnr, ActionFunctor edit_media_fnr);

void DeleteBrowserMedia(MediaItem md, Gtk::TreeIter& itr, RefPtr<MediaStore> ms);
void DeleteMediaFromBrowser(MediaBrowser& mb);

// выполнить действие для текущего медиа в браузере
typedef boost::function<void(MediaItem, Gtk::TreeIter&)> MediaActionFnr;
// :DEPRECATED:
void ExecuteForMedia(MediaBrowser& mb, MediaActionFnr fnr);

//
// Работа с путями
//

Gtk::TreePath& GetBrowserPath(StorageItem si);


RefPtr<MediaStore> CreateEmptyMediaStore();
RefPtr<MediaStore> CreateMediaStore();

} // namespace Project

#endif // #ifndef __MGUI_PROJECT_MEDIA_BROWSER_H__

