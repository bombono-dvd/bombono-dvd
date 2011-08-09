//
// mgui/project/browser.cpp
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

#include <mgui/_pc_.h>

#include "browser.h"
#include "handler.h"
#include "dnd.h"
#include "mconstructor.h" // GetAStores()

#include <mbase/project/menu.h>
#include <mbase/project/table.h>  // AData()
#include <mgui/sdk/packing.h>
#include <mgui/dialog.h>
#include <mgui/key.h>
#include <mgui/gettext.h>

#include <mlib/sdk/logger.h>

namespace Project
{

Gtk::TreePath& ObjectStore::LocalPath(const Gtk::TreeIter& itr) const
{
    return Project::LocalPath(GetMedia(itr).get());
}

bool ObjectStore::drag_data_received_vfunc(
    const TreeModel::Path& dst_path, const Gtk::SelectionData& data)
{
    bool do_drop = MyParent::drag_data_received_vfunc(dst_path, data);
    if( do_drop )
    {
        //
        // Учитываем, что медиа:
        // - вставлено на новое место dst_path, но еще не удалено
        //   со старого места;
        // - если переносили вниз, то старое место - src_path,
        //   иначе - src_path+1
        //
        ASSERT( dst_path.size() == 1 );
        Gtk::TreePath src_path = GetSourcePath(data);
        Gtk::TreePath old_path = LocalPath(get_iter(dst_path));
        if( old_path != src_path )
        {
            LOG_ERR << "Error on dragging media, old != src: " << old_path.to_string() 
                    << " != " << src_path.to_string() << io::endl;
            ASSERT(0);
        }
        SyncOnDragReceived(dst_path, src_path, MakeRefPtr(this));
    }
    return do_drop;
}

// только для путей верхнего уровня
Gtk::TreePath& LocalPath(Media* mi)
{
    return mi->GetData<Gtk::TreePath>();
}

// переиндексируем все, начиная с этого
void ReindexFrom(RefPtr<ObjectStore> os, const Gtk::TreeIter& from_itr)
{
    if( !from_itr )
        return;

    Gtk::TreeIter itr(from_itr);
    Gtk::TreePath path = os->get_path(itr);
    for( ; itr; ++itr, path.next() )
        os->LocalPath(itr) = path;
}

void SyncOnDragReceived(const Gtk::TreePath& dst_path, const Gtk::TreePath& src_path, RefPtr<ObjectStore> os)
{
    // направление перемещения
    int src_num = src_path[0];
    int dst_num = dst_path[0];

    if( (src_num != dst_num) && (src_num+1 != dst_num) )
    {
        if( src_num < dst_num )
        {
            // вниз
            Gtk::TreeIter itr = os->get_iter(src_path);
            for( Gtk::TreePath path = src_path, prev_path = src_path; 
                 path.next(), ++itr, path <= dst_path ; prev_path = path )
                os->LocalPath(itr) = prev_path;
        }
        else
        {
            // вверх
            Gtk::TreeIter itr = os->get_iter(dst_path);
            for( Gtk::TreePath path = dst_path; path <= src_path ; path.next(), ++itr )
                os->LocalPath(itr) = path;
        }
        InvokeOn(GetMedia(os, dst_path), "Reorder");
    }
}

Gtk::TreePath GetSourcePath(const Gtk::SelectionData& data)
{
    Gtk::TreePath src_path;
    bool true_ = Gtk::TreePath::get_from_selection_data(data, src_path);
    ASSERT_OR_UNUSED( true_ );

    return src_path;
}

const char* ConfirmQuestions[] = {
    N_("Do you really want to delete \"%1%\" from Media List?"),
    N_("Do you really want to delete chapter \"%1%\"?"),
    N_("Do you really want to delete menu \"%1%\"?")
};

class DelConfirmationStrVis: public ObjVisitor
{
    public:
        std::string templStr;

