//
// mgui/project/menu-browser.cpp
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

#include "media-browser.h"
#include "menu-browser.h"
#include "handler.h"

#include <mgui/editor/toolbar.h>
#include <mgui/timeline/mviewer.h>
#include <mgui/render/menu.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/widget.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/menu.h>
#include <mgui/dialog.h>
#include <mgui/gettext.h>
#include <mgui/init.h>
#include <mgui/win_utils.h>
#include <mgui/key.h>

#include <mbase/project/table.h> // MakeMenu()

#include <mlib/sdk/logger.h>
#include <mlib/sigc.h>

#include <gtk/gtkversion.h>

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

///////////////////////////////
// Настройки меню

static Gtk::RadioButton& AddAudioChoice(const char* label, Gtk::VBox& vbox, Gtk::RadioButtonGroup& grp,
                                        Gtk::Widget& wdg, RefPtr<Gtk::SizeGroup> sg)
{
    Gtk::RadioButton& r_btn = *Gtk::manage(new Gtk::RadioButton(grp, label, true));
    PackNamedWidget(vbox, r_btn, wdg, sg, Gtk::PACK_EXPAND_WIDGET);
    return r_btn;
}

static void OnMotionChoice(Gtk::CheckButton& mtn_btn, Gtk::VBox& vbox)
{
    vbox.set_sensitive(mtn_btn.get_active());
}

static void OnAudioTypeChoice(Gtk::RadioButton& prj_btn, Gtk::Button& a_btn,
                              Gtk::FileChooserButton& ext_btn)
{
    bool is_project = prj_btn.get_active();
    a_btn.set_sensitive(is_project);
    ext_btn.set_sensitive(!is_project);
}

static void SetMediaLabel(Gtk::Label& lbl, MediaItem mi)
{
    std::string text(_("No Link"));
    if( mi )
    {
        text = mi->mdName;
        if( ChapterItem ci = IsChapter(mi) )
            // \xc2\xbb \xe2\x80\xa3 \xe2\x80\xba \xe2\x96\xb8
            text = boost::format("%1% \xe2\x80\xa3 %2%") % ci->owner->mdName % text % bf::stop;
    }

    lbl.set_text(text);
}

// выпадающее меню вместо нажатия (потому ToggleButton)
class MenuButton: public Gtk::ToggleButton
{
    public:
            MenuButton();

    protected:
           Gtk::Label& label;

    virtual CommonMenuBuilder* MakeBuilder() = 0;

bool OnLinkButtonPress(GdkEventButton *event);
};

class AudioButton: public MenuButton
{
    typedef MenuButton MyParent;
    public:

 MediaItem  GetMI() { return mItem; }
      void  SetMI(MediaItem mi);

    protected:
            MediaItem  mItem;

    virtual CommonMenuBuilder* MakeBuilder();
};

class MenuPAButton: public MenuButton
{
    typedef MenuButton MyParent;
    public:

   PostAction  PAction() { return pAct; }
         void  SetPAction(const PostAction& pa)
         {
             pAct = pa;
             Update();
         }

         void  Update();

    protected:
            PostAction  pAct;

    virtual CommonMenuBuilder* MakeBuilder();
};

MenuButton::MenuButton(): label(NewManaged<Gtk::Label>())
{
    Gtk::Button& a_btn = *this;
    sig::connect(a_btn.signal_button_press_event(), bb::bind(&MenuButton::OnLinkButtonPress, this, _1), false);
    Gtk::HBox& box = Add(a_btn, NewManaged<Gtk::HBox>(false, 4));

    //priv->image = gtk_image_new ();
    //gtk_box_pack_start (GTK_BOX (box), priv->image, FALSE, FALSE, 0);
    //gtk_widget_show (priv->image);

    //SetMediaLabel(label, mItem);
    Gtk::Label& lbl = PackStart(box, label, Gtk::PACK_EXPAND_WIDGET);
    lbl.set_ellipsize(Pango::ELLIPSIZE_END);
    SetAlign(lbl);

    PackStart(box, NewManaged<Gtk::VSeparator>());
    //PackStart(box, NewManaged<Gtk::Image>(Gtk::Stock::OPEN, Gtk::ICON_SIZE_MENU));
    PackStart(box, NewManaged<Gtk::Arrow>(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE));
}

Rect GetAllocation(Gtk::Widget& wdg);

