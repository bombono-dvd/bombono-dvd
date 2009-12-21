//
// mgui/project/mconstructor.cpp
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

#include "mconstructor.h"
#include "menu-actions.h"
#include "menu-render.h"
#include "mb-actions.h"
#include <mgui/text_obj.h> // CalcAbsSizes()

#include <mgui/win_utils.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/menu.h>
#include <mgui/sdk/widget.h>
#include <mgui/trackwindow.h>
#include <mgui/dialog.h> // ChooseFileSaveTo()
#include <mgui/img-factory.h> // GetFactoryImage()
#include <mgui/author/script.h> // VideoSizeSum()
#include <mgui/author/output.h> // PackOutput()

#include <mgui/project/handler.h> // RegisterHook()
#include <mlib/sigc.h>
#include <mlib/filesystem.h>

#include <gtk/gtkaboutdialog.h>
#include <gtk/gtklinkbutton.h> // gtk_button_set_relief

namespace Project
{

void PackMediasWindow(Gtk::Container& contr, RefPtr<MediaStore> ms, MediasWindowPacker pack_fnr)
{
    MediaBrowser& brw = NewManaged<MediaBrowser>(ms);
    pack_fnr(contr, brw);
}

void PackFullMBrowser(Gtk::Container& contr, MediaBrowser& brw)
{
    using namespace boost;
    PackTrackWindow(contr, lambda::bind(&PackMBWindow, lambda::_1, lambda::_2, lambda::_3, boost::ref(brw)));
}

void PackMediasWindow(Gtk::Container& contr, RefPtr<MediaStore> ms)
{
    PackMediasWindow(contr, ms, &Project::PackFullMBrowser);
}

void SetTabName(Gtk::Notebook& nbook, const std::string& name, int pos)
{
    Gtk::Label& lbl = NewManaged<Gtk::Label>("<span weight = \"bold\" style  = \"italic\">"
                                             + name + "</span>");
    lbl.set_use_markup(true);
    lbl.set_use_underline(true);

    // попытка сделать вертикальные закладки читабельными
    //switch( nbook.get_tab_pos() )
    //{
    //case Gtk::POS_LEFT:
    //case Gtk::POS_RIGHT:
    //    lbl.set_angle(+90.);
    //    {
    //        RefPtr<Pango::Layout> lay = lbl.get_layout();
    //        //lay->set_spacing(0);
    //        PangoContext* cnxt = lay->get_context()->gobj();
    //        pango_context_set_base_gravity(cnxt, PANGO_GRAVITY_EAST);
    //        pango_context_set_gravity_hint(cnxt, PANGO_GRAVITY_HINT_STRONG);
    //    }
    //    break;
    //default:
    //    ;
    //}

    nbook.set_tab_label(*nbook.get_nth_page(pos), lbl);
}

static void SaveProjectData(RefPtr<MenuStore> mn_store, RefPtr<MediaStore> md_store)
{
    ADatabase& db = AData();
    // * сохраняемся
    for( MediaStore::iterator itr = md_store->children().begin(), end = md_store->children().end();
         itr != end; ++itr )
        db.GetML().Insert(md_store->GetMedia(itr));
    for( MenuStore::iterator  itr = mn_store->children().begin(), end = mn_store->children().end();
         itr != end; ++itr )
    {
        Menu mn = GetMenu(mn_store, itr);
        Project::SaveMenu(mn);
        db.GetMN().Insert(mn);
    }
    db.SetOut(false);
    db.Save();
}

static std::string MakeProjectTitle(bool with_path_breakdown = false)
{
    ADatabase& db = AData();
    if( !db.IsProjectSet() )
        return "untitled.xml";

    fs::path full_path(db.GetProjectFName());
    std::string res_str = full_path.leaf();
    if( with_path_breakdown )
        res_str += " (" + full_path.branch_path().string() + ")";
    return res_str;
}

static bool SaveProjectAs(Gtk::Widget& for_wdg) 
{
    bool res = false;
    std::string fname = MakeProjectTitle();
    if( ChooseFileSaveTo(fname, "Save Project As...", for_wdg) )
    {
        ASSERT( !fname.empty() );
        AData().SetProjectFName(fname);
        res = true;
    }

    return res;
}

AStores& GetAStores()
{
    return AData().GetData<AStores>();
}

static void LoadProjectInteractive(const std::string& prj_file_name)
{
    ADatabase& db = AData();
    bool res = true;
    std::string err_str;
    try
    {
        void DbSerializeProjectImpl(Archieve& ar);
        db.LoadWithFnr(prj_file_name, DbSerializeProjectImpl);
    }
    catch (const std::exception& err)
    {
        res = false;
        err_str = err.what();
    }
    if( !res )
    {
        // мягкая очистка
        db.Clear(false);
        db.ClearSettings();
        MessageBox("Cant open project file \"" + prj_file_name + "\"", Gtk::MESSAGE_ERROR, 
                   Gtk::BUTTONS_OK, err_str);
    }
}

// создать списки медиа и меню
static AStores& InitAStores()
{
    RefPtr<MediaStore> md_store = CreateEmptyMediaStore();
    RefPtr<MenuStore>  mn_store = CreateEmptyMenuStore();

    AStores& as = GetAStores();
    as.mdStore  = md_store;
    as.mnStore  = mn_store;

    return as;
}

static void SetAppTitle(ConstructorApp& app, bool clear_change_flag = true);
static void UpdateDVDSize(SizeBar& sz_bar);

static void LoadApp(ConstructorApp& app, const std::string& fname)
{
    LoadProjectInteractive(ConvertPathToUtf8(fname));

    AStores& as = GetAStores();
    ADatabase& db = AData();

    PublishMediaStore(as.mdStore);
    PublishMenuStore(as.mnStore, db.GetMN());
    UpdateDVDSize(app.SB());

    db.SetOut(true);
    SetAppTitle(app);
}

AStores& InitAndLoadPrj(const std::string& fname)
{
    AStores& as = InitAStores();

    ADatabase& db = AData();
    db.Load(ConvertPathToUtf8(fname));

    PublishMediaStore(as.mdStore);
    PublishMenuStore(as.mnStore, db.GetMN());

    db.SetOut(true);
    return as;
}

static Rect GetAllocation(Gtk::Widget& wdg)
{
    return MakeRect(*wdg.get_allocation().gobj());
}

static bool EraseTabLineOnExpose(Gtk::Notebook& nbook, GdkEventExpose* event, bool erase_down,
				 Gtk::Notebook* book_tabs)
{
    Rect plc = GetAllocation(nbook);
    if( !plc.IsNull() )
    {
        plc.lft += 1;
        if( erase_down )
    	{
    	    plc.rgt -= 1;
    	    plc.top = plc.btm-1;
    	}
        else
    	{
            // стираем верхнюю линию рамки только до окончания верхних табов
    	    plc.rgt = std::min(plc.rgt, GetAllocation(*book_tabs).rgt)-1;
    	    plc.btm = plc.top+1;
    	}
        if( plc.Intersects(MakeRect(event->area)) )
        {
            Cairo::RefPtr<Cairo::Context> cr = nbook.get_window()->create_cairo_context();
            cr->set_line_width(erase_down ? 1.0 : 2.0);
            CR::SetColor(cr, GetBGColor(nbook));

            double y = plc.top + (erase_down ? H_P : 1);
            cr->move_to(plc.lft, y);
            cr->line_to(plc.rgt, y);
            cr->stroke();
        }
    }
    return true;
}

//
// COPY_N_PASTE_ETALON из go-file.c, проект Gnumeric, http://projects.gnome.org/gnumeric/
//

#ifndef GOFFICE_WITH_GNOME
static char *
check_program (char const *prog)
{
    if (NULL == prog)
        return NULL;
    if (g_path_is_absolute (prog))
    {
        if (!g_file_test (prog, G_FILE_TEST_IS_EXECUTABLE))
            return NULL;
    }
    else if (!g_find_program_in_path (prog))
        return NULL;
    return g_strdup (prog);
}
#endif

//GError *
static void
go_url_show(GtkAboutDialog*, const gchar* url, gpointer)
{
#ifdef G_OS_WIN32
    ShellExecute (NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);

    return; //NULL;
#else
    GError *err = NULL;
#ifdef GOFFICE_WITH_GNOME
    gnome_url_show (url, &err);
#else
    guint8 *browser = NULL;
    guint8 *clean_url = NULL;

    /* 1) Check BROWSER env var */
    browser = (guint8*)check_program (getenv ("BROWSER"));

    if (browser == NULL)
    {
        static char const * const browsers[] = {
            "sensible-browser", /* debian */
            "epiphany",     /* primary gnome */
            "galeon",       /* secondary gnome */
            "encompass",
            "firefox",
            "mozilla-firebird",
            "mozilla",
            "netscape",
            "konqueror",
            "xterm -e w3m",
            "xterm -e lynx",
            "xterm -e links"
        };
        unsigned i;
        for (i = 0 ; i < G_N_ELEMENTS (browsers) ; i++)
            if (NULL != (browser = (guint8*)check_program (browsers[i])))
                break;
    }

    if (browser != NULL)
    {
        gint    argc;
        gchar **argv = NULL;
        char   *cmd_line = g_strconcat ((gchar*)browser, " %1", NULL);

        if (g_shell_parse_argv (cmd_line, &argc, &argv, &err))
        {
            /* check for '%1' in an argument and substitute the url
             * otherwise append it */
            gint i;
            char *tmp;

            for (i = 1 ; i < argc ; i++)
                if (NULL != (tmp = strstr (argv[i], "%1")))
                {
                    *tmp = '\0';
                    tmp = g_strconcat (argv[i],
                                       (clean_url != NULL) ? (char const *)clean_url : url,
                                       tmp+2, NULL);
                    g_free (argv[i]);
                    argv[i] = tmp;
                    break;
                }

                /* there was actually a %1, drop the one we added */
            if (i != argc-1)
            {
                g_free (argv[argc-1]);
                argv[argc-1] = NULL;
            }
            g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                           NULL, NULL, NULL, &err);
            g_strfreev (argv);
        }
        g_free (cmd_line);
    }
    g_free (browser);
    g_free (clean_url);
#endif
#endif
    //return err;
    if( err )
        g_error_free(err);
}

