//
// mgui/project/media-browser.cpp
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

#include <mgui/_pc_.h>

#include "mb-actions.h"
#include "handler.h"
#include "dnd.h"

#include <mgui/timeline/mviewer.h>
#include <mgui/sdk/packing.h>
#include <mgui/dialog.h> // MessageBox
#include <mgui/gettext.h>

#include <mlib/sdk/logger.h>
#include <mlib/filesystem.h>

namespace Project
{

MediaItem MediaStore::GetMedia(const Gtk::TreeIter& itr) const
{
    return itr->get_value(columns.media);
}

bool MediaStore::row_drop_possible_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& data) const
{
    Gtk::TreePath tmp_path = GetSourcePath(data);

    RefPtr<MediaStore> this_ = MakeRefPtr(const_cast<MediaStore*>(this));
    // 1 главы вообще никак нельзя передвигать в браузере
    if( IsChapter(Project::GetMedia(this_, tmp_path)) )
        return false;

    // 2
    bool can_drop = false;
    if( dest.get_depth() == 1 )
        can_drop = true;
    else
    {
        tmp_path = dest;
        tmp_path.up();
        if( Project::GetMedia(this_, tmp_path)->IsFolder() )
            can_drop = true;
    }
    return can_drop;
}

bool ValidateMediaInsertionPos(Gtk::TreePath& brw_pth, bool want_ia)
{
    bool insert_after = false;
    if( !brw_pth.empty() )
    {
        while( brw_pth.get_depth() > 1 )
        {
            insert_after = true;
            brw_pth.up();
        }

        if( !insert_after )
            insert_after = want_ia;
    }
    return insert_after;
}

static void OnURIsDrop(MediaBrowser& brw, const StringList& paths, const Point& loc)
{
    Gtk::TreePath brw_pth;
    Gtk::TreeViewDropPosition pos;
    brw.get_dest_row_at_pos(loc.x, loc.y, brw_pth, pos);

    ValidatePath(brw_pth);
    bool insert_after = ValidateMediaInsertionPos(brw_pth, pos != Gtk::TREE_VIEW_DROP_BEFORE);

    TryAddMedias(paths, brw, brw_pth, insert_after);
}

MediaBrowser::MediaBrowser(RefPtr<MediaStore> a_lst)
{
    set_model(a_lst);
    BuildStructure();

    SetupURIDrop(*this, bl::bind(&OnURIsDrop, boost::ref(*this), bl::_1, bl::_2));
}

// Названия типов для i18n
F_("Video")
F_("Chapter")
F_("Still Picture")

void RenderMediaType(Gtk::CellRendererText* rndr, MediaItem mi)
{
    rndr->property_text() = gettext(mi->TypeString().c_str());
}

void MediaBrowser::BuildStructure()
{
    RefPtr<MediaStore> ms = GetMediaStore();
    const MediaStore::TrackFields& trk_fields = ms->columns;

    SetupBrowser(*this, trk_fields.media.index(), true);

    // 1 миниатюра + имя
    {
        Gtk::TreeView::Column& name_cln = *Gtk::manage( new Gtk::TreeView::Column(_("Name")) );
        // ширину колонки можно менять
        name_cln.set_resizable(true);
        
        name_cln.pack_start(trk_fields.thumbnail, false);
        
        // 2 имя
        Gtk::CellRendererText& rndr = *Gtk::manage( new Gtk::CellRendererText() );
        //name_cln.set_renderer(rndr, trk_fields.title);
        SetupNameRenderer(name_cln, rndr, ms);

        append_column(name_cln);
    }

    // 3 тип
    {
        Gtk::TreeView::Column& cln  = *Gtk::manage( new Gtk::TreeView::Column(_("Type")) );
        Gtk::CellRendererText& rndr = *Gtk::manage( new Gtk::CellRendererText() );

        cln.pack_start(rndr, false);
        // не используем данных,- вычисляем на лету
        //cln.set_renderer(rndr, trk_fields.title);
        SetRendererFnr(cln, rndr, ms, &RenderMediaType);

        append_column(cln);
    }
}

void ExecuteForMedia(MediaBrowser& mb, MediaActionFnr fnr)
{
    if( Gtk::TreeIter itr = GetSelectPos(mb) )
    {
        RefPtr<ObjectStore> os = mb.GetObjectStore();
        fnr(os->GetMedia(itr), itr);
    }
}

void DeleteBrowserMedia(MediaItem md, Gtk::TreeIter& itr,
                        RefPtr<MediaStore> ms)
{
    GetBrowserDeletionSign(md) = true;
    DeleteMedia(ms, itr);
}

void ConfirmDeleteBrowserMedia(MediaItem md, Gtk::TreeIter& itr,
                               RefPtr<MediaStore> ms)
{
    if( ConfirmDeleteMedia(md) )
        DeleteBrowserMedia(md, itr, ms);
}

