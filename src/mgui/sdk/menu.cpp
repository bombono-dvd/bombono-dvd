
#include <mgui/_pc_.h>

#include "menu.h"

Gtk::MenuItem& AppendMI(Gtk::MenuShell& ms, Gtk::MenuItem& mi)
{
    ms.append(mi);
    return mi;
}

Gtk::Menu& MakeSubmenu(Gtk::MenuItem& mi)
{
    Gtk::Menu& sub_menu = NewManaged<Gtk::Menu>();
    mi.set_submenu(sub_menu);
    return sub_menu;
}

Gtk::MenuItem& MakeAppendMI(Gtk::MenuShell& ms, const char* name)
{
    return AppendMI(ms, NewManaged<Gtk::MenuItem>(name));
}

void AppendSeparator(Gtk::MenuShell& ms)
{
    ms.append(NewManaged<Gtk::SeparatorMenuItem>());
}

void Popup(Gtk::Menu& mn, GdkEventButton* event, bool show_all)
{
    if( show_all )
        mn.show_all();
    mn.popup(event->button, event->time);
}

void AddEnabledItem(Gtk::Menu& menu, const char* name, const ActionFunctor& fnr, bool is_enabled)
{
    Gtk::MenuItem& itm = MakeAppendMI(menu, name);
    itm.set_sensitive(is_enabled);
    itm.signal_activate().connect(fnr);
}