//
// COPY_N_PASTE_ETALON_END из go-file.c, проект Gnumeric, http://projects.gnome.org/gnumeric/
//

static void HookUpURLs()
{
    static bool is_init = false;
    if( !is_init )
    {
        is_init = true;
        gtk_about_dialog_set_url_hook(go_url_show, NULL, NULL);
    }
}

static void SetNoButtonRelief(GtkWidget* wdg)
{
    if( GTK_IS_BUTTON(wdg) )
    {
        GtkReliefStyle style = GTK_IS_LINK_BUTTON(wdg) ? GTK_RELIEF_NORMAL : GTK_RELIEF_NONE ;
        gtk_button_set_relief(GTK_BUTTON(wdg), style);
    }
}

static void OnCloseAboutDlg(Gtk::AboutDialog* dlg, int reps)
{
    if( reps == Gtk::RESPONSE_CANCEL )
        dlg->hide();
}

static void OnDlgAbout(Gtk::Window& win)
{
    const char* license =
        "This program is free software; you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation; either version 2 of the License, or\n"
        "(at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA";

    HookUpURLs();

    //GtkWindow* gwin = win.gobj();
    //gtk_show_about_dialog(gwin,
    //                      "name", APROJECT_NAME,
    //                      "version", APROJECT_VERSION,
    //                      "copyright", "Copyright \xc2\xa9 2007-2009 Ilya Murav'jov",
    //                      "license", license,
    //                      "website", "http://www.bombono.org",
    //                      "comments", APROJECT_NAME" is a DVD authoring program with nice and clean GUI",
    //                      //"authors", authors,
    //                      //"documenters", documentors,
    //                      "logo", Gdk::Pixbuf::create_from_file(AppendPath(GetDataDir(), "about-front.png"))->gobj(),
    //                      //"decorated", 0,
    //                      NULL);
    //
    //if( GtkWidget* about_dlg = (GtkWidget*)g_object_get_data(G_OBJECT(gwin), "gtk-about-dialog") )
    //{
    //    gtk_widget_set_name(about_dlg, "AboutBombonoDVD");
    //    ForAllWidgets(about_dlg, SetNoButtonRelief);
    //}

    static ptr::one<Gtk::AboutDialog> dlg;
    if( !dlg )
    {
        dlg = new Gtk::AboutDialog;
        dlg->Gtk::Widget::set_name("AboutBombonoDVD");
        dlg->set_transient_for(win);
    
        dlg->set_name(APROJECT_NAME);
        dlg->set_version(APROJECT_VERSION);
        dlg->set_copyright("Copyright \xc2\xa9 2007-2009 Ilya Murav'jov");
        dlg->set_license(license);
        dlg->set_website("http://www.bombono.org");
        dlg->set_comments(APROJECT_NAME" is a DVD authoring program with nice and clean GUI");
        dlg->set_logo(Gdk::Pixbuf::create_from_file(AppendPath(GetDataDir(), "about-front.png")));
        //dlg.set_authors(authors);
        //dlg.set_documenters(documenters);
        //dlg.set_decorated(false);
    
        ForAllWidgets(static_cast<Gtk::Widget&>(*dlg).gobj(), SetNoButtonRelief);
        dlg->signal_response().connect(bl::bind(OnCloseAboutDlg, dlg.get(), bl::_1));
    }

    //dlg.run();
    dlg->show();
    dlg->present();
}


