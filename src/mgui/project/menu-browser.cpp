//
// mgui/project/menu-browser.cpp
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

#include "media-browser.h"
#include "menu-browser.h"
#include "handler.h"

#include <mgui/editor/toolbar.h>
#include <mgui/timeline/mviewer.h>
#include <mgui/render/menu.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/menu.h>

#include <mlib/sdk/logger.h>

namespace Project
{

Menu GetMenu(RefPtr<MenuStore> ms, const Gtk::TreeIter& itr)
{
    return itr->get_value(ms->columns.menu);
}

MediaItem MenuStore::GetMedia(const Gtk::TreeIter& itr) const
{
    return GetMenu(MakeRefPtr(const_cast<MenuStore*>(this)), itr);
}

bool MenuStore::row_drop_possible_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& /*data*/) const
{
    return dest.get_depth() == 1;
}

MenuBrowser::MenuBrowser(RefPtr<MenuStore> ms)
{
    set_name("MenuBrowser");
    set_model(ms);
    SetupBrowser(*this, ms->columns.menu.index());

    const MenuStore::TrackFields& trk_fields = ms->columns;

    // 1 миниатюра + имя
    {
        Gtk::TreeView::Column& name_cln = NewManaged<Gtk::TreeView::Column>(Glib::ustring("Name"));
        name_cln.pack_start(trk_fields.thumbnail, false);

        // 2 имя
        Gtk::CellRendererText& rndr = NewManaged<Gtk::CellRendererText>();

        rndr.property_size_points() = 13.0; // синхронизировать со стилем "MenuEntryStyle"
        rndr.property_xpad() = 5;
        //rndr.property_weight() = PANGO_WEIGHT_BOLD;
        SetupNameRenderer(name_cln, rndr, ms);

        append_column(name_cln);
    }
}

void DeleteMenuFromBrowser(MenuBrowser& brw)
{
    if( Gtk::TreeIter itr = GetSelectPos(brw) )
    {
        RefPtr<ObjectStore> os = brw.GetObjectStore();
        if( ConfirmDeleteMedia(os->GetMedia(itr)) )
            DeleteMedia(os, itr);
    }
}

void MenuBrowser::DeleteMedia()
{
    DeleteMenuFromBrowser(*this);
}

static void SetMenuTitle(Gtk::Label& title_lbl, const std::string& title)
{
    title_lbl.set_markup("<span font_desc=\"Sans Italic 13\">" + 
                         title + "</span>");
}

void EditMenu(MenuBrowser& brw, MEditorArea& meditor, Gtk::Label& title_lbl)
{
    Gtk::TreePath path = GetCursor(brw);
    if( !path.empty() )
    {
        RefPtr<MenuStore> ms = brw.GetMenuStore();
        Menu menu = GetMenu(ms, ms->get_iter(path));
        if( meditor.CurMenu() != menu )
        {
            meditor.LoadMenu(menu);
            SetMenuTitle(title_lbl, menu->mdName);
        }
    }
}

class EdtHandlerVis: public  ObjVisitor
{
    public:
                    EdtHandlerVis(MEditorArea& m_edt, Gtk::Label& t_lbl)
                        : mEdt(m_edt), titleLbl(t_lbl) {}

     virtual  void  Visit(MenuMD& obj) 
                    {
                        if( &obj == mEdt.CurMenu().get() )
                            ChangeEditor();
                    }
     virtual  void  ChangeEditor() = 0;

    protected:
            MEditorArea& mEdt;
             Gtk::Label& titleLbl;
};

class EdtOnChangeNameVis: public EdtHandlerVis
{
    typedef EdtHandlerVis MyParent;
    public:
                    EdtOnChangeNameVis(MEditorArea& m_edt, Gtk::Label& t_lbl)
                        : MyParent(m_edt, t_lbl) {}

     virtual  void  ChangeEditor() 
                    {
                        SetMenuTitle(titleLbl, mEdt.CurMenu()->mdName);
                    }
};

class EdtOnDeleteVis: public EdtHandlerVis
{
    typedef EdtHandlerVis MyParent;
    public:
                    EdtOnDeleteVis(MEditorArea& m_edt, Gtk::Label& t_lbl)
                        : MyParent(m_edt, t_lbl) {}

     virtual  void  ChangeEditor() 
                    {
                        SetMenuTitle(titleLbl, "");
                        mEdt.LoadMenu(0);
                    }
};

