
#ifndef __MGUI_PREFS_H__
#define __MGUI_PREFS_H__

#include <mlib/patterns.h>

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

            Preferences() { Init(); }

      void  Init();
};

inline Preferences& Prefs()
{
    return Preferences::Instance();
}

void LoadPrefs();
void ShowPrefs(Gtk::Window* win = 0);

#endif // #ifndef __MGUI_PREFS_H__

