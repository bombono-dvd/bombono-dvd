//
// mgui/tests/test_dnd.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#include <mgui/tests/_pc_.h>

#include "mgui_test.h"

#include <mgui/project/dnd.h>

#include <gtk/gtkdnd.h>

namespace DND
{

void PrintSelectionData(const Gtk::SelectionData& selection_data, const std::string& location_str)
{
    io::cout << "\nFrom " << location_str << ":" << io::endl;
    io::cout << "Target: " << selection_data.get_target() << io::endl;
    io::cout << "Selection: " << gdk_atom_name(selection_data.get_selection()) << io::endl;
}

void SetData(Gtk::SelectionData& selection_data, void* dat, int dat_sz)
{
    selection_data.set(selection_data.get_target(), 8 /* 8 bits format */, 
                       (const guchar*)dat, dat_sz /* the length of data in bytes */);
}


//
// DnD-ошибка: вызов gtk_drag_get_data() в "drag-motion" ведет к крушению
// Код, практически без изменений, из примеров gtkmm.
// 

// class DnDWindow : public Gtk::Window
// {
//
// public:
//             DnDWindow();
//   virtual  ~DnDWindow();
//
// protected:
//   virtual void  on_button_drag_data_get(const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time);
//   virtual void  on_label_drop_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& selection_data, guint info, guint time);
//
//     Gtk::HBox  m_HBox;
//   Gtk::Button  m_Button_Drag;
//    Gtk::Label  m_Label_Drop;
//
//          bool  whileDrop;
//
// bool  OnDNDMotion(const Glib::RefPtr<Gdk::DragContext>& context, int, int, guint time);
// };
//
// std::string MyObjStr = "MyObject";
//
// DnDWindow::DnDWindow()
// : m_Button_Drag("Drag Here\n"),
//   m_Label_Drop("Drop here\n"),
//   whileDrop(false)
// {
//     set_title("DnD example");
//
//     add(m_HBox);
//
//     std::list<Gtk::TargetEntry> listTargets;
//     listTargets.push_back( Gtk::TargetEntry(MyObjStr) );
//
//     //Drag site:
//     m_Button_Drag.drag_source_set(listTargets);
//     m_Button_Drag.signal_drag_data_get().connect( sigc::mem_fun(*this, &DnDWindow::on_button_drag_data_get));
//
//     m_HBox.pack_start(m_Button_Drag);
//
//     //Drop site:
//     m_Label_Drop.drag_dest_set(listTargets);
//     m_Label_Drop.signal_drag_data_received().connect( sigc::mem_fun(*this, &DnDWindow::on_label_drop_drag_data_received) );
//     //m_Label_Drop.signal_drag_leave().connect( lambda::bind(&DnDWindow::OnDNDLeave, this) );
//     m_Label_Drop.signal_drag_motion().connect( sigc::mem_fun(*this, &DnDWindow::OnDNDMotion) );
//
//     m_HBox.pack_start(m_Label_Drop);
//
//     show_all();
// }
//
// DnDWindow::~DnDWindow()
// {
// }
//
// bool DnDWindow::OnDNDMotion(const Glib::RefPtr<Gdk::DragContext>& context, int, int, guint time)
// {
//     io::cout << "motion!" << io::endl;
//     whileDrop = true;
//
// //     Glib::ustring target_str = m_Label_Drop.drag_dest_find_target(context); //MyObjStr;
// //     ASSERT(target_str == MyObjStr);
// //     m_Label_Drop.drag_get_data(context, target_str, time);
//     GtkWidget* wdg = (GtkWidget*)m_Label_Drop.gobj();
//     GdkAtom targ = gtk_drag_dest_find_target(wdg, context->gobj(), NULL);
//     gtk_drag_get_data(wdg, context->gobj(), targ, time);
//
//     io::cout << "motion2!" << io::endl;
//     return true;
// }
//
// void DnDWindow::on_button_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&, Gtk::SelectionData& selection_data, guint, guint)
// {
//   selection_data.set(selection_data.get_target(), 8 /* 8 bits format */, (const guchar*)"I'm Data!", 9 /* the length of I'm Data! in bytes */);
// }
//
// void DnDWindow::on_label_drop_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int, int, const Gtk::SelectionData& selection_data, guint, guint time)
// {
//     PrintSelectionData(selection_data, "drag-data-received");
//     CheckSelFormat(selection_data);
//
//     if ((selection_data.get_length() >= 0) && (selection_data.get_format() == 8))
//     {
//         io::cout << "Received \"" << selection_data.get_data_as_string() << "\" in label " << io::endl;
//     }
//
//     if( whileDrop )
//     {
//         io::cout << "was drop!" << io::endl;
//         gdk_drag_status(context->gobj(), (GdkDragAction)0, time);
//     }
//     else
//     {
//         io::cout << "drop!" << io::endl;
//         context->drag_finish(true, false, time);
//     }
//     whileDrop = false;
// }
//
} // namespace DND

//
// BOOST_AUTO_TEST_CASE( TestDND )
// {
//     //return;
//     InitGtkmm();
//     DND::DnDWindow win;
//     RunWindow(win);
// }

BOOST_AUTO_TEST_CASE( TestGIO )
{
    return;
    InitGtkmm();
    
    //const char* dst_fname = "/home/ilya/opt/programming/atom-project/fbh_copy.png";
    //
    //RefPtr<Gio::File> src = Gio::File::create_for_uri("http://ftp.gnome.org/pub/GNOME/teams/art.gnome.org/backgrounds/ABSTRACT-Aurora_1280x1024.jpg");
    //RefPtr<Gio::FileInfo> f_inf = src->query_info("*");
    //if( f_inf )
    //{
    //    //io::cout << f_inf->get_file_type() << ", " << f_inf->get_content_type() << "}}}" << io::endl;
    //    ;
    //    for( char** it = g_file_info_list_attributes(f_inf->gobj(), 0); *it; it++ )
    //    //boost_foreach( Glib::ustring attr, f_inf->list_attributes("") )
    //        io::cout << *it << " = " << f_inf->get_attribute_as_string(*it) << io::endl;
    //}
    //else
    //    io::cout << "no info!" << io::endl;
    //
    //return;
    //
    //
    //src->copy(Gio::File::create_for_path(dst_fname));
    //
    //return;
    
    const char* uri_fname = "http://www.ubuntu.com/start-download?distro=desktop&bits=32&release=latest"; // "http://ftp.gnome.org/pub/GNOME/teams/art.gnome.org/backgrounds/ABSTRACT-Aurora_1280x1024.jpg";
    //const char* uri_fname = "file:///home/ilya/opt/programming/atom-project/fbh.png";
    
    io::cout << "Destination path: " << Uri2LocalPath(uri_fname) << io::endl;
}