static void CoordBelow(Gtk::ToggleButton* btn, int& x, int& y, bool& push_in)
{
    RefPtr<Gdk::Window> win = btn->get_window();
    ASSERT( win );
    Rect plc = GetAllocation(*btn);
// точно версию не знаю, но это и не важно
#if GTK_CHECK_VERSION(2,18,0)
    win->get_root_coords(plc.lft, plc.btm, x, y);
#else
    win->get_origin(x, y);
    x += plc.lft;
    y += plc.btm; 
#endif

    push_in = true;
}

// по аналогии с GtkComboBox
bool MenuButton::OnLinkButtonPress(GdkEventButton *event)
{
    if( event->type == GDK_BUTTON_PRESS && IsLeftButton(event) )
    {
        //if( focus_on_click && !GTK_WIDGET_HAS_FOCUS(priv->button) )
        //    gtk_widget_grab_focus (priv->button);

        ptr::one<CommonMenuBuilder> bld(MakeBuilder());
        Gtk::Menu& mn = bld->Create();
        SetDeleteOnDone(mn);

        // интерактив кнопки
        set_active(true);
        mn.signal_hide().connect(bb::bind(&Gtk::ToggleButton::set_active, this, false));

        //Popup(mn, event, true);
    	mn.show_all();
    	mn.popup(bb::bind(&CoordBelow, this, _1, _2, _3), event->button, event->time);

        return true;
    }
    return false;
}

void AudioButton::SetMI(MediaItem mi)
{
    mItem = mi;
    SetMediaLabel(label, mItem);
}

class AudioMenuBld: public CommonMenuBuilder
{
    typedef CommonMenuBuilder MyParent;
    public:
                    AudioMenuBld(AudioButton* btn_): 
                        MyParent(btn_->GetMI(), true, true), aBtn(btn_) {}

    virtual ActionFunctor CreateAction(Project::MediaItem mi)
    {
        return bb::bind(&AudioButton::SetMI, aBtn, mi);
    }

    protected:
        AudioButton* aBtn;
};

CommonMenuBuilder* AudioButton::MakeBuilder() { return new AudioMenuBld(this); }

std::string Pat2Name(PostActionType pat, bool is_video)
{
    std::string res;
    switch( pat )
    {
    case patAUTO:
        if( is_video )
        {
            const char* real_cmd = MenusCnt() ? _("Previous Menu") : _("Next Video") ;
            res = BF_("Auto (%1%)") % real_cmd % bf::stop;
        }
        else
            res = _("Loop");
        break;
    case patNEXT_TITLE:
        if( is_video )
            res = _("Next Video");
        break;
    case patPLAY_ALL:
        res = _("Play All");
    case patEXP_LINK:
    default:
        // запрещены
        break;
    }

    ASSERT( !res.empty() );
    return res;
}

void AddPA(EndActionMenuBld& bld, PostActionType pat, bool is_video)
{
    bld.AddConstantItem(Pat2Name(pat, is_video), pat);
}

static void MenuAddConstantChoice(EndActionMenuBld& bld)
{
    AddPA(bld, patAUTO,     false);
    AddPA(bld, patPLAY_ALL, false);
}

CommonMenuBuilder* MenuPAButton::MakeBuilder() 
{ 
    return new EndActionMenuBld(pAct, bb::bind(&MenuPAButton::Update, this),
                                &MenuAddConstantChoice); 
}

void MenuPAButton::Update()
{
    if( pAct.paTyp == patEXP_LINK )
        SetMediaLabel(label, pAct.paLink);
    else
        label.set_markup(BoldItalicText(Pat2Name(pAct.paTyp, false)));
}

static void OnDurationChanged(Gtk::SpinButton& dur_btn, Gtk::Label& sz_lbl)
{
    std::string str = ShortSizeString(FFmpegSizeForDVD(dur_btn.get_value()));
    sz_lbl.set_label(str);
}

static DialogVBox& AddDialogPage(Gtk::Notebook& nbook, const char* label, bool homogeneous = false)
{
    DialogVBox& vbox = NewManaged<DialogVBox>(homogeneous, 10);
    vbox.set_border_width(5);
    nbook.append_page(vbox, label, true);

    return vbox;
}

static void RestoreSPColors(Gtk::ColorButton& sel_btn, Gtk::ColorButton& act_btn)
{
    SetColor(sel_btn, RGBA::Pixel(HIGH_CLR));
    SetColor(act_btn, RGBA::Pixel(SELECT_CLR));
}