typedef Glib::ListHandle<Gtk::Widget*> WidgetListHandle;
static Gtk::RadioMenuItem* GetNthGo(ConstructorApp& app, int pos)
{
    WidgetListHandle children = app.GoMenu().get_children();
    int menu_cnt = (int)children.size();
    ASSERT( pos <= menu_cnt );

    Gtk::RadioMenuItem* itm = 0;
    // в момент создания закладки еще пункт меню не создан
    if( pos < menu_cnt )
    {
        WidgetListHandle::iterator it = children.begin();
        for( int i=0; i<pos; i++ )
            ++it;

        itm = dynamic_cast<Gtk::RadioMenuItem*>(*it);
    }
    return itm;
}

static void OnGoItemToggled(Gtk::RadioMenuItem& itm, ConstructorApp& app)
{
    if( itm.get_active() )
    {
        WidgetListHandle children = app.GoMenu().get_children();
        WidgetListHandle::iterator it = children.begin();
        int i=0;
        for( ; *it != &itm ; i++ )
            ++it;
    
        app.BookTabs().set_current_page(i);
    }
}

static Gtk::RadioMenuItem& CreateGoMenuItem(Gtk::RadioButtonGroup& grp, ConstructorApp& app)
{
    Gtk::RadioMenuItem& itm = NewManaged<Gtk::RadioMenuItem>(grp);
    itm.signal_toggled().connect(boost::lambda::bind(OnGoItemToggled, boost::ref(itm), boost::ref(app)));

    return itm;
}

