//
// mgui/project/mconstructor.cpp
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

#include "mconstructor.h"
#include "serialize.h"

#include "mb-actions.h"
#include "menu-actions.h"
#include "add.h"

#include <mgui/editor/kit.h> // new Editor::Kit()
#include <mgui/text_obj.h> // CalcAbsSizes()
#include <mgui/trackwindow.h>
#include <mgui/author/script.h> // ForeachVideo()
#include <mgui/author/output.h> // PackOutput()

#include <mgui/sdk/window.h>
#include <mgui/sdk/menu.h>

#include <mgui/prefs.h>
#include <mgui/mux.h>
#include <mgui/gettext.h>

#include <mgui/project/handler.h> // RegisterHook()

#include <mlib/sigc.h>

#include <gtk/gtkaboutdialog.h>
#include <gtk/gtklinkbutton.h> // gtk_button_set_relief

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

void GoUrl(const gchar* url)
{
    go_url_show(0, url, 0);
}

namespace Project
{

void PackMediasWindow(Gtk::Container& contr, RefPtr<MediaStore> ms, MediasWindowPacker pack_fnr)
{
    MediaBrowser& brw = NewManaged<MediaBrowser>(ms);
    pack_fnr(contr, brw);
}

void PackFullMBrowser(Gtk::Container& contr, MediaBrowser& brw)
{
    PackTrackWindow(contr, bb::bind(&PackMBWindow, _1, _2, _3, boost::ref(brw)));
}

void PackMediasWindow(Gtk::Container& contr, RefPtr<MediaStore> ms)
{
    PackMediasWindow(contr, ms, &Project::PackFullMBrowser);
}

AStores& GetAStores()
{
    return AData().GetData<AStores>();
}

static void ClearPA(PostAction& pa, MediaItem del_mi)
{
    if( pa.paLink == del_mi )
    {
        pa.paLink = 0;
        pa.paTyp  = patAUTO;
    }
}

static bool ClearEndAction(VideoItem vi, MediaItem mi)
{
    ClearPA(vi->PAction(), mi);
    return true;
}

static bool ClearEndActionM(Menu mn, MediaItem mi)
{
    ClearPA(mn->MtnData().pAct, mi);
    return true;
}

static void OnDeleteEndAction(MediaItem mi, const char* action)
{
    if( mi && (strcmp("OnDelete", action) == 0) )
    {
        ForeachVideo(bb::bind(ClearEndAction, _1, mi));
        ForeachMenu(bb::bind(ClearEndActionM, _1, mi));
    }
}

// создать списки медиа и меню
AStores& InitAStores()
{
    RefPtr<MediaStore> md_store = CreateEmptyMediaStore();
    RefPtr<MenuStore>  mn_store = CreateEmptyMenuStore();

    AStores& as = GetAStores();
    as.mdStore  = md_store;
    as.mnStore  = mn_store;

    // для ForeachVideo требуется готовый as.mdStore
    RegisterHook(&OnDeleteEndAction);
    return as;
}

Rect GetAllocation(Gtk::Widget& wdg)
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

static void OnlineHelp()
{
    //GoUrl("http://www.bombono.org/Documentation");
    GoUrl("http://www.bombono.org/Bombono_Tutorial");
}

static void OnDlgAbout()
{
    // :TODO: включить кнопку лицензии пока нельзя из ограничения окна About на размер
    //const char* license =
    //    "This program is free software; you can redistribute it and/or modify\n"
    //    "it under the terms of the GNU General Public License as published by\n"
    //    "the Free Software Foundation; either version 2 of the License, or\n"
    //    "(at your option) any later version.\n"
    //    "\n"
    //    "This program is distributed in the hope that it will be useful,\n"
    //    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    //    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    //    "GNU General Public License for more details.\n"
    //    "\n"
    //    "You should have received a copy of the GNU General Public License\n"
    //    "along with this program; if not, write to the Free Software\n"
    //    "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA";

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
        dlg->set_transient_for(Application().win);
    
        dlg->set_name(APROJECT_NAME);
        dlg->set_version(APROJECT_VERSION);
        dlg->set_copyright("Copyright \xc2\xa9 2007-2011 Ilya Murav'jov");
        //dlg->set_license(license);
        dlg->set_website("http://www.bombono.org");
        dlg->set_comments(_("Bombono DVD is a DVD authoring program with nice and clean GUI"));
        dlg->set_logo(DataDirImage("about-front.png"));
        //dlg.set_authors(authors);
        //dlg.set_documenters(documenters);
        //dlg.set_decorated(false);
        dlg->set_translator_credits(_("translator-credits"));
    
        ForAllWidgets(static_cast<Gtk::Widget&>(*dlg).gobj(), SetNoButtonRelief);
        dlg->signal_response().connect(bb::bind(OnCloseAboutDlg, dlg.get(), _1));
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
    itm.signal_toggled().connect( bb::bind(OnGoItemToggled, boost::ref(itm), boost::ref(app)) );

    return itm;
}

static void OnContentPageAdd(Gtk::Widget* , guint page_num)
{
    ConstructorApp& app = Application();
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

static void OnMenuNotebookSwitch(GtkNotebookPage* , guint page_num)
{
    ConstructorApp& app = Application();
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

static bool OnConstructorAppClose(ConstructorApp& app)
{
    return !app.askSaveOnExit || CheckBeforeClosing(app);
}

static void QuitApplication()
{
    ConstructorApp& app = Application();
    // согласно политике gtkmm окна Gtk::Window не уничтожаются; признаком
    // выхода из цикла является скрытие того окна, которое было аргументом
    // функции Gtk::Main::run() = "обработка очереди сообщений пока не скрыто окно"
    if( OnConstructorAppClose(app) )
        app.win.hide();
}

static bool OnDeleteApp(GdkEventAny* )
{
    QuitApplication();
    return true; // окно закрывает только QuitApplication()
}

static void OnChangeProject(MediaItem /*mi*/, const char* /*action*/)
{
    ConstructorApp& app = Application();
    if( !app.isProjectChanged )
    {
        app.isProjectChanged = true;
        SetAppTitle(false);
    }
}

static void ImportFromDVD()
{
    DVD::RunImport(Application().win);
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

static gchar* 
brasero_utils_get_size_string(gint64 dsize,
                              gboolean with_unit,
                              gboolean round)
{
	int unit;
	int size;
	int remain = 0;
    // раньше Brasero использовал KiB, MiB и GiB, как более точные технически
    // (вроде как размеры HDD принято измерять по 1GB=1000MB); теперь Brasero
    // перешел на стандарт KB/MB/GB, ну и BmD тоже (а че, мы не гордые :)
	const char *units[] = { "", N_("KB"), N_("MB"), N_("GB") };

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

	remain = (size % 1024) / 102.4;
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

std::string ToSizeString(io::pos size, bool round)
{
    gchar* c_str = brasero_utils_get_size_string(size, TRUE, round ? TRUE : FALSE);
    std::string str_res = c_str;
    g_free(c_str);

    return str_res;
}

io::pos DVDPayloadSize()
{
    SizeBar& sz_bar = Application().SB();
    io::pos typ_sz = DVDTypeSize((DVDType)sz_bar.dvdTypes.get_active_row_number());

    // зарезервируем для IFO, BUP, VIDEO_TS-файлов
    const int AUX_DVD_STUFF = 512*1024;
    return typ_sz - AUX_DVD_STUFF;
}

void UpdateDVDSize()
{
    io::pos sz = ProjectSizeSum();
    std::string sz_str;
    if( sz != 0 )
    {
        sz_str = ToSizeString(sz, true);
        io::pos typ_sz = DVDPayloadSize();

        if( sz > typ_sz )
            sz_str = "<span color=\"red\">" + sz_str + "</span>";
        sz_str += " of ";
    }
    Application().SB().szLbl.set_markup(sz_str);
}

class UpdateDVDSizeVis: public ObjVisitor
{
    public:

    virtual      void  Visit(VideoMD& ) { UpdateDVDSize(); }
};

void MuxAddStreams(const std::string& src_fname)
{
    std::string dest_fname;
    if( MuxStreams(dest_fname, src_fname) )
        TryAddMediaQuiet(dest_fname, "MuxAddStreams");
}

//////////////////////////////////////////////////

ConstructorApp::ConstructorApp(): askSaveOnExit(true), isProjectChanged(false)
{
    Add(win, vBox);
    // * область главного меню
    {
        // *
        Gtk::HBox& hbox = PackStart(vBox, NewManaged<Gtk::HBox>());
        Gtk::MenuBar& main_bar = PackStart(hbox, NewManaged<Gtk::MenuBar>());

        // Project
        Gtk::MenuItem& prj_mi = AppendMI(main_bar, NewManaged<Gtk::MenuItem>(_("_Project"), true));
        {
            RefPtr<Gtk::ActionGroup> prj_actions = Gtk::ActionGroup::create("ProjectActions");

            prj_actions->add( Gtk::Action::create("ProjectMenu", "_Project") );
            AddSrlActions(prj_actions);

            prj_actions->add( Gtk::Action::create("Quit",   Gtk::Stock::QUIT, _("_Quit")), 
                              Gtk::AccelKey("<control>Q"), &QuitApplication );
            prj_actions->add( Gtk::Action::create("Import DVD", DOTS_("Add Videos from _DVD"), _("DVD-Import Assistant")), 
                              &ImportFromDVD );
            prj_actions->add( Gtk::Action::create("Mux Streams", DOTS_("_Mux"), _("Mux Elementary Streams into MPEG2")),
                              bb::bind(&MuxAddStreams, std::string()) );
            prj_actions->add( Gtk::Action::create("Preferences", Gtk::Stock::PREFERENCES, DOTS_("Pr_eferences")), 
                              bb::bind(&ShowPrefs, &win) );


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
            "    <menuitem action='Mux Streams'/>"
            "    <separator/>"
            "    <menuitem action='Save'/>"
            "    <menuitem action='SaveAs'/>"
            "    <separator/>"
            "    <menuitem action='Preferences'/>"
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
        Gtk::MenuItem& go_mi = AppendMI(main_bar, NewManaged<Gtk::MenuItem>(_("_Go"), true));
        go_mi.set_submenu(goMenu);
        Gtk::RadioButtonGroup grp;
        goMenu.append(CreateGoMenuItem(grp, *this)); // первый вставляем сразу
        
        // Help
        {
            Gtk::Menu& hlp_menu = MakeSubmenu(AppendMI(main_bar, NewManaged<Gtk::MenuItem>(_("_Help"), true)));

            Gtk::Image& hlp_img   = NewManaged<Gtk::Image>(Gtk::Stock::HELP, Gtk::ICON_SIZE_MENU);
            Gtk::MenuItem& hlp_mi = AppendMI(hlp_menu, 
                                             *Gtk::manage(new Gtk::ImageMenuItem(hlp_img, _("_Online Help"), true)));
            hlp_mi.signal_activate().connect(&OnlineHelp);
            Gtk::MenuItem& about_mi = AppendMI(hlp_menu, *Gtk::manage(new Gtk::ImageMenuItem(Gtk::Stock::ABOUT)));
            about_mi.signal_activate().connect(&OnDlgAbout);
        }

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

        sig::connect(bookTabs.signal_expose_event(), 
                     bb::bind(&EraseTabLineOnExpose, boost::ref(bookTabs), _1, true, (Gtk::Notebook*)0));
        //bookCont.set_show_border(false);
        sig::connect(bookCont.signal_expose_event(),
                     bb::bind(&EraseTabLineOnExpose, boost::ref(bookCont), _1, false, &bookTabs));

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
            std::string sz_str = ToSizeString(DVDTypeSize(DVDType(i)), true);
            dvd_types.append_text("DVD " + sz_str);
        }
        dvd_types.set_active(dvdFIVE);
        // обработчики
        dvd_types.signal_changed().connect(&UpdateDVDSize);
        MHandler mh = new UpdateDVDSizeVis();
        RegisterOnInsert(mh);
        RegisterOnDelete(mh);
    }

    // * рабочая область
    PackStart(vBox, bookCont, Gtk::PACK_EXPAND_WIDGET);
    //bookCont.property_homogeneous() = true;
    //bookCont.property_tab_hborder() = 5;
    //bookCont.property_tab_pos()     = Gtk::POS_LEFT; // RIGHT;
    bookCont.property_show_tabs()   = false;

    bookCont.signal_page_added().connect(&OnContentPageAdd);
    bookTabs.signal_switch_page().connect(&OnMenuNotebookSwitch);

    //Gtk::PositionType pt = bookCont.get_tab_pos();
    //if( (pt == Gtk::POS_LEFT) || (pt == Gtk::POS_RIGHT) )
    //    bookCont.property_tab_hborder() = 1;
    
    win.signal_delete_event().connect(&OnDeleteApp);
    RegisterHook(&OnChangeProject);
}

void SetTabName(Gtk::Notebook& nbook, const std::string& name, int pos)
{
    Gtk::Label& lbl = NewBoldItalicLabel(name, true);

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

void ConstructorApp::SetTabName(const std::string& name, int pos)
{
    Project::SetTabName(bookTabs, name, pos);
    GetNthGo(*this, pos)->add(NewManaged<Gtk::Label>(name, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, true));
}

static bool UpdateAppSizes(GdkEventConfigure* event)
{
    Gtk::Window& win = Application().win;
    UnnamedPreferences& up = UnnamedPrefs();

    up.appSz.x = event->width;
    up.appSz.y = event->height;

    win.get_position(up.appPos.x, up.appPos.y);
    return false;
}

ActionFunctor BuildConstructor(ConstructorApp& app, const std::string& prj_file_name)
{
    // *
    InitAStores();
    // *
    LoadApp(prj_file_name);

    // * установка размеров и положения программы
    // не смотря на то, что документация повторяет N>5 раз (см., например, 
    // gtk_window_get_position()), что WM должен устанавливать положение (а лучше и размеры)
    // окон на экране, в реальности все по-другому (всем по^Wнас^Wвсе равно); в то время, когда
    // народ жаждет запоминания расположения программ на рабочем столе (http://brainstorm.ubuntu.com/idea/1442/),
    // разработчики Gnome перекладывают ответственность на разработчиков приложений, последним
    // угрожают,- "ничего не трогать, это забота WM" (Rhythmbox, Evince сделали вид, плохо слышат); 
    // а WM-ы ничего не могут и не хотят (без помощи от разработчиков приложений),- эпический провал (c)
    // 
    // Отписался по проблеме тут: https://bugzilla.gnome.org/show_bug.cgi?id=79285
    UnnamedPreferences& up = UnnamedPrefs();
    // используем мягкую форму вместо gtk_window_resize(), чтобы 
    // не уродовали наш продукт (два раза ку) уменьшением размеров до нуля
    app.win.set_default_size(up.appSz.x, up.appSz.y);
    if( up.isLoaded )
        app.win.move(up.appPos.x, up.appPos.y);
    app.win.signal_configure_event().connect(&UpdateAppSizes, false);
    
    // *
    AStores& as = GetAStores();
    RefPtr<MediaStore> md_store = as.mdStore;
    RefPtr<MenuStore>  mn_store = as.mnStore;

    // * закладки с содержимым
    Gtk::Notebook& nbook = app.BookContent();
    PackMediasWindow(nbook, md_store);
    app.SetTabName(C_("MainTabs", "_Source"), 0);
    PackMenusWindow(nbook, mn_store, md_store);
    app.SetTabName(C_("MainTabs", "_Menu"), 1);
    ActionFunctor after_fnr = PackOutput(app, prj_file_name);
    app.SetTabName(C_("MainTabs", "_Output"), 2);

    return after_fnr;
}

void RunConstructor(const std::string& prj_file_name, bool ask_save_on_exit)
{
    DBCleanup db_cleanup(false);
    // *
    InitI18n();
    // *
    LoadPrefs();
    AData().SetPalTvSystem(Prefs().isPAL);

    {
        SingletonStack<ConstructorApp> app_ssi;
        SingletonStack<MEditorArea> edt_ssi;
        // *
        ConstructorApp& app = Application();
        app.askSaveOnExit = ask_save_on_exit;
    
        std::list<RefPtr<Gdk::Pixbuf> > pix_lst;
        static const fs::directory_iterator end_itr;
        for( fs::directory_iterator itr(DataDirPath("icons"));
            itr != end_itr; ++itr )
            pix_lst.push_back(Gdk::Pixbuf::create_from_file(itr->string()));
        Gtk::Window::set_default_icon_list(pix_lst);
    
        ActionFunctor after_fnr = BuildConstructor(app, prj_file_name);
        RunWindow(app.win);
        after_fnr();
    }
    // сохраняем настройки после закрытия (=> обновления информации) окон
    SaveUnnamedPrefs();
}

} // namespace Project

void InitI18n()
{
    static bool is_init = false;
    if( !is_init )
    {
        is_init = true;
    
        setlocale(LC_ALL, "");
        const char* prefix = GetInstallPrefix();
        std::string locale_prefix(prefix ? (fs::path(prefix) / "share" / "locale").string() : "build/po/locale");
        bindtextdomain("bombono-dvd", locale_prefix.c_str());
        // результат gettext() должен быть в UTF-8, а не в пользовательской локали
        // (проблемы с не UTF8-локалями вроде ru_RU.KOI8-R)
        bind_textdomain_codeset("bombono-dvd", "UTF-8");
        textdomain("bombono-dvd");
    }
}

