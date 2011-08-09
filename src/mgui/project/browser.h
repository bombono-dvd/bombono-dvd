//
// mgui/project/browser.h
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

#ifndef __MGUI_PROJECT_BROWSER_H__
#define __MGUI_PROJECT_BROWSER_H__

#include <mbase/project/media.h> // Project::MediaItem
#include <mgui/mguiconst.h>

#include <mgui/sdk/dndtreeview.h>

namespace Project
{

class ObjectStore: public Gtk::TreeStore
{
    typedef Gtk::TreeStore MyParent;
    public:
                               // получить ссылку на путь в объекте
                               // (только для путей верхнего уровня)
    virtual Gtk::TreePath& LocalPath(const Gtk::TreeIter& itr) const;
    virtual     MediaItem  GetMedia(const Gtk::TreeIter& itr) const = 0;

    protected:

    virtual          bool  drag_data_received_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& data);
};


inline MediaItem GetMedia(RefPtr<ObjectStore> ms, const Gtk::TreePath& path)
{
    return ms->GetMedia(ms->get_iter(path));
}
// только для путей верхнего уровня
Gtk::TreePath& LocalPath(Media* mi);

// переиндексируем все, начиная с этого
void ReindexFrom(RefPtr<ObjectStore> os, const Gtk::TreeIter& from_itr);
// переиндексация при перемещении
void SyncOnDragReceived(const Gtk::TreePath& dst_path, const Gtk::TreePath& src_path, RefPtr<ObjectStore> os);
// получить путь того, что перетаскиваем
Gtk::TreePath GetSourcePath(const Gtk::SelectionData& data);

bool ConfirmDeleteMedia(MediaItem mi);
void DeleteMedia(RefPtr<ObjectStore> os, const Gtk::TreeIter& itr);

inline int Size(RefPtr<ObjectStore> os)
{ return (int)os->children().size(); }

//
// ObjectBrowser
// 

class ObjectBrowser : public Gtkmm2ext::DnDTreeView<MediaItem> //Gtk::TreeView
{
    typedef Gtkmm2ext::DnDTreeView<MediaItem> /*Gtk::TreeView*/ MyParent;
    public:

       RefPtr<ObjectStore> GetObjectStore()
                           { return RefPtr<ObjectStore>::cast_static(get_model()); }

                           // удалить из браузера выделенные объекты
    virtual          void  DeleteMedia() = 0;
    virtual          bool  on_key_press_event(GdkEventKey* event);

                           // DnDTreeView
    virtual          bool  on_drag_motion(const RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
    virtual     MediaItem  get_dnd_column_data(const Gtk::TreeIter& itr) { return GetObjectStore()->GetMedia(itr); }
};


// операции с курсором
Gtk::TreePath GetCursor(Gtk::TreeView& brw);
MediaItem GetCurMedia(ObjectBrowser& brw);

// возвращает вставленную позицию
Gtk::TreeIter InsertByPos(RefPtr<ObjectStore> ms, const Gtk::TreePath& pth, bool after = true);
void GoToPos(ObjectBrowser& brw, const Gtk::TreePath& pth);

// :KLUDGE: получить отмеченный элемент
// (только в случае режима одиночного выделения)
// для получения позиции курсора использовать get_cursor()/GetCursor()
inline Gtk::TreeIter GetSelectPos(Gtk::TreeView& tv)
{
    return tv.get_selection()->get_selected();
}

Gtk::HButtonBox& CreateMListButtonBox();
// dnd_column - где MediaItem лежит, который нужно dnd-ить
void SetupBrowser(ObjectBrowser& brw, int dnd_column, bool need_headers = false);

// отрисовка
typedef boost::function<std::string(MediaItem)> RFFunctor;
void SetRendererFnr(Gtk::TreeView::Column& name_cln, Gtk::CellRendererText& rndr, 
                    RefPtr<ObjectStore> os, const RFFunctor& fnr);
void SetupNameRenderer(Gtk::TreeView::Column& name_cln, Gtk::CellRendererText& rndr, 
                     RefPtr<ObjectStore> os);

inline void ValidatePath(Gtk::TreePath& pth)
{
    // :BUG: в gtkmm - нулем быть не должен
    if( !pth.gobj() )
        pth = Gtk::TreePath();
}

typedef boost::function<void(ObjectBrowser&, MediaItem, GdkEventButton*)> RightButtonFunctor;
void SetOnRightButton(ObjectBrowser& brw, const RightButtonFunctor& fnr);

} // namespace Project

#endif // #ifndef __MGUI_PROJECT_BROWSER_H__