        virtual  void  Visit(StillImageMD&)   { templStr = ConfirmQuestions[0]; }
        virtual  void  Visit(VideoMD&)        { templStr = ConfirmQuestions[0]; }
        virtual  void  Visit(VideoChapterMD&) { templStr = ConfirmQuestions[1]; }
        virtual  void  Visit(MenuMD&)         { templStr = ConfirmQuestions[2]; }
};

static std::string GetDelConfirmationStr(MediaItem md)
{
    DelConfirmationStrVis vis;
    md->Accept(vis);
    return BF_(vis.templStr) % md->mdName % bf::stop;
}

bool ConfirmDeleteMedia(MediaItem mi)
{
    return Gtk::RESPONSE_OK == MessageBox(GetDelConfirmationStr(mi), 
                                          Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
}

void DeleteMedia(RefPtr<ObjectStore> os, const Gtk::TreeIter& itr)
{
    LOG_INF << "Delete Media!" << io::endl;
    MediaItem mi = os->GetMedia(itr);

    Gtk::TreeIter next_itr = os->erase(itr);
    if( !IsChapter(mi) )
        ReindexFrom(os, next_itr);

    DeleteMedia(mi);
}

bool IsControlKey(int state)
{
    return (state & SH_CTRL_ALT_MASK) == GDK_CONTROL_MASK;
}

MediaItem GetCurMedia(ObjectBrowser& brw)
{
    MediaItem mi;
    Gtk::TreePath path = GetCursor(brw);
    if( !path.empty() )
        mi = GetMedia(brw.GetObjectStore(), path);
    return mi;
}

class RedrawThumbnailVis: public ObjVisitor
{
    public:
    virtual   void  Visit(MenuMD& obj);
    virtual   void  Visit(VideoMD& obj);

    virtual   void  Visit(StillImageMD&)   { ASSERT(0); } // не нужно
    virtual   void  Visit(VideoChapterMD&) { ASSERT(0); }
};

void RedrawThumbnailVis::Visit(VideoMD& obj)
{
    void FillThumbnail(RefPtr<MediaStore> ms, StorageItem si);
    FillThumbnail(GetAStores().mdStore, &obj);
}

void RedrawThumbnailVis::Visit(MenuMD& obj)
{
    void FillThumbnail(const Gtk::TreeIter& itr, RefPtr<MenuStore> ms, bool force_thumb = false);
    RefPtr<MenuStore> ms = GetAStores().mnStore;
    FillThumbnail(ms->get_iter(LocalPath(&obj)), ms, true);
}

void RedrawThumbnail(MediaItem mi)
{
    if( mi )
    {
        RedrawThumbnailVis vis;
        mi->Accept(vis);
    }
}

bool ObjectBrowser::on_key_press_event(GdkEventKey* event)
{
    // :TODO: "key_binding" - см. meditor_text.cpp
    switch( event->keyval )
    {
    case GDK_Delete:  case GDK_KP_Delete:
        if( CanShiftOnly(event->state) )
            DeleteMedia();
        break;
    case GDK_E: case GDK_e:
        if( IsControlKey(event->state) ) // :DOC: Ctrl+E = установка начального (Entrance) медиа
            if( MediaItem mi = GetCurMedia(*this) )
            {
                if( IsVideo(mi) || IsMenu(mi) )
                {
                    MediaItem& fp = AData().FirstPlayItem();
                    MediaItem old_fp = fp;
                    if( fp != mi )
                    {
                        fp = mi;
                        RedrawThumbnail(fp);
                    }
                    else
                        fp = MediaItem();
                    RedrawThumbnail(old_fp);
                }
                else
                    MessageBox(_("First-Play media can be Video or Menu only."), 
                               Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
            }
        break;
    default:
        break;
    }

    return MyParent::on_key_press_event(event);
}

bool ObjectBrowser::on_drag_motion(const RefPtr<Gdk::DragContext>& context, int x, int y, guint time)
{
    if( drag_dest_find_target(context) == MediaItemDnDTVType())
    {
        // не такой тип только для внешнего использования
        context->drag_refuse(time);
        return false;
    }
    return MyParent::on_drag_motion(context, x, y, time);
}

Gtk::TreePath GetCursor(Gtk::TreeView& brw)
{
    Gtk::TreePath pth;
    Gtk::TreeViewColumn* col = 0;
    brw.get_cursor(pth, col);
    ValidatePath(pth);

    return pth;
}

Gtk::TreeIter InsertByPos(RefPtr<ObjectStore> os, const Gtk::TreePath& pth,
                          bool after)
{
    Gtk::TreeIter itr;
    if( !pth.empty() )
    {
        itr = os->get_iter(pth);
        if( after )
            ++itr;
    }
    if( !itr )
        itr = os->children().end();
    return os->insert(itr);
}

void GoToPos(ObjectBrowser& brw, const Gtk::TreePath& pth)
{
    brw.set_cursor(pth);
    brw.grab_focus();
}

Gtk::HButtonBox& CreateMListButtonBox()
{
    Gtk::HButtonBox& hb = *Gtk::manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_START, 2));
    //hb.set_child_min_width(70);
    hb.set_name("MListButtonBox");

    return hb;
}

std::string MediaItemDnDTVType() { return "DnDTreeView<"DND_MI_NAME">"; }

void SetupBrowser(ObjectBrowser& brw, int dnd_column, bool need_headers)
{
    // если хотим "полосатый" браузер
    //brw.set_rules_hint();
    brw.set_headers_visible(need_headers);
    brw.get_selection()->set_mode(Gtk::SELECTION_BROWSE);

    // * DND
    //brw.set_reorderable(true); // не нужно, для DndTreeView автоматом включено
    if( dnd_column != -1 )
        brw.add_object_drag(dnd_column, MediaItemDnDTVType());
}

static void OnTitleEdited(RefPtr<ObjectStore> os, const Glib::ustring& path_str, 
                          const Glib::ustring& new_title)
{
    MediaItem mi = GetMedia(os, Gtk::TreePath(path_str));
    DoNameChange(mi, new_title);
}

static std::string RenderMediaName(MediaItem mi)
{
    return mi->mdName;
}

void SetupNameRenderer(Gtk::TreeView::Column& name_cln, Gtk::CellRendererText& rndr, 
                     RefPtr<ObjectStore> os)
{
    name_cln.pack_start(rndr);
    rndr.property_editable() = true;
    rndr.signal_edited().connect( bb::bind(&OnTitleEdited, os, _1, _2) );
    SetRendererFnr(name_cln, rndr, os, &RenderMediaName);
}

} // namespace Project