static void PackColorButton(DialogVBox& vbox, Gtk::ColorButton& btn, const RGBA::Pixel& pxl,
                            const char* label)
{
    ConfigureRGBAButton(btn, pxl);
    AppendWithLabel(vbox, btn, label);
}

void MenuSettings(Menu mn, Gtk::Dialog& dlg)
{
    //DialogVBox& vbox_all = AddHIGedVBox(dlg);
    Gtk::Notebook& nbook = Add(*dlg.get_vbox(), NewManaged<Gtk::Notebook>());
    nbook.set_border_width(2);

    MotionData& mtn_dat = mn->MtnData();
    Gtk::CheckButton& mtn_btn = NewManaged<Gtk::CheckButton>();
    Gtk::SpinButton&  dur_btn = NewManaged<Gtk::SpinButton>();
    Gtk::CheckButton& sp_btn  = NewManaged<Gtk::CheckButton>(_("_Still picture"), true);
    
    Gtk::RadioButton* prj_choice = 0;
    AudioButton& a_btn = NewManaged<AudioButton>();
    Gtk::FileChooserButton& a_ext_btn = NewManaged<Gtk::FileChooserButton>(
        _("Select external audio file"), Gtk::FILE_CHOOSER_ACTION_OPEN);
    MenuPAButton& pa_btn = NewManaged<MenuPAButton>();
    {
        Gtk::VBox& vbox_all = AddDialogPage(nbook, _("_Motion menu"));

        Add(mtn_btn, NewBoldLabel(_("_Motion menu")));
        mtn_btn.set_active(mtn_dat.isMotion);
        PackStart(vbox_all, mtn_btn);

        // вроде и без отступа хорошо
        DialogVBox& vbox = PackDialogVBox(vbox_all);
        OnMotionChoice(mtn_btn, vbox);
        mtn_btn.signal_toggled().connect(bb::bind(&OnMotionChoice, boost::ref(mtn_btn), boost::ref(vbox)));

        // основное
        {
            Gtk::VBox& box = PackParaBox(vbox);
            // длительность
            {
                ConfigureSpin(dur_btn, mtn_dat.duration, MAX_MOTION_DURATION);
                Gtk::Label& sz_lbl = Pack2NamedWidget(box, SMCLN_("_Duration (in seconds)"), dur_btn,
                                                      vbox.labelSg);

                OnDurationChanged(dur_btn, sz_lbl);
                dur_btn.signal_value_changed().connect(bb::bind(&OnDurationChanged, b::ref(dur_btn), b::ref(sz_lbl)));
            }

            // Still Picture
            sp_btn.set_active(mtn_dat.isStillPicture);
            SetTip(sp_btn, _("Still menu with audio in the background"));
            PackStart(box, sp_btn);
        }

    	// аудио
        {
            Gtk::VBox& a_box = PackParaBox(vbox, _("Audio"));
            Gtk::RadioButtonGroup grp;

            a_btn.SetMI(mtn_dat.audioRef.lock());
            prj_choice = &AddAudioChoice(SMCLN_("_From the project"), a_box, grp, a_btn, vbox.labelSg);

            SetFilename(a_ext_btn, mtn_dat.audioExtPath);
            Gtk::RadioButton& ext_choice = AddAudioChoice(
                SMCLN_("_External audio"), a_box, grp, a_ext_btn, vbox.labelSg);
            if( !mtn_dat.isIntAudio )
                ext_choice.set_active(true);

            OnAudioTypeChoice(*prj_choice, a_btn, a_ext_btn);
            prj_choice->signal_toggled().connect(
                bb::bind(&OnAudioTypeChoice, boost::ref(*prj_choice), boost::ref(a_btn), 
                         boost::ref(a_ext_btn)));
        }

        pa_btn.SetPAction(mtn_dat.pAct);
        // постдействие
        {
            Gtk::VBox& pa_box = PackParaBox(vbox, _("End Action"));
            PackNamedWidget(pa_box, NewManaged<Gtk::Label>(), pa_btn, vbox.labelSg, Gtk::PACK_EXPAND_WIDGET);
        }
    }

    // цвета для субкартинок (subpicture)
    SubpicturePalette& pal = mn->subPal;
    Gtk::ColorButton& sel_btn = NewManaged<Gtk::ColorButton>();
    Gtk::ColorButton& act_btn = NewManaged<Gtk::ColorButton>();
    {
        DialogVBox& vbox = AddDialogPage(nbook, _("_Subpicture Colors"), true);

        PackColorButton(vbox, sel_btn, pal.selClr, SMCLN_("S_elected item color"));
        PackColorButton(vbox, act_btn, pal.actClr, SMCLN_("_Activated item color"));

        Gtk::Button& btn = PackStart(vbox, NewManaged<Gtk::Button>(_("_Restore default colors"), true));
        btn.signal_clicked().connect(bb::bind(&RestoreSPColors, b::ref(sel_btn), b::ref(act_btn)));
    }

    if( CompleteAndRunOk(dlg) )
    {
        mtn_dat.isMotion = mtn_btn.get_active();
        mtn_dat.duration = dur_btn.get_value();
        mtn_dat.isStillPicture = sp_btn.get_active();

        mtn_dat.isIntAudio   = prj_choice->get_active();
        mtn_dat.audioRef     = a_btn.GetMI();
        mtn_dat.audioExtPath = a_ext_btn.get_filename();

        mtn_dat.pAct = pa_btn.PAction();

        pal.selClr = GetColor(sel_btn);
        pal.actClr = GetColor(act_btn);

        // факт изменения отобразить
        void RedrawThumbnail(MediaItem mi);
        RedrawThumbnail(mn);
        // :TRICKY: рассмотреть вариант обновления через обработчик,
        // потому как вызов UpdateDVDSize() требует наличия Application()
        // (и валится в противном случае)
        UpdateDVDSize();
    }
}