void DeleteMediaFromBrowser(MediaBrowser& mb)
{
    using namespace boost;
    ExecuteForMedia(mb, lambda::bind(&ConfirmDeleteBrowserMedia, lambda::_1, lambda::_2, 
                                     mb.GetMediaStore()));
}

void MediaBrowser::DeleteMedia()
{
    DeleteMediaFromBrowser(*this);
}

void PackMediaBrowser(Gtk::Container& contr, MediaBrowser& brw)
{
    Gtk::VBox& vbox   = *Gtk::manage(new Gtk::VBox);
    contr.add(PackWidgetInFrame(vbox, Gtk::SHADOW_OUT));

//     // не меньше чем размер шрифта элемента в списке
//     Gtk::Label& label = *Gtk::manage(new Gtk::Label("<span font_desc=\"Sans Bold 12\">Media List</span>"));
//     label.set_use_markup(true);
//     vbox.pack_start(label, Gtk::PACK_SHRINK);
//     Gtk::Requisition req = label.size_request();
//     label.set_size_request(0, req.height+10);
    vbox.pack_start(MakeTitleLabel(_("Media List")), Gtk::PACK_SHRINK);
    PackHSeparator(vbox);

//     Gtk::ScrolledWindow* scr_win = Gtk::manage(new Gtk::ScrolledWindow);
//     scr_win->set_shadow_type(Gtk::SHADOW_NONE); //IN);
//     scr_win->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
//     vbox.pack_start(*scr_win);
//     scr_win->add(brw);
    vbox.pack_start(PackInScrolledWindow(brw));
}

void OnMBChangeCursor(MediaBrowser& brw, Gtk::Button* edit_btn)
{
    bool is_on = false;
    if( MediaItem mi = GetCurMedia(brw) )
        is_on = ViewMediaVis::GetViewerFunctor(mi);

    edit_btn->set_sensitive(is_on);
}

static void SetDefaultButtonOnEveryMap(Gtk::Button& btn)
{
    // при смене вкладки, например, теряется фокус по умолчанию
    btn.signal_map().connect(boost::lambda::bind(&SetDefaultButton, boost::ref(btn)));
}

void PackMediaBrowserAll(Gtk::Container& contr, MediaBrowser& brw, ActionFunctor add_media_fnr, 
                         ActionFunctor remove_media_fnr, ActionFunctor edit_media_fnr)
{
    Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 2));
    contr.add(*vbox);
    {
        PackMediaBrowser(*vbox, brw);

        // группа кнопок
        //Gtk::HBox& hbox = *Gtk::manage(new Gtk::HBox(true, 4));
        Gtk::HButtonBox& hbox = CreateMListButtonBox();
        vbox->pack_start(hbox, Gtk::PACK_SHRINK);
        {
            Gtk::Button* add_btn = CreateButtonWithIcon("", Gtk::Stock::ADD, 
                                                        _("Add Media from File Browser"));
            hbox.pack_start(*add_btn);
            //bbox.pack_start(*add_btn);
            add_btn->signal_clicked().connect(add_media_fnr);
            // при смене вкладки теряется фокус по умолчанию
            //SetDefaultButton(*add_btn);
            SetDefaultButtonOnEveryMap(*add_btn);

            Gtk::Button* rm_btn = CreateButtonWithIcon("", Gtk::Stock::REMOVE,
                                                       _("Remove Media"));
            hbox.pack_start(*rm_btn);
            //bbox.pack_start(*rm_btn);
            rm_btn->signal_clicked().connect(remove_media_fnr);
            // Translators: it is normal to translate "Edit" as " " (empty) and
            // to keep the button small; let the tooltip tell the purpose. The same thing 
            // with the button "Edit" in Menu List
            // Замечание: так как переводчики не обращают внимание (фин, например), то ставим пусто,
            // чтобы качество GUI не зависело от локали
            //const char* edit_text = C_("MediaBrowser", "Edit");
            const char* edit_text = "";
            Gtk::Button* edit_btn = CreateButtonWithIcon(edit_text, Gtk::Stock::YES, _("Make Chapters for Video"));
            hbox.pack_start(*edit_btn);
            //bbox.pack_start(*edit_btn);
            edit_btn->signal_clicked().connect(edit_media_fnr);
            // управление состоянием кнопки
            edit_btn->set_sensitive(false);
            brw.signal_cursor_changed().connect( 
                boost::lambda::bind(&OnMBChangeCursor, boost::ref(brw), edit_btn) );
        }
    }
}

Gtk::TreePath& GetBrowserPath(StorageItem si)
{
    return LocalPath(si.get());
}

RefPtr<MediaStore> CreateEmptyMediaStore()
{
    RefPtr<MediaStore> ms(new MediaStore);
    void RegisterMSHandlers(RefPtr<MediaStore> ms);
    RegisterMSHandlers(ms);

    return ms;
}

RefPtr<MediaStore> CreateMediaStore()
{
    RefPtr<MediaStore> ms = CreateEmptyMediaStore();

    PublishMediaStore(ms);
    return ms;
}

} // namespace Project

