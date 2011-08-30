//
// mgui/project/add.h
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

#ifndef __MGUI_PROJECT_ADD_H__
#define __MGUI_PROJECT_ADD_H__

#include "media-browser.h"

#include <mlib/filesystem.h>
#include <mlib/string.h>

namespace Project
{
StorageItem TryAddMedia(const char* fname, Gtk::TreePath& pth, std::string& err_str, 
                 bool insert_after = true);
StorageItem CheckExists(const fs::path& pth, RefPtr<MediaStore> ms);
    
// интерактивный вариант TryAddMedia()
void TryAddMedias(const Str::List& paths, MediaBrowser& brw,
                  Gtk::TreePath& brw_pth, bool insert_after);
// desc - метка происхождения, добавления
void TryAddMediaQuiet(const std::string& fname, const std::string& desc);

void OneMediaError(const fs::path& err_pth, const std::string& desc);

// заполнить медиа в браузере
void PublishMedia(const Gtk::TreeIter& itr, RefPtr<MediaStore> ms, MediaItem mi);
void PublishMediaStore(RefPtr<MediaStore> ms);
void MediaBrowserAdd(MediaBrowser& brw, Str::List& paths);

void MuxAddStreams(const std::string& src_fname);

// ограничиваем возможность вставки верхним уровнем
// want_ia - где хотим вставить (dnd)
// возвращает - куда надо вставить (до или после)
bool ValidateMediaInsertionPos(Gtk::TreePath& brw_pth, bool want_ia = true);

// дополнительная инфо к возвращаемому значению IsVideoDVDCompliant()
struct Mpeg2Info
{
           bool  isMpeg2;
           bool  videoCheck; // видеопоток DVD-совместим
    std::string  errStr;
    
    Mpeg2Info(): isMpeg2(false), videoCheck(false) {}
};
bool IsVideoDVDCompliant(const char* fname, Mpeg2Info& inf);

bool GetPicDimensions(const char* fname, Point& sz);
} // namespace Project

namespace DVD {

void RunImport(Gtk::Window& par_win, const std::string& dvd_path = std::string());

} // namespace DVD

#endif // #ifndef __MGUI_PROJECT_ADD_H__

