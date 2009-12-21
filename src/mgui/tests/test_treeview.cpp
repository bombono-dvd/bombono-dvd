//
// mgui/test_treeview.cpp
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

#include <mgui/tests/_pc_.h>

#include "mgui_test.h"

#include <mgui/img-factory.h>
#include <mgui/sdk/dndtreeview.h>
#include <mgui/sdk/window.h>

#include <mgui/project/browser.h>
#include <mgui/project/dnd.h>

#include <mlib/stream.h>
#include <mlib/sigc.h>
#include <mlib/lambda.h>

#include <vector>

namespace DNDExample
{

struct TestDNDData
{
    std::string idName;

     TestDNDData(): idName("Default Name!") {}
     TestDNDData(const std::string& id_name): idName(id_name) {}
    ~TestDNDData()
     {
        io::cout << "Deleting TestDNDData: " << idName << io::endl;
     }
};

typedef ptr::shared<TestDNDData> DNDData;


//
// DnDWindow
// 

class DnDWindow : public Gtk::Window
{

public:
  DnDWindow();
  virtual ~DnDWindow();

  class ModelColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:

      ModelColumns()
      { add(m_col_id); add(m_col_name); add(m_col_number); add(m_col_percentage); add(m_col_DNDData); }

      Gtk::TreeModelColumn<unsigned int> m_col_id;
      Gtk::TreeModelColumn<Glib::ustring> m_col_name;
      Gtk::TreeModelColumn<short> m_col_number;
      Gtk::TreeModelColumn<int> m_col_percentage;
      Gtk::TreeModelColumn<DNDData> m_col_DNDData;
  };

protected:
  //Signal handlers:
  virtual void on_button_drag_data_get(const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time);
  virtual void on_label_drop_drag_data_received   (const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& selection_data, guint info, guint time);

  //Member widgets:
  Gtk::HBox m_HBox;
  Gtk::Button m_Button_Drag;
  //Gtk::Label m_Label_Drop;
  Gtk::DrawingArea m_DA_Drop;

  Gtkmm2ext::DnDTreeView<DNDData> m_TreeView;
  Glib::RefPtr<Gtk::TreeStore> m_refTreeModel;

  //void Setup(Gtk::TreeView& trk_view);

  ModelColumns m_Columns;


     bool  whileDrop;
  DNDData  dndDat;

  void  OnDNDLeave();
  bool  OnDNDMotion(const Glib::RefPtr<Gdk::DragContext>& cnt, int, int, guint);
  void  ButtonDNDBegin(const Glib::RefPtr<Gdk::DragContext>& cnt);

  bool  OnDaButton(GdkEventKey* event);
  void  OnDNDEnd();
};

void ButtonDNDDelete()
{
    io::cout << "ButtonDNDDelete() call!" << io::endl;
}

void DnDWindow::OnDNDLeave()
{
    whileDrop = false;
    io::cout << "OnDNDLeave()" << io::endl;
}

bool DnDWindow::OnDNDMotion(const Glib::RefPtr<Gdk::DragContext>& , int, int, guint)
{
    if( !whileDrop )
    {
        io::cout << "First motion!" << io::endl;

        //cnt->set_icon(GetFactoryImage("dvdmark.png"), 0, 0);
    }
    whileDrop = true;
    return true;
}

void DnDWindow::ButtonDNDBegin(const Glib::RefPtr<Gdk::DragContext>& cnt)
{
    cnt->set_icon(GetFactoryImage("dvdmark.png"), 0, 0);
}

bool DnDWindow::OnDaButton(GdkEventKey* event)
{
    io::cout << "Button: " << event->state << " ; key: " << event->keyval << io::endl;
    return true;
}

void DnDWindow::OnDNDEnd()
{
    io::cout << "OnDNDEnd: beg" << io::endl;

    dndDat.reset();

    io::cout << "OnDNDEnd: end" << io::endl;
}

static void FillTestDNDData(Gtk::TreeModel::Row row, DnDWindow::ModelColumns& m_columns)
{
    Glib::ustring u_str = row[m_columns.m_col_name];
    row[m_columns.m_col_DNDData] = new TestDNDData(u_str);
}

static void PrintDrops(Gtk::TreeView& tv, const StringList& paths, const Point& loc)
{
    Gtk::TreePath brw_pth; // = Project::GetCursor(tv);
    Gtk::TreeViewDropPosition pos;
    tv.get_dest_row_at_pos(loc.x, loc.y, brw_pth, pos);
    io::cout << "where: " << brw_pth[0] << io::endl;

    for( StringList::const_iterator itr = paths.begin(), end = paths.end(); 
         itr != end; ++itr )
        io::cout << "filepath = " << *itr << io::endl;
}