///////////////////////////////

DialogParams MenuSettingsDialog(Menu mn, Gtk::Widget* par_wdg)
{
    return DialogParams(_("Menu Settings"), bb::bind(&MenuSettings, mn, _1),
                        420, par_wdg);
}

static void CopyMenu(Menu orig_mn, ObjectBrowser& brw)
{
    std::string name = BF_("%1% copy") % orig_mn->mdName % bf::stop;
    Menu mn = MakeMenu(name, orig_mn->Params().GetAF());

    mn->MtnData() = orig_mn->MtnData();
    mn->subPal    = orig_mn->subPal;
    SaveMenu(mn, orig_mn);
    
    InsertMenuIntoBrowser(static_cast<MenuBrowser&>(brw), mn);
}

static void OnRightButton(ObjectBrowser& brw, MediaItem mi, GdkEventButton* event)
{
    Menu mn = IsMenu(mi);
    ASSERT(mn);

    Gtk::Menu& gmn = NewPopupMenu();
    AppendRenameAction(gmn, brw);
    AddEnabledItem(gmn, _("Copy Menu"), bb::bind(&CopyMenu, mn, b::ref(brw)));
    AddDialogItem(gmn, MenuSettingsDialog(mn, &brw));
    
    Popup(gmn, event, true);
}