void PackHSeparator(Gtk::VBox& vbox)
{
    vbox.pack_start(NewManaged<Gtk::HSeparator>(), Gtk::PACK_SHRINK);
}

Gtk::ScrolledWindow& PackInScrolledWindow(Gtk::Widget& wdg, bool need_hz)
{
    Gtk::ScrolledWindow& scr_win = *Gtk::manage(new Gtk::ScrolledWindow);
    scr_win.set_shadow_type(Gtk::SHADOW_NONE); //SHADOW_OUT); //IN);
    scr_win.set_policy( need_hz ? Gtk::POLICY_AUTOMATIC : Gtk::POLICY_NEVER, 
                        Gtk::POLICY_AUTOMATIC );

    scr_win.add(wdg);
    return scr_win;
}


//////////////////////////////////////////////////////////////////////////
// DnD для URI

struct UriDropFunctorImpl
{
    UriDropFunctorImpl(const UriDropFunctor& f_): fnr(f_) {}

    void operator()(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, 
                    const Gtk::SelectionData& data, guint , guint time)
    {
        //
        // В функции get_uris() проверяется тип dnd (подходит только "text/uri-list"),
        // поэтому доп. проверки нет.
        // Замечание: судя по коду ardour (см. функцию convert_drop_to_paths()), раньше были
        // "дропы" и с типом "text/uri-list" (от Nautilus'а) - если будут проблемы можно 
        // попробовать convert_drop_to_paths() вместо get_uris()
        //

        typedef Glib::StringArrayHandle URIList;
        URIList uris = data.get_uris();
        if( !uris.empty() )
        {
            StringList paths;
            for( URIList::iterator itr = uris.begin(), end = uris.end(); itr != end ; ++itr )
                paths.push_back(Glib::filename_from_uri(*itr));

            //io::stream out_strm("../ttt", iof::out);
            //for( StringList::iterator itr = paths.begin(), end = paths.end(); itr != end; ++itr )
            //    out_strm << "filepath = " << *itr << io::endl;

            context->drag_finish(true, false, time);

            fnr(paths, Point(x, y));
        }
    }
    protected:
        UriDropFunctor fnr;
};

void ConnectOnDropUris(Gtk::Widget& dnd_wdg, const UriDropFunctor& fnr)
{
    dnd_wdg.signal_drag_data_received().connect(UriDropFunctorImpl(fnr));
}