DnDWindow::DnDWindow()
: m_HBox(true, 2),
  m_Button_Drag("Drag Here\n")
 //,m_Label_Drop("Drop here\n")
  , whileDrop(false)
{
    set_title("DnD example");

    add(m_HBox);

    //Targets:
    Gtk::TargetEntry mi_ent(DND_MI_NAME, Gtk::TARGET_SAME_APP);

    std::list<Gtk::TargetEntry> listTargets;

    //listTargets.push_back( Gtk::TargetEntry("text/plain") );
    listTargets.push_back(mi_ent);

    //Drag site:

    //Make m_Button_Drag a DnD drag source:
    m_Button_Drag.drag_source_set(listTargets);

    //Connect signals:
    m_Button_Drag.signal_drag_data_get().connect( sigc::mem_fun(*this, &DnDWindow::on_button_drag_data_get));
    using namespace boost;
    m_Button_Drag.signal_drag_begin().connect( lambda::bind(&DnDWindow::ButtonDNDBegin, this, lambda::_1));
    m_Button_Drag.signal_drag_data_delete().connect( lambda::bind(&ButtonDNDDelete));
    m_Button_Drag.signal_drag_end().connect( lambda::bind(&DnDWindow::OnDNDEnd, this) );

    m_HBox.pack_start(m_Button_Drag);

    //Drop site:

    //Make m_Label_Drop a DnD drop destination:
    std::list<Gtk::TargetEntry> listTargets2;
    listTargets2.push_back( mi_ent );
    listTargets2.push_back( Gtk::TargetEntry("text/plain") );
    listTargets2.push_back( Gtk::TargetEntry(Project::MediaItemDnDTVType(), Gtk::TARGET_SAME_APP) );
    m_DA_Drop.drag_dest_set(listTargets2);

    //Connect signals:
    m_DA_Drop.signal_drag_data_received().connect( sigc::mem_fun(*this, &DnDWindow::on_label_drop_drag_data_received) );
    m_DA_Drop.signal_drag_leave().connect( lambda::bind(&DnDWindow::OnDNDLeave, this) );
    m_DA_Drop.signal_drag_motion().connect( sigc::mem_fun(*this, &DnDWindow::OnDNDMotion) );

    m_HBox.pack_start(m_DA_Drop);

    //m_DA_Drop.property_can_focus() = true;
    m_Button_Drag.signal_key_press_event().connect( sigc::mem_fun(*this, &DnDWindow::OnDaButton) );


    //
    // TreeView
    //

    m_refTreeModel = Gtk::TreeStore::create(m_Columns);
    m_TreeView.set_model(m_refTreeModel);

    //Fill the TreeView's model
    Gtk::TreeModel::Row row = *(m_refTreeModel->append());
    row[m_Columns.m_col_id] = 1;
    row[m_Columns.m_col_name] = "Billy Bob";
    FillTestDNDData(row, m_Columns);

    Gtk::TreeModel::Row childrow = *(m_refTreeModel->append(row.children()));
    childrow[m_Columns.m_col_id] = 11;
    childrow[m_Columns.m_col_name] = "Billy Bob Junior";
    FillTestDNDData(childrow, m_Columns);

    childrow = *(m_refTreeModel->append(row.children()));
    childrow[m_Columns.m_col_id] = 12;
    childrow[m_Columns.m_col_name] = "Sue Bob";
    FillTestDNDData(childrow, m_Columns);

    row = *(m_refTreeModel->append());
    row[m_Columns.m_col_id] = 2;
    row[m_Columns.m_col_name] = "Joey Jojo";
    FillTestDNDData(row, m_Columns);

    row = *(m_refTreeModel->append());
    row[m_Columns.m_col_id] = 3;
    row[m_Columns.m_col_name] = "Rob McRoberts";
    FillTestDNDData(row, m_Columns);

    childrow = *(m_refTreeModel->append(row.children()));
    childrow[m_Columns.m_col_id] = 31;
    childrow[m_Columns.m_col_name] = "Xavier McRoberts";
    FillTestDNDData(childrow, m_Columns);

    //Add the TreeView's view columns:
    m_TreeView.append_column("ID", m_Columns.m_col_id);
    m_TreeView.append_column("Name", m_Columns.m_col_name);
    m_TreeView.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

    // DND
    m_TreeView.add_object_drag(m_Columns.m_col_DNDData.index(), Project::MediaItemDnDTVType());

    // drop
    SetupURIDrop(m_TreeView, bl::bind(&PrintDrops, boost::ref(m_TreeView), bl::_1, bl::_2));

    m_HBox.pack_start(m_TreeView);

    show_all();
}

DnDWindow::~DnDWindow()
{
}

static void PrintSelectionData(const Gtk::SelectionData& selection_data, const std::string& location_str)
{
    io::cout << "\nFrom " << location_str << ":" << io::endl;
    io::cout << "Target: " << selection_data.get_target() << io::endl;
    io::cout << "Selection: " << gdk_atom_name(selection_data.get_selection()) << io::endl;
}

static void SetData(Gtk::SelectionData& selection_data, void* dat, int dat_sz)
{
    selection_data.set(selection_data.get_target(), 8 /* 8 bits format */, 
                       (const guchar*)dat, dat_sz /* the length of data in bytes */);
}

