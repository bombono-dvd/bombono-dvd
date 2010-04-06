
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


#endif // #ifndef __MGUI_PREFS_H__