MenuBrowser::MenuBrowser(RefPtr<MenuStore> ms)
{
    set_name("MenuBrowser");
    set_model(ms);
    SetupBrowser(*this, ms->columns.menu.index(), false);

    const MenuStore::TrackFields& trk_fields = ms->columns;

    // 1 миниатюра + имя
    Gtk::CellRendererText& rndr = AppendNameColumn(*this, trk_fields.thumbnail, false, ms);
    rndr.property_size_points() = 13.0; // синхронизировать со стилем "MenuEntryStyle"
    rndr.property_xpad() = 5;
    //rndr.property_weight() = PANGO_WEIGHT_BOLD;

    SetOnRightButton(*this, bb::bind(&OnRightButton, _1, _2, _3));
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

static void AddNoStaffItem(Gtk::Menu& lnk_list, const std::string& label)
{
    Gtk::MenuItem& mn_itm = NewManaged<Gtk::MenuItem>(Glib::ustring(label));
    mn_itm.set_sensitive(false);
    lnk_list.append(mn_itm);
}

static std::string MakeMediaItemNameForMenu(MediaItem mi)
{
    return mi ? mi->mdName + " (" + gettext(mi->TypeString().c_str()) + ")" : _("No Link") ;
}

void AppendRadioItem(Gtk::RadioMenuItem& itm, bool is_active, const ActionFunctor& fnr, Gtk::Menu& lnk_list)
{
    itm.set_active(is_active);
    // .connect() строго после установки set_active()
    SetForRadioToggle(itm, fnr);
    lnk_list.append(itm);
}

CommonMenuBuilder::CommonMenuBuilder(MediaItem cur_itm, bool for_poster, bool only_with_audio): 
   curItm(cur_itm), forPoster(for_poster), resMenu(NewManaged<Gtk::Menu>()), onlyWithAudio(only_with_audio) {}

static Gtk::RadioMenuItem& 
AddMediaItemChoice(CommonMenuBuilder& mb, Gtk::Menu& lnk_list, MediaItem mi, const std::string& name,
                   bool exp_link)
{
    std::string itm_name = name.empty() ? MakeMediaItemNameForMenu(mi) : name ;

    Gtk::RadioMenuItem& itm = NewManaged<Gtk::RadioMenuItem>(mb.radioGrp, itm_name);
    AppendRadioItem(itm, exp_link && (mi == mb.curItm), mb.CreateAction(mi), lnk_list);
    return itm;
}

Gtk::RadioMenuItem& 
CommonMenuBuilder::AddMediaItemChoice(Gtk::Menu& lnk_list, MediaItem mi, const std::string& name)
{
    return Project::AddMediaItemChoice(*this, lnk_list, mi, name, true);
}

void CommonMenuBuilder::AddPredefinedItem(const std::string& label, bool is_active, const ActionFunctor& fnr)
{
    Gtk::RadioMenuItem& itm = NewManaged<Gtk::RadioMenuItem>(radioGrp);
    SetAlign(Add(itm, NewBoldItalicLabel(label)));
    AppendRadioItem(itm, is_active, fnr, resMenu);
}

void AddNoLinkItem(CommonMenuBuilder& mb, bool exp_link)
{
    AddMediaItemChoice(mb, mb.resMenu, MediaItem(), "", exp_link);
}

void CommonMenuBuilder::AddConstantChoice()
{
    AddNoLinkItem(*this, true);
}

Gtk::Menu& CommonMenuBuilder::Create()
{
    AStores& as = GetAStores();
    RefPtr<MediaStore> mdStore = as.mdStore;
    RefPtr<MenuStore>  mnStore = as.mnStore;

    Gtk::Menu& lnk_list        = resMenu;
    Gtk::RadioButtonGroup& grp = radioGrp;

    // создаем пустой по умолчанию; он будет нажат, иначе им будет No Link
    // и при вызове set_active() он тоже отработает ("отжали кнопку")
    Gtk::RadioMenuItem empty_itm(grp);

    AddConstantChoice();
    lnk_list.append(NewManaged<Gtk::SeparatorMenuItem>());
    // * Menus
    if( !forPoster )
    {
        if( Size(mnStore) )
        {
            for( MenuStore::iterator itr = mnStore->children().begin(), end = mnStore->children().end();
                 itr != end; ++itr )
                AddMediaItemChoice(lnk_list, mnStore->GetMedia(itr));
        }
        else
            AddNoStaffItem(lnk_list, "<No Menu>");
        lnk_list.append(NewManaged<Gtk::SeparatorMenuItem>());
    }

    // * Medias
    // :TRICKY: mdStore может быть нулем в тесте test_meditor.cpp
    if( mdStore && Size(mdStore) )
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

                    AddMediaItemChoice(chp_menu, mi, mi->mdName);
                    chp_menu.append(NewManaged<Gtk::SeparatorMenuItem>());

                    for( VideoMD::Itr itr = vd->List().begin(), end = vd->List().end(); itr != end; ++itr )
                    {
                        ChapterItem chp = *itr;
                        AddMediaItemChoice(chp_menu, chp, chp->mdName);
                    }
                }
                else
                    AddMediaItemChoice(lnk_list, mi);
            }
            else if( ptr::dynamic_pointer_cast<StillImageMD>(mi) )
            {
                bool is_sensitive = forPoster && !onlyWithAudio;
                AddMediaItemChoice(lnk_list, mi).set_sensitive(is_sensitive);
            }
        }
    }
    else
        AddNoStaffItem(lnk_list, "<No Media>");

    return lnk_list;
}

//class SetLinkMenuVis: public ObjVisitor
//{
//    public:
//                SetLinkMenuVis(RefPtr<MenuStore> mn_store, RefPtr<MediaStore> md_store)
//                    :mnStore(mn_store), mdStore(md_store) {}
//
// virtual  void  Visit(MenuMD& obj);
//
//    protected:
//        RefPtr<MenuStore> mnStore;
//       RefPtr<MediaStore> mdStore;
//};
//
//void SetLinkMenuVis::Visit(MenuMD& obj)
//{
//    SetLinkMenu& slm = obj.GetData<SetLinkMenu>();
//
//    MenuPack& mp = obj.GetData<MenuPack>();
//    if( !mp.editor )
//    {
//        LOG_ERR << "SetLinkMenuVis::Visit: where is editor?" << io::endl;
//        return;
//    }
//    slm.linkMenu = &LinkMenuBuilder(slm, *mp.editor).Create();
//}

