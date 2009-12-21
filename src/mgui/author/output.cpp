//
// mgui/author/output.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009 Ilya Murav'jov
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

#include "output.h"
#include "script.h"
#include "burn.h"

#include <mgui/dialog.h>
#include <mgui/win_utils.h>
#include <mgui/img-factory.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/widget.h>
#include <mgui/timer.h>

#include <mbase/resources.h>
#include <mlib/filesystem.h>

#include <gtk/gtkdialog.h>
#include <gtk/gtkmessagedialog.h>

#include <boost/filesystem/convenience.hpp> // fs::create_directories()

static Gtk::Alignment& MakeNullAlg()
{
    return NewManaged<Gtk::Alignment>(0.0, 0.0, 0.0, 0.0);
}

namespace Project 
{

const std::string DVDOperation = "DVD-Video Building";

Gtk::Button& FillBuildButton(Gtk::Button& btn, bool not_started, 
                             const std::string& op_name)
{
    if( Gtk::Widget* wdg = btn.get_child() )
        delete wdg; // = gtk_widget_destroy()

    const char* pix_fname = "button/still.ico";
    const char* tooltip   = not_started ? "Build DVD-Video of the project." : 0 ;
    std::string cancel_name = "_Cancel " + op_name;
    std::string label     = not_started ? std::string("_Build DVD-Video") : cancel_name.c_str() ;

    ASSERT( btn.has_screen() ); // ради image-spacing
    int image_spacing = 0;
    btn.get_style_property("image-spacing", image_spacing);
    Gtk::HBox& box = NewManaged<Gtk::HBox>(false, image_spacing);
    btn.add(box);

    PackStart(box, NewManaged<Gtk::Image>(GetFactoryImage(pix_fname))).set_name("BurnImage");

    Gtk::Label& lbl = NewManaged<Gtk::Label>("<span weight=\"bold\" size=\"large\">" + label + "</span>");
    lbl.set_use_markup(true);
    lbl.set_use_underline(true);
    box.pack_start(lbl, false, false);
    lbl.set_mnemonic_widget(btn);

    SetTip(btn, tooltip);
    // неуместно выглядит фокус на кнопке после нажатия
    btn.set_focus_on_click(false);
    btn.show_all_children();
    return btn;
}

static void SetAuthorMode(Author::Mode mode, Gtk::RadioButton& btn)
{
    if( btn.get_active() )
        Author::GetES().mode = mode;
}

static Gtk::RadioButton& AddAuthoringMode(Gtk::Box& box, Gtk::RadioButtonGroup& grp, 
                                          const char* name, Author::Mode mode, bool init_this = false)
{
    Gtk::RadioButton& btn = NewManaged<Gtk::RadioButton>(grp, name);
    btn.set_use_underline(true);
    btn.signal_toggled().connect(boost::lambda::bind(&SetAuthorMode, mode, boost::ref(btn)));
    PackStart(box, btn);

    // ручками устанавливаем режим вначале
    if( init_this )
    {
        ASSERT( btn.get_active() );
        SetAuthorMode(mode, btn);
    }
    return btn;
}

Gtk::Label& FillAuthorLabel(Gtk::Label& lbl, const std::string& name, bool is_left)
{
    lbl.set_text(name); //"<span size=\"large\">" + name + "</span>");
    SetAlign(lbl, is_left);
    //lbl.set_use_markup(true);
    lbl.set_use_underline(true);

    return lbl;
}

void PackProgressBar(Gtk::VBox& vbox, Author::ExecState& es)
{
    PackStart(vbox, es.prgLabel);
    PackStart(vbox, es.prgBar);
}

static Gtk::Alignment& MakeSubOptionsAlg()
{
    Gtk::Alignment& alg = NewManaged<Gtk::Alignment>(0.5, 0.5, 1.0, 1.0);
    alg.set_padding(0, 0, 30, 0);
    return alg;
}

static Gtk::Label& MakeAuthorLabel(const std::string& name, bool is_left)
{
    return FillAuthorLabel(NewManaged<Gtk::Label>(), name, is_left);
}

ActionFunctor PackOutput(ConstructorApp& app, const std::string& prj_fname)
{
    Gtk::Notebook& nbook = app.BookContent();
    Gtk::Alignment& alg = Add(nbook, NewManaged<Gtk::Alignment>(0.05, 0.05, 0.5, 0.5));

    Author::ExecState& es = Author::GetInitedES();
    {
        Gtk::VBox& vbox = Add(alg, NewManaged<Gtk::VBox>(false, 5));

        // * куда
        // :TODO: хотелось бы видеть полный путь до папки
        // Реализовать на основе Gtk::Button, с внешним видом как у ComboBox:
        //  - gtk_icon_theme_load_icon (theme, "gnome-fs-regular", priv->icon_size, 0, NULL);
        //  - gtk_icon_theme_load_icon (icon_theme, "gnome-fs-directory", button->priv->icon_size, 0, NULL);
        //  - gtk_vseparator_new (), gtk_toggle_button_new (), gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE)
        //  
        // Или варианты:
        // - взять код gtkfilechooserbutton.c и изменить упаковку комбо-бокса, см. gtk_file_chooser_button_init(),
        //   где добавление атрибутов (более простой способ - удалить все атрибуты и набрать самому, но до номеров
        //   столбцов не достучаться)
        // - попробовать libsexy/libview - может там чего есть
        Gtk::FileChooserButton& ch_btn = NewManaged<Gtk::FileChooserButton>("Select output folder", 
            Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
        {
            Gtk::VBox& box = PackStart(vbox, NewManaged<Gtk::VBox>());

            Gtk::Label& lbl = PackStart(box, MakeAuthorLabel("Select Output _Folder:", true));
            lbl.set_mnemonic_widget(ch_btn);
            // по умолчанию будет директория проекта
            if( !prj_fname.empty() )
                ch_btn.set_filename(prj_fname);
            PackStart(box, ch_btn);
        }

        // * выбор режима
        {
            // внешний нужен чтобы рамка не распахивалась, внутренний - для отступа с боков
            Gtk::Alignment& out_alg = PackStart(vbox, MakeNullAlg());
            Gtk::Alignment& alg = MakeNullAlg();
            alg.set_padding(0, 0, 5, 5);
            out_alg.add(PackWidgetInFrame(alg, Gtk::SHADOW_ETCHED_IN, " Choose Author Mode: "));

            Gtk::VBox& mode_box = Add(alg, NewManaged<Gtk::VBox>(false, 2));

            Gtk::RadioButtonGroup grp;
            // цель по умолчанию
            AddAuthoringMode(mode_box, grp, "_Write DVD Folder", Author::modFOLDER, true);
            AddAuthoringMode(mode_box, grp, "Write Disk _Image", Author::modDISK_IMAGE);

            RefPtr<Gtk::SizeGroup> labels_sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
            RefPtr<Gtk::SizeGroup> wdg_sg    = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
            Author::BurnData& bd = Author::GetInitedBD();
            {
                Gtk::Alignment& alg = PackStart(mode_box, MakeSubOptionsAlg());

                Gtk::HBox& box = Add(alg, NewManaged<Gtk::HBox>(false));
                labels_sg->add_widget(PackStart(box, MakeAuthorLabel("Disc Label: ", false)));
                Gtk::Entry& ent = PackStart(box, bd.Label());
                wdg_sg->add_widget(ent);
            }

            Gtk::RadioButton& burn_rb = AddAuthoringMode(mode_box, grp, "Burn to _DVD", Author::modBURN);
            {
                Gtk::Alignment& alg = PackStart(mode_box, MakeSubOptionsAlg());
                Gtk::Table& tbl = Add(alg, NewManaged<Gtk::Table>(2, 2, false));

                Gtk::ComboBoxText& dvd_btn = bd.DVDDevices();
                Gtk::ComboBox& speed_btn   = bd.SpeedBtn();
    
                Gtk::Label& drv_lbl = MakeAuthorLabel("DVD Drive: ", false);
                labels_sg->add_widget(drv_lbl);
                tbl.attach(drv_lbl, 0, 1, 0, 1, Gtk::SHRINK);
                wdg_sg->add_widget(dvd_btn);
                tbl.attach(dvd_btn, 1, 2, 0, 1, Gtk::SHRINK);

                Gtk::Label& speed_lbl = MakeAuthorLabel("Writing Speed: ", false);
                labels_sg->add_widget(speed_lbl);
                tbl.attach(speed_lbl, 0, 1, 1, 2, Gtk::SHRINK);
                wdg_sg->add_widget(speed_btn);
                tbl.attach(speed_btn, 1, 2, 1, 2, Gtk::SHRINK);
    
                if( !dvd_btn.get_model()->children().size() )
                    burn_rb.set_sensitive(false);
            }

            PackHSeparator(mode_box);
            AddAuthoringMode(mode_box, grp, "_Rendering only (for experts)", Author::modRENDERING);
        }

        // *
        {
            Gtk::VBox& box = PackStart(vbox, NewManaged<Gtk::VBox>());
            PackProgressBar(box, es);

            Gtk::Expander& expdr = PackStart(box, NewManaged<Gtk::Expander>("Show/_Hide Details", true));
            Gtk::TextView& txt_view = es.detailsView;
            txt_view.set_size_request(0, 200);
            expdr.add(Author::PackDetails(txt_view));
        }

        Gtk::Button& build_btn = Add(PackStart(vbox, MakeNullAlg()), es.ExecButton());
        FillBuildButton(build_btn, true, DVDOperation);

        using namespace boost;
        build_btn.signal_clicked().connect( lambda::bind(&Author::OnDVDBuild, boost::ref(ch_btn)) );
    }

    // :BUG: связка GtkComboBox(включая производные классы, содержащие его - GtkFileChooserButton) в GtkNotebook
    // дает segfault, если:
    // - оставили первый в фокусе и затем перешли в другую вкладку;
    // - тогда при выходе из приложения:
    //   - закладки будут удаляться последовательно, с переходом фокуса на следующие;
    //   - сам фокус опять перейдет к кнопке (GtkToggleButton) компобокса;
    //   - при удалении GtkComboBox в самом конце (finalize) вызывается gtk_widget_unparent() для его кнопки,
    //     что повлечет вызов gtk_container_set_focus_child(combo, NULL) опять на комбо, что делать
    //     нельзя, потому что тот уже не дееспособен => segfault
    // Выход: заранее скрываем закладку с проблемным GtkComboBox (альтернатива - сделать комбобокс невыбираемым)
    //
    //nbook.get_nth_page(nbook.get_n_pages()-1)->hide();
    return bl::bind(&Gtk::Widget::hide, nbook.get_nth_page(nbook.get_n_pages()-1));
}

} // namespace Project

namespace Author
{

static void SetExecState(ExecState& es, bool is_exec, const std::string& operation)
{
    es.Set(is_exec);
    Project::FillBuildButton(es.ExecButton(), !is_exec, operation);
    if( is_exec )
    {
        es.Clean();
        es.operationName = operation;
    }
}

void SetExecState(ExecState& es, bool is_exec)
{
    std::string op = (es.mode == modRENDERING) ? std::string("Rendering") : Project::DVDOperation;
    SetExecState(es, is_exec, op);
    if( is_exec )
        InitStageMap(es.mode);
}

static void FindBurnImage(GtkWidget* wdg, Gtk::Image** img_wdg)
{
    if( strcmp(gtk_widget_get_name(wdg), "BurnImage") == 0 )
        *img_wdg = Glib::wrap((GtkImage*)wdg);
}

struct DiscSpinning
{
    DiscSpinning(ExecState& es): imgWdg(0), idx(0)
    {
        for( int i=0; i<8; i++ )
        {
            std::string img_path = AppendPath(GetDataDir(), (str::stream("button/00") << i+1 << ".ico").str());
            imgList.push_back(Gdk::Pixbuf::create_from_file(img_path));
        }
         
        ForAllWidgets(static_cast<Gtk::Widget&>(es.ExecButton()).gobj(), bl::bind(&FindBurnImage, bl::_1, &imgWdg));
        ASSERT( imgWdg );

        tm.Connect(bl::bind(&DiscSpinning::Turn, this), 150); 
    }
   ~DiscSpinning() 
    {   
        tm.Disconnect(); 
    }