static void OnContentPageAdd(ConstructorApp& app, guint page_num)
{
    Gtk::Notebook& nbook = app.BookTabs();
    int cnt = nbook.get_n_pages();
    ASSERT_RTL( app.BookContent().get_n_pages() == cnt + 1 );

    Gtk::Label& lbl = NewManaged<Gtk::Label>("");
    lbl.set_size_request(0, 0);
    nbook.insert_page(lbl, page_num);

    // меню Go
    if( cnt > 0 )
    {
        Gtk::Menu& go_menu = app.GoMenu();
        WidgetListHandle children = go_menu.get_children();
        ASSERT( (int)children.size() == cnt );

        // эти Radio-группы в gtkmm - "дурдом на выезде"!
        // 1) запрет на создание new - неужели по-другому нельзя!
        // 2) как и в RadioToolItem, здесь глючит set_group()
        Gtk::Widget* wdg = *children.begin();
        Gtk::RadioButtonGroup grp = dynamic_cast<Gtk::RadioMenuItem*>(wdg)->get_group();

        go_menu.insert(CreateGoMenuItem(grp, app), page_num);
    }
}

static void OnMenuNotebookSwitch(ConstructorApp& app, guint page_num)
{
    app.BookContent().set_current_page(page_num);
    //app.BookContent().get_nth_page(page_num)->child_focus(Gtk::DIR_TAB_FORWARD);

    if( Gtk::RadioMenuItem* itm = GetNthGo(app, page_num) )
        itm->set_active(true);
}

static Gtk::Menu& DetachMenuFromUI(RefPtr<Gtk::UIManager> mngr, const char* path)
{
    Gtk::MenuItem* mi = dynamic_cast<Gtk::MenuItem*>(mngr->get_widget(path));
    ASSERT( mi );
    Gtk::Menu& menu = *mi->get_submenu();
    // надо отсоединить от mi => увеличиваем счетчик
    //g_object_ref(menu.gobj());
    menu.reference();
    menu.detach();

    return menu;
}

static void SetAppTitle(ConstructorApp& app, bool clear_change_flag)
{
    if( clear_change_flag )
        app.isProjectChanged = false;

    const char* ch_flag = app.isProjectChanged ? "*" : "" ;
    app.win.set_title(ch_flag + MakeProjectTitle(true) + " - " APROJECT_NAME);
}

static void ClearStore(RefPtr<ObjectStore> os)
{
    Gtk::TreeModel::Children children = os->children();
    // с конца быстрее - не требуется реиндексация
    while( children.size() )
        DeleteMedia(os, --children.end());
}

static void NewProject()
{
    // * очищаем списки
    AStores& as = GetAStores();
    ClearStore(as.mnStore);
    ClearStore(as.mdStore);
    // * остальное в базе
    AData().ClearSettings();
}

static void OnSaveAsProject(ConstructorApp& app);
static void OnSaveProject(ConstructorApp& app);