static void OnMenuBrowserRowActivated(ActionFunctor edit_fnr)
{
    edit_fnr();
}

class SetLinkMenuVis: public ObjVisitor
{
    public:
                SetLinkMenuVis(RefPtr<MenuStore> mn_store, RefPtr<MediaStore> md_store)
                    :mnStore(mn_store), mdStore(md_store) {}

 virtual  void  Visit(MenuMD& obj);

    protected:
        RefPtr<MenuStore> mnStore;
       RefPtr<MediaStore> mdStore;
};

static void AddNoStaffItem(Gtk::Menu& lnk_list, const std::string& label)
{
    Gtk::MenuItem& mn_itm = NewManaged<Gtk::MenuItem>(Glib::ustring(label));
    mn_itm.set_sensitive(false);
    lnk_list.append(mn_itm);
}

static std::string MakeMediaItemNameForMenu(MediaItem mi)
{
    return mi ? mi->mdName + " (" + mi->TypeString() + ")" : "No Link" ;
}

static Gtk::RadioMenuItem& 
AddMediaItemChoice(Gtk::Menu& lnk_list, MediaItem mi, Gtk::RadioButtonGroup& grp,
                   const SetLinkMenu& slm, MEditorArea* editor, const std::string& name = std::string())
{
    std::string itm_name = name.empty() ? MakeMediaItemNameForMenu(mi) : name ;

    Gtk::RadioMenuItem& itm = NewManaged<Gtk::RadioMenuItem>(grp, itm_name);
    bool is_default = (mi == slm.newLink);
    itm.set_active(is_default);
    // строго после установки сигнала set_active()
    itm.signal_activate().connect(boost::lambda::bind(SetSelObjectsLinks, boost::ref(*editor), mi, slm.isForBack));

    lnk_list.append(itm);
    return itm;
}

void SetLinkMenuVis::Visit(MenuMD& obj)
{
    SetLinkMenu& slm = obj.GetData<SetLinkMenu>();
    Gtk::Menu& lnk_list = NewManaged<Gtk::Menu>();
    slm.linkMenu = &lnk_list;

    MenuPack& mp = obj.GetData<MenuPack>();
    if( !mp.editor )
    {
        LOG_ERR << "SetLinkMenuVis::Visit: where is editor?" << io::endl;
        return;
    }
    MediaItem def_mi = slm.newLink;

    Gtk::RadioButtonGroup grp;
    // создаем пустой по умолчанию; он будет нажат, иначе им будет No Link
    // и при вызове set_active() он тоже отработает ("отжали кнопку"); а все потому,
    // что используем signal_activate вместо signal_toggled
    Gtk::RadioMenuItem empty_itm(grp);
    // No Link
    AddMediaItemChoice(lnk_list, MediaItem(), grp, slm, mp.editor);
    lnk_list.append(NewManaged<Gtk::SeparatorMenuItem>());
    // * Menus
    if( !slm.isForBack )
    {
        if( mnStore->children().size() )
        {
            for( MenuStore::iterator itr = mnStore->children().begin(), end = mnStore->children().end();
                 itr != end; ++itr )
                AddMediaItemChoice(lnk_list, mnStore->GetMedia(itr), grp, slm, mp.editor);
        }
        else
            AddNoStaffItem(lnk_list, "<No Menu>");
        lnk_list.append(NewManaged<Gtk::SeparatorMenuItem>());
    }

    // * Medias
    if( mdStore->children().size() )
    {
        for( MediaStore::iterator itr = mdStore->children().begin(), end = mdStore->children().end();
             itr != end; ++itr )
        {
            MediaItem mi = mdStore->GetMedia(itr);
            if( VideoItem vd = IsVideo(mi) )
            {
                if( vd->List().size() )
                {
                    Gtk::MenuItem& mn_itm = AppendMI(lnk_list, NewManaged<Gtk::MenuItem>(MakeMediaItemNameForMenu(mi)));
                    Gtk::Menu& chp_menu = MakeSubmenu(mn_itm);

                    AddMediaItemChoice(chp_menu, mi, grp, slm, mp.editor, mi->mdName);
                    chp_menu.append(NewManaged<Gtk::SeparatorMenuItem>());

                    for( VideoMD::Itr itr = vd->List().begin(), end = vd->List().end(); itr != end; ++itr )
                    {
                        ChapterItem chp = *itr;
                        AddMediaItemChoice(chp_menu, chp, grp, slm, mp.editor, chp->mdName);
                    }
                }
                else
                    AddMediaItemChoice(lnk_list, mi, grp, slm, mp.editor);
            }
            else if( ptr::dynamic_pointer_cast<StillImageMD>(mi) )
                AddMediaItemChoice(lnk_list, mi, grp, slm, mp.editor).set_sensitive(slm.isForBack);
        }
    }
    else
        AddNoStaffItem(lnk_list, "<No Media>");
}

