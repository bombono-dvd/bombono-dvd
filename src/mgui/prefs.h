//
// mgui/prefs.h
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

#ifndef __MGUI_PREFS_H__
#define __MGUI_PREFS_H__

#include <mlib/patterns.h>
#include <string>

enum PlayAuthoring
{
    paTOTEM = 0,
    paXINE,
    paVLC       // позже добавим, если кому-то потребуется
};

struct Preferences: public Singleton<Preferences>
{
             bool  isPAL;   // PAL vs NTSC
    PlayAuthoring  player;
      std::string  authorPath;
             bool  showSrcFileBrowser; // добавление медиа по-старому

            Preferences() { Init(); }

      void  Init();
};

inline Preferences& Prefs()
{
    return Preferences::Instance();
}

void LoadPrefs();
void ShowPrefs(Gtk::Window* win = 0);

//
// FileChooser не создает директории при set_filename(), если их нет, а 
// устанавливает первую существующую; эта функция:
// - пытается создать директорию
// - если не получилось, то посылает предупреждение в лог
void TrySetDirectory(Gtk::FileChooser& fc, const std::string& dir_path);
inline void TryDefaultAuthorPath(Gtk::FileChooser& fc)
{
    TrySetDirectory(fc, Prefs().authorPath);
}

//
// Восстановление размеров с прошлого запуска
// 

struct UnnamedPreferences: public Singleton<UnnamedPreferences>
{
    bool   isLoaded; // настройки были загружены, а не по умолчанию

    Point  appSz; // размеры и положение приложения
    Point  appPos; 
      int  fbWdh; // ширина File Browser
      int  mdBrw1Wdh; // ширина Media Browser на Sources

        UnnamedPreferences() { Init(); }
  void  Init();
};

UnnamedPreferences& UnnamedPrefs();
void SaveUnnamedPrefs();

void SetUpdatePos(Gtk::HPaned& hpaned, int& saved_pos);

std::string PreferencesPath(const char* fname);

#endif // #ifndef __MGUI_PREFS_H__