void OnNewMenu(MenuBrowser& brw)
{
    Menu mn = MakeMenu(MenuAutoName(MenusCnt()), IsMenuToBe4_3() ? af4_3 : af16_9);
    InsertMenuIntoBrowser(brw, mn);
}

void PackMenusWindow(Gtk::Container& contr, RefPtr<MenuStore> ms, RefPtr<MediaStore> md_store)
{
    MenuBrowser& menu_brw = NewManaged<MenuBrowser>(ms);

    Gtk::Label& title_lbl = NewManaged<Gtk::Label>();
    title_lbl.set_padding(0, 5);
    MEditorArea& meditor = MenuEditor();
    meditor.StandAlone() = false;
    RegisterOnChangeName(new EdtOnChangeNameVis(meditor, title_lbl));
    RegisterOnDelete(new EdtOnDeleteVis(meditor, title_lbl));
    //RegisterHandler(new SetLinkMenuVis(ms, md_store), "SetLinkMenu");

    Gtk::HPaned& hp = NewManaged<Gtk::HPaned>();
    // :TRICKY:
    hp.set_position(250);
    contr.add(hp);

    // * бразуеры меню и медиа
    {
        Gtk::VBox& vbox = NewManaged<Gtk::VBox>();
        hp.add1(PackInScrolledWindow(vbox));

        // * subj
        vbox.pack_start(MakeTitleLabel(_("Menu List")), Gtk::PACK_SHRINK);
        PackHSeparator(vbox);
        vbox.pack_start(menu_brw);
        PackHSeparator(vbox);

        // * группа кнопок
        Gtk::EventBox& eb = NewManaged<Gtk::EventBox>();
        eb.modify_bg(Gtk::STATE_NORMAL, Gdk::Color("white")); // устанавливается при "реализации" виджета
        vbox.pack_start(eb, Gtk::PACK_SHRINK);

        Gtk::Alignment& alg = Add(eb, NewPaddingAlg(0, 0, 2, 2));
        //Gtk::HBox& hb = *Gtk::manage(new Gtk::HBox(true, 4));
        Gtk::HButtonBox& hb = Add(alg, CreateMListButtonBox());
        {
            Gtk::Button* add_btn = CreateButtonWithIcon("", Gtk::Stock::ADD,
                                                        _("Add Menu"));
            hb.pack_start(*add_btn);
            add_btn->signal_clicked().connect(bb::bind(&OnNewMenu, boost::ref(menu_brw)));

            Gtk::Button* rm_btn = CreateButtonWithIcon("", Gtk::Stock::REMOVE,
                                                       _("Remove Menu"));
            hb.pack_start(*rm_btn);
            rm_btn->signal_clicked().connect(bb::bind(&DeleteMenuFromBrowser, boost::ref(menu_brw)));
            //const char* edit_text = C_("MenuBrowser", "Edit");
            const char* edit_text = "";
            Gtk::Button* edit_btn = CreateButtonWithIcon(edit_text, Gtk::Stock::YES, _("Edit Menu"));
            hb.pack_start(*edit_btn);
            ActionFunctor edit_fnr =
                bb::bind(&EditMenu, boost::ref(menu_brw), boost::ref(meditor), boost::ref(title_lbl));
            edit_btn->signal_clicked().connect(edit_fnr);
            menu_brw.signal_row_activated().connect( bb::bind(&OnMenuBrowserRowActivated, edit_fnr) );
        }

        // *
        PackHSeparator(vbox);
        vbox.pack_start(MakeTitleLabel(_("Media List")), Gtk::PACK_SHRINK);
        PackHSeparator(vbox);

        MediaBrowser& brw = NewManaged<MediaBrowser>(md_store, false);
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

static RefPtr<Project::MenuStore> GetMenuStore()
{
    return GetAStores().mnStore;
}

static Menu ToMenuTransform(const Gtk::TreeRow& row) 
{
    return GetMenu(GetMenuStore(), row);
}

fe::range<Menu> AllMenus()
{
    return fe::make_any( GetMenuStore()->children() | fe::transformed(ToMenuTransform) );
}

} // namespace Project