void PackMenusWindow(Gtk::Container& contr, RefPtr<MenuStore> ms, RefPtr<MediaStore> md_store)
{
    MenuBrowser& menu_brw = NewManaged<MenuBrowser>(ms);

    Gtk::Label& title_lbl = NewManaged<Gtk::Label>();
    title_lbl.set_padding(0, 5);
    MEditorArea& meditor  = NewManaged<MEditorArea>();
    meditor.StandAlone() = false;
    RegisterOnChangeName(new EdtOnChangeNameVis(meditor, title_lbl));
    RegisterOnDelete(new EdtOnDeleteVis(meditor, title_lbl));
    RegisterHandler(new SetLinkMenuVis(ms, md_store), "SetLinkMenu");

    Gtk::HPaned& hp = NewManaged<Gtk::HPaned>();
    // :TRICKY:
    hp.set_position(250);
    contr.add(hp);

    // * бразуеры меню и медиа
    {
        Gtk::VBox& vbox = NewManaged<Gtk::VBox>();
        hp.add1(PackInScrolledWindow(vbox));

        // * subj
        vbox.pack_start(MakeTitleLabel("Menu List"), Gtk::PACK_SHRINK);
        PackHSeparator(vbox);
        vbox.pack_start(menu_brw);
        PackHSeparator(vbox);

        // * группа кнопок
        Gtk::EventBox& eb = NewManaged<Gtk::EventBox>();
        eb.modify_bg(Gtk::STATE_NORMAL, Gdk::Color("white")); // устанавливается при "реализации" виджета
        vbox.pack_start(eb, Gtk::PACK_SHRINK);

        Gtk::Alignment& alg = NewManaged<Gtk::Alignment>();
        alg.set_padding(0, 0, 2, 2);
        eb.add(alg);

        //Gtk::HBox& hb = *Gtk::manage(new Gtk::HBox(true, 4));
        Gtk::HButtonBox& hb = CreateMListButtonBox();
        alg.add(hb);
        {
            using namespace boost;
            Gtk::Button* add_btn = CreateButtonWithIcon("", Gtk::Stock::ADD,
                                                        "Add Menu");
            hb.pack_start(*add_btn);
            add_btn->signal_clicked().connect(lambda::bind(&InsertMenuIntoBrowser, boost::ref(menu_brw)));

            Gtk::Button* rm_btn = CreateButtonWithIcon("", Gtk::Stock::REMOVE,
                                                       "Remove Menu");
            hb.pack_start(*rm_btn);
            rm_btn->signal_clicked().connect(lambda::bind(&DeleteMenuFromBrowser, boost::ref(menu_brw)));

            Gtk::Button* edit_btn = CreateButtonWithIcon("Edit", Gtk::Stock::YES,
                                                         "Edit Menu");
            hb.pack_start(*edit_btn);
            ActionFunctor edit_fnr =
                lambda::bind(&EditMenu, boost::ref(menu_brw), boost::ref(meditor), boost::ref(title_lbl));
            edit_btn->signal_clicked().connect(edit_fnr);
            menu_brw.signal_row_activated().connect( lambda::bind(&OnMenuBrowserRowActivated, edit_fnr) );
        }

        // *
        PackHSeparator(vbox);
        vbox.pack_start(MakeTitleLabel("Media List"), Gtk::PACK_SHRINK);
        PackHSeparator(vbox);

        MediaBrowser& brw = NewManaged<MediaBrowser>(md_store);
        vbox.pack_start(brw);
    }

    // * MEditor
    {
        Gtk::VBox& vbox = NewManaged<Gtk::VBox>();
        hp.add2(vbox);
        // *
        vbox.pack_start(title_lbl, Gtk::PACK_SHRINK);
        // *
        Editor::PackToolbar(meditor, vbox);
        // *
        //SetVideoBackground(meditor);
        //hp.add2(PackWidgetInFrame(meditor, Gtk::SHADOW_ETCHED_IN));
        //hp.add2(meditor);
        vbox.pack_start(meditor);
    }

}

} // namespace Project