    bool Turn()
    {
        if( imgWdg )
        {
            imgWdg->set(imgList[idx]);
            idx = (idx+1) % imgList.size();
        }
        return true;
    }

    protected:
                            Gtk::Image* imgWdg;
                                 Timer  tm;

                                   int  idx;
      std::vector<RefPtr<Gdk::Pixbuf> > imgList;
};

class CommonStateSetter
{
    public:
            CommonStateSetter(ExecState& es_): es(es_) {}

      void  StartSpinning() 
            { 
                // создание обязательно после SetXXXState()!
                ds = new DiscSpinning(es);
            }
    protected:
                ExecState& es;
    ptr::one<DiscSpinning> ds;
};

class ExecStateSetter: public CommonStateSetter
{
    public:
        ExecStateSetter(ExecState& es_): CommonStateSetter(es_)
        { 
            SetExecState(es, true);
            StartSpinning();
        }
       ~ExecStateSetter()
        { SetExecState(es, false); }
};

const std::string BurnOperation = "DVD Burning";

static void SetBurningState(ExecState& es, bool is_exec)
{
    SetExecState(es, is_exec, BurnOperation);
}

class BurningStateSetter: public CommonStateSetter
{
    public:
        BurningStateSetter(ExecState& es_): CommonStateSetter(es_)
        { 
            SetBurningState(es, true);
            StartSpinning();
        }
       ~BurningStateSetter()
        { SetBurningState(es, false); }
};

fs::path TargetPath(Mode mode, const std::string& dir)
{
    std::string res;
    switch( mode )
    {
    case modFOLDER:     
        res = "dvd";
        break;
    case modDISK_IMAGE: 
        res = "dvd.iso";
        break;
    case modRENDERING:       
        res = "";
        break;
    default:
        ASSERT_RTL(0);
    }
    return fs::path(dir)/res;
}

static std::string MakeDescForOutput(Mode mode, const std::string& dir)
{
    str::stream dsc_strm;
    if( mode != modBURN )
    {
        dsc_strm << "The result is here: <span style=\"italic\">" << TargetPath(mode, dir).string() << "</span>.";
        if( mode == modRENDERING )
            dsc_strm << "\n\nYou can run authoring manually by executing command \"scons\" at " 
                        "the specified folder. Also, see README file for other options over there.";
    }
    return dsc_strm.str();
}

static void SetFinalStatus(const std::string& status, bool is_ok)
{
    ExecState& es = GetES();
    es.SetIndicator(is_ok ? 100. : 0.);
    es.SetStatus(status);
}

static void FinalMessageBox(const std::string& status, bool is_ok, Gtk::MessageType typ,
                            const std::string& desc_str)
{
    SetFinalStatus(status, is_ok);
    MessageBox(status, typ, Gtk::BUTTONS_OK, desc_str);
}

static bool CanUseForAuthoring(const std::string& dir_str)
{
    bool res = true;
    std::string abort_str(". Authoring is cancelled.");
    if( !fs::exists(dir_str) )
    {
        try { fs::create_directories(dir_str); } 
        catch( const std::exception& )
        {
            MessageBox("Cant create directory " + dir_str + " (check permissions)" + abort_str,
                       Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            res = false;
        }
    }
    else if( !fs::is_directory(dir_str) )
    {
        MessageBox(dir_str + " is not a directory" + abort_str,
                   Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        res = false;
    }
    if( !res )
        return false;

    fs::path dir_path(dir_str);
    ASSERT( fs::is_directory(dir_path) );
    if( !Project::HaveFullAccess(dir_path) )
    {
        MessageBox("Cant have full access to directory " + dir_str + " (read, write)" + abort_str,
                   Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        res = false;
    }
    else if( !fs::is_empty_directory(dir_path) )
    {
        bool is_empty = false;
        if( Gtk::RESPONSE_YES == 
            MessageBox(dir_str + " is not empty. We need to remove all files in it for authoring process.\n"
                       "Continue?", Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO) )
        {
            std::string err_str;
            is_empty = Project::ClearAllFiles(dir_path, err_str);
            if( !is_empty )
                MessageBox("Error during removing files: " + err_str + abort_str,
                           Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        }
        res = is_empty;
    }
    return res;
}

///////////////////
// Run()

// COPY_N_PASTE_ETALON gtkdialog.c
struct RunInfo
{
    GtkDialog* dialog;
         gint  response_id;
    GMainLoop* loop;
     gboolean  destroyed;

     // если вернет true, то выйдем из диалога
     typedef boost::function<bool(gint)> ResponseFnr;
  ResponseFnr  fnr;
};

static void
shutdown_loop (RunInfo *ri)
{
    if (g_main_loop_is_running (ri->loop))
        g_main_loop_quit (ri->loop);
}

static void
run_unmap_handler (GtkDialog* , gpointer data)
{
    RunInfo *ri = (RunInfo*)data;
    shutdown_loop (ri);
}

static void
run_response_handler (GtkDialog* ,
                      gint response_id,
                      gpointer data)
{
    RunInfo *ri = (RunInfo*)data;
    bool is_shutdown = !ri->fnr || ri->fnr(response_id);

    if( is_shutdown )
    {
        ri->response_id = response_id;
        shutdown_loop (ri);
    }
}

static gint
run_delete_handler (GtkDialog* ,
                    GdkEventAny* ,
                    gpointer data)
{
    RunInfo *ri = (RunInfo*)data;
    shutdown_loop (ri);

    return TRUE; /* Do not destroy */
}

static void
run_destroy_handler (GtkDialog* , gpointer data)
{
    RunInfo *ri = (RunInfo*)data;
    /* shutdown_loop will be called by run_unmap_handler */
    ri->destroyed = TRUE;
}

static Gtk::ResponseType Run(Gtk::Dialog& dlg, bool run_modal, 
                             RunInfo::ResponseFnr fnr = RunInfo::ResponseFnr())
{
    GtkDialog* dialog = dlg.gobj();

    RunInfo ri = { NULL, GTK_RESPONSE_NONE, NULL, FALSE, fnr };
    gboolean was_modal;
    gulong response_handler;
    gulong unmap_handler;
    gulong destroy_handler;
    gulong delete_handler;

    //g_return_val_if_fail (GTK_IS_DIALOG (dialog), -1);

    g_object_ref (dialog);

    was_modal = GTK_WINDOW (dialog)->modal;
    //if (!was_modal)
    if (run_modal)
        gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

    if (!GTK_WIDGET_VISIBLE (dialog))
        gtk_widget_show (GTK_WIDGET (dialog));

    response_handler =
    g_signal_connect (dialog,
                      "response",
                      G_CALLBACK (run_response_handler),
                      &ri);

    unmap_handler =
    g_signal_connect (dialog,
                      "unmap",
                      G_CALLBACK (run_unmap_handler),
                      &ri);

    delete_handler =
    g_signal_connect (dialog,
                      "delete-event",
                      G_CALLBACK (run_delete_handler),
                      &ri);

    destroy_handler =
    g_signal_connect (dialog,
                      "destroy",
                      G_CALLBACK (run_destroy_handler),
                      &ri);

    ri.loop = g_main_loop_new (NULL, FALSE);

    GDK_THREADS_LEAVE ();  
    g_main_loop_run (ri.loop);
    GDK_THREADS_ENTER ();  

    g_main_loop_unref (ri.loop);

    ri.loop = NULL;

    if (!ri.destroyed)
    {
        if (!was_modal)
            gtk_window_set_modal (GTK_WINDOW(dialog), FALSE);

        g_signal_handler_disconnect (dialog, response_handler);
        g_signal_handler_disconnect (dialog, unmap_handler);
        g_signal_handler_disconnect (dialog, delete_handler);
        g_signal_handler_disconnect (dialog, destroy_handler);
    }

    g_object_unref (dialog);

    return(Gtk::ResponseType)ri.response_id;
}

///////////////////

const guint RESPONSE_TOTEM = 0;
const guint RESPONSE_BURN  = 1;

bool NotForPlay(guint response_id, const std::string& dir_str)
{
    bool res = true; 
    if( response_id == RESPONSE_TOTEM )
    {
        res = false;
        Spawn(dir_str.c_str(), "scons totem");
    }
    return res;
}

static void FailureMessageBox(ExecState& es, const std::string& reason)
{
    if( es.userAbort )
        FinalMessageBox(es.operationName + " cancelled.", false, Gtk::MESSAGE_INFO, "");
    else
        FinalMessageBox(es.operationName + " broken.", false, Gtk::MESSAGE_ERROR,
                        "The reason is \"" + reason + "\" (see Details)");
}

static std::string OperationCompleted(ExecState& es)
{
    return es.operationName + " successfully completed.";
}

bool CheckDVDBlank();
static bool CheckDVDBlankForBurning()
{
    bool res = true;
    if( GetES().mode == modBURN )
        res = CheckDVDBlank();
    return res;
}

static void PostBuildOperation(bool res, const std::string& dir_str)
{
    ExecState& es = GetES();
    if( res )
    {
        std::string status = OperationCompleted(es);
        std::string desc_str = MakeDescForOutput(es.mode, dir_str);
        if( es.mode == modRENDERING )
        {
            FinalMessageBox(status, true, Gtk::MESSAGE_INFO, desc_str);
        }
        else
        {
            SetFinalStatus(status, true);
            Gtk::MessageDialog dlg(MakeMessageBoxTitle(status), true, 
                                   Gtk::MESSAGE_INFO, Gtk::BUTTONS_NONE);
            dlg.set_secondary_text(desc_str, true);

            dlg.add_button("_Play in Totem", RESPONSE_TOTEM);
            dlg.add_button("_Burn to DVD",   RESPONSE_BURN);
            std::string dvd_drive;
            if( /*(es.mode == modBURN) ||*/ !Author::IsBurnerSetup(dvd_drive) )
                dlg.set_response_sensitive(RESPONSE_BURN, false);
            dlg.add_button(Gtk::Stock::OK,   Gtk::RESPONSE_OK);
            dlg.set_default_response(Gtk::RESPONSE_OK);

            using namespace boost;
            while( Run(dlg, false, lambda::bind(&NotForPlay, lambda::_1, dir_str)) == RESPONSE_BURN )
            {
                dlg.hide();
                if( !CheckDVDBlank() )
                    break;

                // * прожиг
                str::stream scons_options;
                FillSconsOptions(scons_options, false);
                ExitData ed;
                {
                    BurningStateSetter bss(es);

                    ConsoleOF of;
                    ed = ExecuteSconsCmd(dir_str, of, modBURN, scons_options);
                }
                if( ed.IsGood() )
                {
                    SetFinalStatus(OperationCompleted(es), true);
                }
                else
                {    
                    FailureMessageBox(es, ExitDescription(ed));
                    break;
                }

                // * заново
                dlg.set_markup(MakeMessageBoxTitle(OperationCompleted(es)));
                gtk_message_dialog_format_secondary_text(dlg.gobj(), 0); // убираем текст
            }
        }
    }
    else
        FailureMessageBox(es, es.exitDesc);
}

void OnDVDBuild(Gtk::FileChooserButton& ch_btn)
{
    ExecState& es = GetES();
    if( !es.isExec )
    {
        std::string dir_str = ch_btn.get_filename();

        if( CanUseForAuthoring(dir_str) && CheckDVDBlankForBurning() )
        {
            bool res = true;
            {
                ExecStateSetter ess(es);
                res = Project::AuthorDVD(dir_str);
            }
            PostBuildOperation(res, dir_str);
        }
    }
    else
    {
        // COPY_N_PASTE - тупо сделал содержимое сообщений как у "TSNAMI-MPEG DVD Author"
        // А что делать - нафига свои придумывать, если смысл один и тот же
        if( Gtk::RESPONSE_YES == MessageBox("You are about to cancel " + es.operationName + 
                                            ". Are you sure?", Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO) )
        {
            es.userAbort = true;
            if( es.pid != NO_HNDL ) // во время выполнения внешней команды
                StopExecution(es.pid);
        }
    }
}

} // namespace Author