void DnDWindow::on_button_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&, Gtk::SelectionData& selection_data, guint, guint)
{
    PrintSelectionData(selection_data, "drag-data-get");

    dndDat = new TestDNDData("I'm Data: TestDNDData");
    SetData(selection_data, (void*)dndDat.get(), sizeof(TestDNDData*));
}

void DnDWindow::on_label_drop_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int, int, const Gtk::SelectionData& selection_data, guint, guint time)
{
    PrintSelectionData(selection_data, "drag-data-received");
    CheckSelFormat(selection_data);

    if( selection_data.get_target() == DND_MI_NAME )
    {
        ASSERT( selection_data.get_length() == sizeof(TestDNDData*) );

        TestDNDData* dat = (TestDNDData*)selection_data.get_data();
        io::cout << "Received TestDNDData: " << dat->idName << io::endl;
    }
    else if(  selection_data.get_target() == Project::MediaItemDnDTVType() )
    {
        typedef Gtkmm2ext::SerializedObjectPointers<DNDData> SOPType;
        SOPType& dat = GetSOP<DNDData>(selection_data);
        io::cout << "Received TestDNDData from DndTreeView: " << io::endl;
        for( SOPType::DataList::iterator itr = dat.data.begin(), end = dat.data.end(); itr != end; ++itr )
            io::cout << "\t" << (*itr)->idName << io::endl;
    }
    else
        ASSERT( 0 );

    whileDrop = false;
    context->drag_finish(true, false, time);
}

} // namespace DNDExample


BOOST_AUTO_TEST_CASE( TestDND )
{
    return;
    InitGtkmm();
    DNDExample::DnDWindow win;
    RunWindow(win);
}

namespace NotebookExample
{

class ExampleWindow : public Gtk::Window
{
public:
  ExampleWindow();
  virtual ~ExampleWindow();

protected:
  //Signal handlers:
  virtual void on_button_quit();

  //Child widgets:
  Gtk::VBox m_VBox;
  Gtk::Notebook m_Notebook;
  Gtk::Label m_Label1, m_Label2;

  Gtk::HButtonBox m_ButtonBox;
  Gtk::Button m_Button_Quit;
};

// static void OnAllocate(Gtk::Label& lbl)
// {
//     io::cout << "allocate " << lbl.get_height() << io::endl;
// }
//
// static void OnShow(Gtk::Label& lbl)
// {
//     io::cout << "show " << lbl.get_height() << io::endl;
// }
//
// static void OnHide(Gtk::Label& lbl)
// {
//     io::cout << "hide " << lbl.get_height() << io::endl;
// }
//
// static void OnMap(Gtk::Label& lbl)
// {
//     io::cout << "map " << lbl.get_height() << io::endl;
// }

ExampleWindow::ExampleWindow()
: m_Label1("Contents of tab 1"),
  m_Label2("Contents of tab 2"),
  m_Button_Quit("Quit")
{
//     using namespace boost;
//     m_Label1.signal_size_allocate().connect(lambda::bind(&OnAllocate, boost::ref(m_Label1)));
//     m_Label1.signal_show().connect(lambda::bind(&OnShow, boost::ref(m_Label1)));
//     m_Label1.signal_hide().connect(lambda::bind(&OnHide, boost::ref(m_Label1)));
//     m_Label1.signal_map().connect(lambda::bind(&OnMap, boost::ref(m_Label1)));

    m_Label1.show_all();

    set_title("Gtk::Notebook example");
    set_border_width(10);
    set_default_size(400, 200);
    
    
    add(m_VBox);
    
    //Add the Notebook, with the button underneath:
    m_Notebook.set_border_width(10);

    m_Notebook.set_name("WinTabs");
    m_Notebook.set_scrollable(true);
    m_Notebook.property_homogeneous()  = true;

    //m_Notebook.property_enable_popup() = true;

    //m_Notebook.property_show_border()  = true;
    //m_Notebook.property_show_tabs()    = false;
    //m_Notebook.property_tab_hborder()  = 15;
    //m_Notebook.property_tab_pos()      = Gtk::POS_BOTTOM;
    //m_Notebook.property_tab_vborder()  = 15;

    m_VBox.pack_start(m_Notebook);
    m_VBox.pack_start(m_ButtonBox, Gtk::PACK_SHRINK);
    
    m_ButtonBox.pack_start(m_Button_Quit, Gtk::PACK_SHRINK);
    m_Button_Quit.signal_clicked().connect( sigc::mem_fun(*this, &ExampleWindow::on_button_quit) );
    
    //Add the Notebook pages:
    m_Notebook.append_page(m_Label1, "First");
    m_Notebook.append_page(m_Label2, "Second page", "Menu label");

    show_all_children();
}

ExampleWindow::~ExampleWindow()
{
}

void ExampleWindow::on_button_quit()
{
  hide();
}

} // namespace NotebookExample

BOOST_AUTO_TEST_CASE( TestNotebook )
{
    return;
    InitGtkmm();
    NotebookExample::ExampleWindow win;
    RunWindow(win);
}


