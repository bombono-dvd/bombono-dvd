//
// mgui/timeline/mviewer.h
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

#ifndef __MGUI_TIMELINE_MVIEWER_H__
#define __MGUI_TIMELINE_MVIEWER_H__

#include <mgui/dialog.h> // FCDFunctor

#include <mlib/function.h>
#include <mlib/string.h>
#include <mlib/tech.h>

typedef boost::function<void(const char*, Str::List&)> OpenFileFnr;
// add_open_button = true - для просмотрщика mviewer
// false - для самого приложения
ActionFunctor PackFileChooserWidget(Gtk::Container& contr, OpenFileFnr fnr, bool is_mviewer);
Gtk::Label& MakeTitleLabel(const char* name);

void OpenFile(Gtk::FileChooser& fc, OpenFileFnr fnr);

void RunMViewer();

//
// Фильтры файлов по расширению
//
struct FileFilter
{
    std::string  title;
      Str::List  extPatLst; // строки в формате "*.mpg"

      FileFilter(const std::string& title_): title(title_) {}
};

typedef std::list<FileFilter> FileFilterList;

FileFilter& AddFileFilter(FileFilterList& lst, const std::string& title);
FileFilter& AddFPs(FileFilter& ff, const char** lst);
void AddAllFF(FileFilterList& lst);

void FillFPLforMedias(FileFilterList& lst);

namespace Project {
const char* AddFilesDialogTitle();
} // namespace Project

// chosen_paths: на вход - выбранный файл, если нужно (но только один принимается), на выход - 
// результат(ы) выбора
bool RunFileDialog(const char* title, bool open_file, Str::List& chosen_paths, 
                   Gtk::Widget& for_wdg, const FileFilterList& pat_lst = FileFilterList(),
                   bool multiple_choice = false, const FCDFunctor& fnr = FCDFunctor());

Str::List GetFilenames(Gtk::FileChooser& fc);
// в отличие от add_pattern() сравнение идет регистронезависимо (для "*.vob" подойдет и "1.VOB")
void AddGlobFilter(Gtk::FileFilter& ff, const std::string& pat);

#endif // #ifndef __MGUI_TIMELINE_MVIEWER_H__

