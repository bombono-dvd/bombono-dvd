//
// mgui/project/mb-actions.h
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

#ifndef __MGUI_PROJECT_MB_ACTIONS_H__
#define __MGUI_PROJECT_MB_ACTIONS_H__

#include "media-browser.h"

#include <mgui/timeline/layout.h>

namespace Project
{

class ViewMediaVis: public ObjVisitor
{
    public:
    typedef boost::function<bool(TrackLayout&)> Fnr;
            Fnr  vFnr;

     static  Fnr  GetViewerFunctor(MediaItem mi)
                  {
                      ViewMediaVis vis;
                      mi->Accept(vis);
                      return vis.vFnr;
                  }

    protected:

            void  Visit(VideoMD& obj);
            void  Visit(VideoChapterMD& obj);
};

void ViewMedia(TrackLayout& layout, MediaItem mi);

void FillThumbnail(const Gtk::TreeIter& itr, RefPtr<MediaStore> ms, Media& md);

// определить тип файла и создать по нему соответствующее медиа
StorageItem CreateMedia(const char* fname, std::string& err_string);

// заполнить медиа в браузере
void PublishMedia(const Gtk::TreeIter& itr, RefPtr<MediaStore> ms, MediaItem mi);
void PublishMediaStore(RefPtr<MediaStore> ms);

void PackMBWindow(Gtk::HPaned& fcw_hpaned, Timeline::DAMonitor& mon, TrackLayout& layout, 
                  MediaBrowser& brw);

// pth - куда вставлять; по выходу pth равен позиции вставленного
// insert_after - вставить после pth, по возможности
bool TryAddMedia(const char* fname, Gtk::TreePath& pth, std::string& err_str, 
                 bool insert_after = true);
// интерактивный вариант TryAddMedia()
void TryAddMedias(const Str::List& paths, MediaBrowser& brw,
                  Gtk::TreePath& brw_pth, bool insert_after);

// ограничиваем возможность вставки верхним уровнем
// want_ia - где хотим вставить (dnd)
// возвращает - куда надо вставить (до или после)
bool ValidateMediaInsertionPos(Gtk::TreePath& brw_pth, bool want_ia = true);

} // namespace Project

namespace DVD {

void RunImport(Gtk::Window& par_win, const std::string& dvd_path = std::string());

} // namespace DVD

#endif // #ifndef __MGUI_PROJECT_MB_ACTIONS_H__