// предложить пользователю сохранить измененный проект перед закрытием
static bool CheckBeforeClosing(ConstructorApp& app)
{
    bool res = true;
    if( app.isProjectChanged )
    {
    
        Gtk::MessageDialog dlg("<span weight=\"bold\" size=\"large\">" 
                               "Save changes to \"" + MakeProjectTitle() + "\"?" 
                               "</span>", true,  Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
        dlg.add_button("Close _without Saving", Gtk::RESPONSE_CLOSE);
        AddCancelSaveButtons(dlg);
    
        bool to_save = false;
        Gtk::ResponseType resp = (Gtk::ResponseType)dlg.run();
        switch( resp )
        {
        case Gtk::RESPONSE_CANCEL:
        case Gtk::RESPONSE_DELETE_EVENT:
            res = false;
            break;
        case Gtk::RESPONSE_OK:
        case Gtk::RESPONSE_CLOSE:
            to_save = resp == Gtk::RESPONSE_OK;
            break;
        default:
            ASSERT(0);
        }

        if( to_save )
            OnSaveProject(app);
    }
    return res;
}

static Gtk::RadioButton& TVSelectionButton(bool &is_pal, bool val, Gtk::RadioButtonGroup& grp)
{
    const char* name = val ? "_PAL/SECAM" : "_NTSC" ;
    Gtk::RadioButton& btn = *Gtk::manage(new Gtk::RadioButton(grp, name, true));
    using namespace boost; // boost::function<> - для приведения результата к void
    btn.signal_toggled().connect( function<void()>(lambda::var(is_pal) = val) );
    return btn;
}

static void OnNewProject(ConstructorApp& app)
{
    if( CheckBeforeClosing(app) )
    {
        bool is_pal = true;
        Gtk::Dialog new_prj_dlg("New Project", app.win, true, true);
        new_prj_dlg.set_name("NewProject");
        new_prj_dlg.set_resizable(false);
        {
            AddCancelDoButtons(new_prj_dlg, Gtk::Stock::OK);
            Gtk::VBox& dlg_box = *new_prj_dlg.get_vbox();
            // :REFACTOR: fs::path(GetDataDir())...
            PackStart(dlg_box, NewManaged<Gtk::Image>((fs::path(GetDataDir())/"cap400.png").string()));
            Gtk::Alignment& alg = PackStart(dlg_box, NewManaged<Gtk::Alignment>());
            alg.set_padding(10, 40, 20, 20);
            Gtk::VBox& vbox = Add(alg, NewManaged<Gtk::VBox>());
            
            PackStart(vbox, NewManaged<Gtk::Label>("Please select a Television standard for your project:",
                                                   0.0, 0.5, true));
            {
                Gtk::Alignment& alg = PackStart(vbox, NewManaged<Gtk::Alignment>());
                alg.set_padding(10, 10, 0, 0);
                Gtk::VBox& vbox2 = Add(alg, NewManaged<Gtk::VBox>());

                Gtk::RadioButtonGroup grp;
                PackStart(vbox2, TVSelectionButton(is_pal, true,  grp));
                PackStart(vbox2, TVSelectionButton(is_pal, false, grp));
            }
            dlg_box.show_all();
        }

        if( Gtk::RESPONSE_OK == new_prj_dlg.run() )
        {
            NewProject();
            AData().SetPalTvSystem(is_pal);
            SetAppTitle(app);
        }
    }
}

static void OnOpenProject(ConstructorApp& app)
{
    if( CheckBeforeClosing(app) )
    {
        Gtk::FileChooserDialog dialog("Open Project", Gtk::FILE_CHOOSER_ACTION_OPEN);
        BuildChooserDialog(dialog, true, app.win);
    
        Gtk::FileFilter prj_filter;
        prj_filter.set_name("Project files (*.xml)");
        prj_filter.add_pattern("*.xml");
        dialog.add_filter(prj_filter);
    
        Gtk::FileFilter all_filter;
        all_filter.set_name("All files");
        all_filter.add_pattern("*");
        dialog.add_filter(all_filter);
    
        if( Gtk::RESPONSE_OK == dialog.run() )
        {
            // в процессе загрузки не нужен
            dialog.hide();
            IteratePendingEvents();
    
            NewProject();
            AData().SetOut(false);

            LoadApp(app, dialog.get_filename());
        }
    }
}

static void SaveProject(ConstructorApp& app)
{
    ASSERT( AData().IsProjectSet() );

    AStores& as = GetAStores();
    RefPtr<MenuStore> mn_store = as.mnStore;
    SaveProjectData(mn_store, as.mdStore);
    // очистка после сохранения
    AData().SetOut(true);
    for( MenuStore::iterator itr = mn_store->children().begin(), end = mn_store->children().end();
         itr != end; ++itr )
        ClearMenuSavedData(GetMenu(mn_store, itr));

    SetAppTitle(app);
}

static void OnSaveProject(ConstructorApp& app)
{
    if( AData().IsProjectSet() )
        SaveProject(app);
    else
        OnSaveAsProject(app);
}

static void OnSaveAsProject(ConstructorApp& app)
{
    if( SaveProjectAs(app.win) )
        SaveProject(app);
}

static bool OnConstructorAppClose(ConstructorApp& app)
{
    return !app.askSaveOnExit || CheckBeforeClosing(app);
}

static void QuitApplication(ConstructorApp& app)
{
    // согласно политике gtkmm окна Gtk::Window не уничтожаются; признаком
    // выхода из цикла является скрытие того окна, которое было аргументом
    // функции Gtk::Main::run() = "обработка очереди сообщений пока не скрыто окно"
    if( OnConstructorAppClose(app) )
        app.win.hide();
}

static bool OnDeleteApp(ConstructorApp& app)
{
    QuitApplication(app);
    return true; // окно закрывает только QuitApplication()
}

static void OnChangeProject(ConstructorApp& app)
{
    if( !app.isProjectChanged )
    {
        app.isProjectChanged = true;
        SetAppTitle(app, false);
    }
}

static void ImportFromDVD(ConstructorApp& app)
{
    DVD::RunImport(app.win);
}

//////////////////////////////////////////////////
// 

enum DVDType
{
    dvdONE  = 0, // DVD-1, 1,4 Gb
    dvdFIVE = 1, // DVD-5, 4,7 Gb
    dvdNINE = 2, // DVD-9, 8,5 Gb

    dvdCNT,
};

static io::pos DVDSize[] =
{
    712891,
    2295104,
    4150390
};

static io::pos DVDTypeSize(DVDType typ)
{
    return DVD_PACK_SZ * DVDSize[typ];
}

//
// COPY_N_PASTE_ETALON из brasero_utils.c, Brasero, http://www.gnome.org/projects/brasero
//

enum {
	BRASERO_UTILS_NO_UNIT,
	BRASERO_UTILS_KO,
	BRASERO_UTILS_MO,
	BRASERO_UTILS_GO
};

#define N_(arg) (arg)
#define _(arg)  (arg)

static gchar* 
brasero_utils_get_size_string(gint64 dsize,
                              gboolean with_unit,
                              gboolean round)
{
	int unit;
	int size;
	int remain = 0;
	const char *units[] = { "", N_("KiB"), N_("MiB"), N_("GiB") };

	if (dsize < 1024) {
		unit = BRASERO_UTILS_NO_UNIT;
		size = (int) dsize;
		goto end;
	}

	size = (int) (dsize / 1024);
	if (size < 1024) {
		unit = BRASERO_UTILS_KO;
		goto end;
	}

	size = (int) (size / 1024);
	if (size < 1024) {
		unit = BRASERO_UTILS_MO;
		goto end;
	}

	remain = (size % 1024) / 100;
	size = size / 1024;
	unit = BRASERO_UTILS_GO;

      end:

	if (round && size > 10) {
		gint remains;

		remains = size % 10;
		size -= remains;
	}

	if (with_unit == TRUE && unit != BRASERO_UTILS_NO_UNIT) {
		if (remain)
			return g_strdup_printf ("%i.%i %s",
						size,
						remain,
						_(units[unit]));
		else
			return g_strdup_printf ("%i %s",
						size,
						_(units[unit]));
	}
	else if (remain)
		return g_strdup_printf ("%i.%i",
					size,
					remain);
	else
		return g_strdup_printf ("%i",
					size);
}

static std::string ToSizeString(gint64 size)
{
    gchar* c_str = brasero_utils_get_size_string(size, TRUE, TRUE);
    std::string str_res = c_str;
    g_free(c_str);

    return str_res;
}

static void UpdateDVDSize(SizeBar& sz_bar)
{
    io::pos sz     = Author::VideoSizeSum();
    io::pos typ_sz = DVDTypeSize((DVDType)sz_bar.dvdTypes.get_active_row_number());

    std::string sz_str;
    if( sz != 0 )
    {
        const int AUX_DVD_STUFF = 512*1024;
        sz += AUX_DVD_STUFF; // зарезервируем для IFO, BUP, VIDEO_TS-файлов

        sz_str = ToSizeString(sz);
        if( sz > typ_sz )
            sz_str = "<span color=\"red\">" + sz_str + "</span>";
        sz_str += " of ";
    }
    sz_bar.szLbl.set_markup(sz_str);
}

class UpdateDVDSizeVis: public ObjVisitor
{
    public:
    SizeBar& szBar;
                       UpdateDVDSizeVis(SizeBar& sz_bar): szBar(sz_bar) {}

    virtual      void  Visit(VideoMD& ) { UpdateDVDSize(szBar); }
};


//////////////////////////////////////////////////

ConstructorApp::ConstructorApp(): askSaveOnExit(true), isProjectChanged(false)
{
    using namespace boost;
    reference_wrapper<ConstructorApp> app_ref(*this);
    Add(win, vBox);
    // * область главного меню
    {
        // *
        Gtk::HBox& hbox = PackStart(vBox, NewManaged<Gtk::HBox>());
        Gtk::MenuBar& main_bar = PackStart(hbox, NewManaged<Gtk::MenuBar>());

        // Project
        Gtk::MenuItem& prj_mi = AppendMI(main_bar, NewManaged<Gtk::MenuItem>("_Project", true));
        {
            RefPtr<Gtk::ActionGroup> prj_actions = Gtk::ActionGroup::create("ProjectActions");

            prj_actions->add( Gtk::Action::create("ProjectMenu", "_Project") ); 
            prj_actions->add( Gtk::Action::create("New",    Gtk::Stock::NEW,  "_New Project"),
                              Gtk::AccelKey("<control>N"), lambda::bind(&OnNewProject, app_ref) );
            prj_actions->add( Gtk::Action::create("Open",   Gtk::Stock::OPEN, "_Open..."),
                              Gtk::AccelKey("<control>O"), lambda::bind(&OnOpenProject, app_ref) );
            prj_actions->add( Gtk::Action::create("Save",   Gtk::Stock::SAVE, "_Save"),
                              Gtk::AccelKey("<control>S"), lambda::bind(&OnSaveProject, app_ref) );
            prj_actions->add( Gtk::Action::create("SaveAs", Gtk::Stock::SAVE_AS, "Save _As..."),
                              lambda::bind(&OnSaveAsProject, app_ref) );
            prj_actions->add( Gtk::Action::create("Quit",   Gtk::Stock::QUIT, "_Quit"), 
                              Gtk::AccelKey("<control>Q"), lambda::bind(&QuitApplication, app_ref) );
            prj_actions->add( Gtk::Action::create("Import DVD", "Add Videos from _DVD", "DVD-Import Assistant"), 
                              lambda::bind(&ImportFromDVD, app_ref) );


            RefPtr<Gtk::UIManager> mngr = Gtk::UIManager::create();
            mngr->insert_action_group(prj_actions);
            win.add_accel_group(mngr->get_accel_group()); // явная привязка акселераторов к окну
    
            // popup-меню в Gtk не хотят показывать акселераторы (комбинации клавиш, которые
            // соответствуют пунктам меню), см. код Gtk, где UI-менеджер обнуляет свойство "accel-closure"
            // для GtkAccelLabel с комментарием "don't show accels in popups"
            // 
            // Поэтому вытащим меню из Menubar'а
            Glib::ustring ui_info = 
            "<ui>"
            "<menubar name='MainMenu'>"
            "  <menu action='ProjectMenu'>"
            "    <menuitem action='New'/>"
            "    <menuitem action='Open'/>"
            "    <separator/>"
            "    <menuitem action='Import DVD'/>"
            "    <separator/>"
            "    <menuitem action='Save'/>"
            "    <menuitem action='SaveAs'/>"
            "    <separator/>"
            "    <menuitem action='Quit'/>"
            "  </menu>"
            "</menubar>"
            "</ui>";
            mngr->add_ui_from_string(ui_info);

            // может не дергаться и сделать все меню через Gtk::UIManager?
            prj_mi.set_submenu(DetachMenuFromUI(mngr, "/MainMenu/ProjectMenu"));
        }

        // Go
        Gtk::MenuItem& go_mi = AppendMI(main_bar, NewManaged<Gtk::MenuItem>("_Go", true));
        go_mi.set_submenu(goMenu);
        Gtk::RadioButtonGroup grp;
        goMenu.append(CreateGoMenuItem(grp, *this)); // первый вставляем сразу
        
        // Help
        Gtk::Menu& hlp_menu     = MakeSubmenu(AppendMI(main_bar, NewManaged<Gtk::MenuItem>("_Help", true)));
        Gtk::MenuItem& about_mi = AppendMI(hlp_menu, *Gtk::manage(new Gtk::ImageMenuItem(Gtk::Stock::ABOUT)));
        about_mi.signal_activate().connect(lambda::bind(&OnDlgAbout, boost::ref(win)));

        // разведем меню и закладки на небольшое расстояние
        main_bar.show_all();
        Gtk::Requisition req = main_bar.size_request();
        main_bar.set_size_request(req.width+30, req.height);

        // *
        // Высота вкладок (по крайней мере в Clearlooks) чуть больше, чем у меню (Menubar);
        // при желании можно уменьшать высоту вкладок следующим образом:
        // - уменьшать vborder = ширина границы заголовков вкладок (но при нуле выглядит сплющенно);
        // - уменьшить с помощью стилей ythickness (у вкладок только!); с одной стороны, это влияет
        //   на ненужную нам рабочую область вкладок (в данном случае), с другой - заголовки вкладок
        //   выглядят обрезанно, некрасиво.
        PackStart(hbox, bookTabs, Gtk::PACK_EXPAND_WIDGET);
        bookTabs.set_name("MenuNotebook");
        bookTabs.set_show_border(false);

        bookTabs.property_homogeneous() = true;
        //nbook.property_tab_border()  = 5;
        bookTabs.property_tab_hborder() = 5;
        bookTabs.property_tab_vborder() = 1;

        bookTabs.signal_expose_event().connect(
            wrap_return<bool>(lambda::bind(&EraseTabLineOnExpose, boost::ref(bookTabs), lambda::_1, 
					   true, (Gtk::Notebook*)0)) );
        //bookCont.set_show_border(false);
        bookCont.signal_expose_event().connect(
            wrap_return<bool>(lambda::bind(&EraseTabLineOnExpose, boost::ref(bookCont), 
					   lambda::_1, false, &bookTabs)) );

        // * размер исходников
        Gtk::Label& lbl = PackStart(hbox, szBar.szLbl);
        {
            // установим минимум ширины для метки
            // Если не установлен явно, то на PangoLayout шрифт = нулю; правильней следует
            // брать шрифт с контекста
            Pango::FontDescription dsc = lbl.get_layout()->get_context()->get_font_description();
            double wdh, hgt;
            CalcAbsSizes(" 9999 MiB of ", dsc, wdh, hgt, GetDPI(lbl));

            lbl.set_size_request(Round(wdh), -1);
            lbl.set_alignment(1.0, 0.5);
        }

        Gtk::ComboBoxText& dvd_types = PackStart(hbox, szBar.dvdTypes);
        dvd_types.set_focus_on_click(false);
        for( int i = 0; i<dvdCNT; i++ )
        {
            std::string sz_str = ToSizeString(DVDTypeSize(DVDType(i)));
            dvd_types.append_text("DVD " + sz_str);
        }
        dvd_types.set_active(dvdFIVE);
        // обработчики
        dvd_types.signal_changed().connect( bl::bind(&UpdateDVDSize, boost::ref(szBar)));
        MHandler mh = new UpdateDVDSizeVis(szBar);
        RegisterOnInsert(mh);
        RegisterOnDelete(mh);
    }

    // * рабочая область
    PackStart(vBox, bookCont, Gtk::PACK_EXPAND_WIDGET);
    //bookCont.property_homogeneous() = true;
    //bookCont.property_tab_hborder() = 5;
    //bookCont.property_tab_pos()     = Gtk::POS_LEFT; // RIGHT;
    bookCont.property_show_tabs()   = false;

    bookCont.signal_page_added().connect(lambda::bind(&OnContentPageAdd, app_ref, lambda::_2));
    bookTabs.signal_switch_page().connect(lambda::bind(&OnMenuNotebookSwitch, app_ref, lambda::_2));

    //Gtk::PositionType pt = bookCont.get_tab_pos();
    //if( (pt == Gtk::POS_LEFT) || (pt == Gtk::POS_RIGHT) )
    //    bookCont.property_tab_hborder() = 1;
    
    win.signal_delete_event().connect( wrap_return<bool>(lambda::bind(&OnDeleteApp, app_ref)) );
    RegisterHook(lambda::bind(&OnChangeProject, app_ref));
}

void ConstructorApp::SetTabName(const std::string& name, int pos)
{
    Project::SetTabName(bookTabs, name, pos);
    GetNthGo(*this, pos)->add(NewManaged<Gtk::Label>(name, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, true));
}

ActionFunctor BuildConstructor(ConstructorApp& app, const std::string& prj_file_name)
{
    // *
    InitAStores();
    // *
    LoadApp(app, prj_file_name);
    SetAppWindowSize(app.win, 1000);
    // *
    AStores& as = GetAStores();
    RefPtr<MediaStore> md_store = as.mdStore;
    RefPtr<MenuStore>  mn_store = as.mnStore;

    // * закладки с содержимым
    Gtk::Notebook& nbook = app.BookContent();
    PackMediasWindow(nbook, md_store);
    app.SetTabName("_Source", 0);
    PackMenusWindow(nbook, mn_store, md_store);
    app.SetTabName("_Menu", 1);
    ActionFunctor after_fnr = PackOutput(app, prj_file_name);
    app.SetTabName("_Output", 2);

    return after_fnr;
}

void RunConstructor(const std::string& prj_file_name, bool ask_save_on_exit)
{
    DBCleanup db_cleanup(false);
    ConstructorApp app;
    app.askSaveOnExit = ask_save_on_exit;

    std::list<RefPtr<Gdk::Pixbuf> > pix_lst;
    static const fs::directory_iterator end_itr;
    for( fs::directory_iterator itr(fs::path(GetDataDir())/"icons");
        itr != end_itr; ++itr )
        pix_lst.push_back(Gdk::Pixbuf::create_from_file(itr->string()));
    Gtk::Window::set_default_icon_list(pix_lst);

    ActionFunctor after_fnr = BuildConstructor(app, prj_file_name);
    RunWindow(app.win);
    after_fnr();
}

} // namespace Project
