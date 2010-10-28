
#include <mgui/_pc_.h>

#include "window.h"

void RunWindow(Gtk::Window& win)
{
    win.show_all();
    Gtk::Main::run(win);
}

Gtk::Window* GetTopWindow(Gtk::Widget& wdg)
{
    return dynamic_cast<Gtk::Window*>(wdg.get_toplevel());
}

Point CalcBeautifulRect(int wdh)
{
    return Point(wdh, wdh*3/5);
}

void SetAppWindowSize(Gtk::Window& win, int wdh)
{
    Point sz = CalcBeautifulRect(wdh);
    win.set_default_size(sz.x, sz.y);
}

