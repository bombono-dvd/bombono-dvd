
#ifndef __MGUI_SDK_WIN32FILESEL_H__
#define __MGUI_SDK_WIN32FILESEL_H__

#include <mgui/timeline/mviewer.h>

#ifdef _WIN32

bool CallWinFileDialog(const char* title, bool open_file, Str::List& chosen_paths, 
                       Gtk::Widget* for_wdg = 0, const FileFilterList& pat_lst = FileFilterList(),
                       bool multiple_choice = false);

#endif

#endif // #ifndef __MGUI_SDK_WIN32FILESEL_H__
