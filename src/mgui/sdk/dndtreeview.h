/*
    Copyright (C) 2000-2007 Paul Davis 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.

*/

//
// COPY_N_PASTE_REALIZE from Ardour, www.ardour.org
// Thanks for code, Paul :)
// 
// I believe in that DnDTreeView is for data exchange inside app, not outside;
// for exchange between apps one should use another technique. So I've changed
// the code a little more C++-able and remove memory leak.
//  							Ilya Murav'jov
// 

#ifndef __gtkmm2ext_dndtreeview_h__
#define __gtkmm2ext_dndtreeview_h__

#include <string>
#include <gtkmm/treeview.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/selectiondata.h>

namespace Gtkmm2ext {

class DnDTreeViewBase : public Gtk::TreeView 
{
  private:
  public:
	DnDTreeViewBase ();
	~DnDTreeViewBase() {}

	void add_drop_targets (std::list<Gtk::TargetEntry>&);
	void add_object_drag (int column, std::string type_name);
	
	void on_drag_leave(const Glib::RefPtr<Gdk::DragContext>& context, guint time) {
		suggested_action = context->get_suggested_action();
		TreeView::on_drag_leave (context, time);
	}

	bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) {
		suggested_action = context->get_suggested_action();
		return TreeView::on_drag_motion (context, x, y, time);
	}

	bool on_drag_drop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);

  protected:
	std::list<Gtk::TargetEntry> draggable;
	Gdk::DragAction             suggested_action;
	int                         data_column;
};

template<class DataType>
struct SerializedObjectPointers 
{
    std::string           type;
    typedef std::vector<DataType> DataList;
    DataList data;

	void clear() {
		type.clear();
		data.clear();
	}
};

template<class DataType>
class DnDTreeView : public DnDTreeViewBase
{
  public:
	DnDTreeView() {} 

	typedef SerializedObjectPointers<DataType> SOPType;
	typedef typename SOPType::DataList DataList;
	sigc::signal<void, std::string, const DataList&> signal_object_drop;

	void on_drag_data_get(const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& selection_data, guint info, guint time) {
		if (selection_data.get_target() == "GTK_TREE_MODEL_ROW") {
			
			TreeView::on_drag_data_get (context, selection_data, info, time);
			
		} else if (data_column >= 0) {
			dnd_data.clear ();

			Gtk::TreeSelection::ListHandle_Path selection = get_selection()->get_selected_rows ();
			serialize_selection (get_model(), selection, selection_data.get_target());
			//selection_data.set (8, (guchar*)&dnd_data, sizeof(&dnd_data));
            void* p_dat = &dnd_data; // copying pointer of dnd_data
            selection_data.set (8, (guchar*)&p_dat, sizeof(&dnd_data));
		}
	}

	void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& selection_data, guint info, guint time) {
		if (suggested_action) {
			/* this is a drag motion callback. just update the status to
			   say that we are still dragging, and that's it.
			*/
			suggested_action = Gdk::DragAction (0);
			TreeView::on_drag_data_received (context, x, y, selection_data, info, time);
			return;
		}
		
		if (selection_data.get_target() == "GTK_TREE_MODEL_ROW") {
			
			TreeView::on_drag_data_received (context, x, y, selection_data, info, time);

		} else if (data_column >= 0) {
			
			/* object D-n-D */
			
			SOPType* sr = *reinterpret_cast<SOPType* const*>(selection_data.get_data());
			
			if (sr) {
				signal_object_drop (sr->type, sr->data);
			}
			
		} else {
			/* some kind of target type added by the app, which will be handled by a signal handler */
		}
	}

	void on_drag_end(const Glib::RefPtr<Gdk::DragContext>& context) {
		dnd_data.clear();
        TreeView::on_drag_end(context);
	}


  private:

	SOPType dnd_data; /* for freeing serialized data at "drag-end" */

	virtual DataType get_dnd_column_data(const Gtk::TreeIter& itr) {

		DataType dat;
		itr->get_value (data_column, dat);
		return dat;
	}

	void serialize_selection (Glib::RefPtr<Gtk::TreeModel> model, 
				  Gtk::TreeSelection::ListHandle_Path& selection,
				  Glib::ustring type) {

		dnd_data.type = type;
		dnd_data.data.resize(selection.size());

		int cnt = 0;
		for (Gtk::TreeSelection::ListHandle_Path::iterator x = selection.begin(); x != selection.end(); ++x, ++cnt) {

			//model->get_iter (*x)->get_value (data_column, dnd_data.data[cnt]);
			dnd_data.data[cnt] = get_dnd_column_data (model->get_iter (*x));
		}

#if 0
		/* this nasty chunk of code is here because X's DnD protocol (probably other graphics UI's too) 
		   requires that we package up the entire data collection for DnD in a single contiguous region
		   (so that it can be trivially copied between address spaces). We don't know the type of DataType so
		   we have to mix-and-match C and C++ programming techniques here to get the right result.

		   The C trick is to use the "someType foo[0];" declaration trick to create a zero-sized array at the
		   end of a SerializedObjectPointers<DataType object. Then we allocate a raw memory buffer that extends
		   past that array and thus provides space for however many DataType items we actually want to pass
		   around.

		   The C++ trick is to use the placement operator new() syntax to initialize that extra
		   memory properly.
		*/
		
		uint32_t cnt = selection->size();
		uint32_t sz = (sizeof (DataType) * cnt) + sizeof (SerializedObjectPointers<DataType>);

		char* buf = new char[sz];
		SerializedObjectPointers<DataType>* sr = (SerializedObjectPointers<DataType>*) buf;

		for (uint32_t i = 0; i < cnt; ++i) {
			new ((void *) &sr->data[i]) DataType ();
		}
		
		sr->cnt  = cnt;
		sr->size = sz;
		snprintf (sr->type, sizeof (sr->type), "%s", type.c_str());
		
		cnt = 0;
		
		for (Gtk::TreeSelection::ListHandle_Path::iterator x = selection->begin(); x != selection->end(); ++x, ++cnt) {
			model->get_iter (*x)->get_value (data_column, sr->data[cnt]);
		}

		return sr;
#endif /* 0 */
	}
};

} // namespace
 
#endif /* __gtkmm2ext_dndtreeview_h__ */
