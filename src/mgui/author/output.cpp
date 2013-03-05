//
// mgui/author/output.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009-2010 Ilya Murav'jov
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

#include <mgui/project/handler.h>
#include <mgui/project/mb-actions.h> // DVDPayloadSize()
#include <mgui/dialog.h>
#include <mgui/win_utils.h>
#include <mgui/img-factory.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/widget.h>
#include <mgui/sdk/textview.h>
#include <mgui/timer.h>
#include <mgui/gettext.h>
#include <mgui/prefs.h>
#include <mgui/text_obj.h> // TextObj
#include <mgui/render/menu.h> // GetMenuRegion()

#include <mbase/resources.h>
#include <mlib/filesystem.h>

#include <gtk/gtkdialog.h>
#include <gtk/gtkmessagedialog.h>

static Gtk::Alignment& MakeNullAlg()
{
    return NewManaged<Gtk::Alignment>(0.0, 0.0, 0.0, 0.0);
}

namespace Project 
{

const std::string DVDOperation = N_("DVD-Video Building");

Gtk::Button& FillBuildButton(Gtk::Button& btn, bool not_started, 
                             const std::string& op_name)
{
    if( Gtk::Widget* wdg = btn.get_child() )
        delete wdg; // = gtk_widget_destroy()

    const char* pix_fname = "button/still.ico";
    const char* tooltip   = not_started ? "Build DVD-Video of the project." : 0 ;
    std::string cancel_name = BF_("_Cancel %1%") % op_name % bf::stop;
    std::string label     = not_started ? std::string(_("_Build DVD-Video")) : cancel_name.c_str() ;

    ASSERT( btn.has_screen() ); // ради image-spacing
    int image_spacing = 0;
    btn.get_style_property("image-spacing", image_spacing);
    Gtk::HBox& box = NewManaged<Gtk::HBox>(false, image_spacing);
    btn.add(box);

    PackStart(box, NewManaged<Gtk::Image>(GetFactoryImage(pix_fname))).set_name("BurnImage");

    Gtk::Label& lbl = NewMarkupLabel("<span weight=\"bold\" size=\"large\">" + label + "</span>", true);
    box.pack_start(lbl, false, false);
    lbl.set_mnemonic_widget(btn);

    SetTip(btn, tooltip);
    // неуместно выглядит фокус на кнопке после нажатия
    btn.set_focus_on_click(false);
    btn.show_all_children();
    return btn;
}

static void SetAuthorMode(Author::Mode mode)
{
    Author::GetES().mode = mode;
}

static Gtk::RadioButton& AddAuthoringMode(Gtk::Box& box, Gtk::RadioButtonGroup& grp, 
                                          const char* name, Author::Mode mode, bool init_this = false)
{
    Gtk::RadioButton& btn = NewManaged<Gtk::RadioButton>(grp, name);
    btn.set_use_underline(true);
    SetForRadioToggle(btn, bb::bind(&SetAuthorMode, mode));
    PackStart(box, btn);

    // ручками устанавливаем режим вначале
    if( init_this )
    {
        ASSERT( btn.get_active() );
        SetAuthorMode(mode);
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

static Gtk::Label& MakeAuthorLabel(const std::string& name, bool is_above)
{
    return FillAuthorLabel(NewManaged<Gtk::Label>(), name + ": ", is_above);
}

ActionFunctor PackOutput(ConstructorApp& app, const std::string& /*prj_fname*/)
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
        const char* ch_msg = _("Select output _folder");
        Gtk::FileChooserButton& ch_btn = NewManaged<Gtk::FileChooserButton>(RMU_(ch_msg), 
            Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
        {
            Gtk::VBox& box = PackStart(vbox, NewManaged<Gtk::VBox>());

            Gtk::Label& lbl = PackStart(box, MakeAuthorLabel(ch_msg, true));
            lbl.set_mnemonic_widget(ch_btn);
            // берем путь из настроек
            //if( !prj_fname.empty() )
            //    ch_btn.set_filename(prj_fname);
            TryDefaultAuthorPath(ch_btn);

            PackStart(box, ch_btn);
        }

        // * выбор режима
        {
            // внешний нужен чтобы рамка не распахивалась, внутренний - для отступа с боков
            Gtk::Alignment& out_alg = PackStart(vbox, MakeNullAlg());
            Gtk::Alignment& alg = MakeNullAlg();
            alg.set_padding(0, 0, 5, 5);
            std::string frame_name = boost::format(" %1%: ") % _("Choose author mode") % bf::stop;
            out_alg.add(PackWidgetInFrame(alg, Gtk::SHADOW_ETCHED_IN, frame_name));

            Gtk::VBox& mode_box = Add(alg, NewManaged<Gtk::VBox>(false, 2));

            Gtk::RadioButtonGroup grp;
            // цель по умолчанию
            AddAuthoringMode(mode_box, grp, _("_Write DVD folder"), Author::modFOLDER, true);
            AddAuthoringMode(mode_box, grp, _("Write disk _image"), Author::modDISK_IMAGE);

            RefPtr<Gtk::SizeGroup> labels_sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
            RefPtr<Gtk::SizeGroup> wdg_sg    = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
            Author::BurnData& bd = Author::GetInitedBD();
            {
                Gtk::Alignment& alg = PackStart(mode_box, MakeSubOptionsAlg());

                Gtk::HBox& box = Add(alg, NewManaged<Gtk::HBox>(false));
                labels_sg->add_widget(PackStart(box, MakeAuthorLabel(_("Disc label"), false)));
                Gtk::Entry& ent = PackStart(box, bd.Label());
                wdg_sg->add_widget(ent);
            }

            Gtk::RadioButton& burn_rb = AddAuthoringMode(mode_box, grp, _("Burn to _DVD"), Author::modBURN);
            {
                Gtk::Alignment& alg = PackStart(mode_box, MakeSubOptionsAlg());
                Gtk::Table& tbl = Add(alg, NewManaged<Gtk::Table>(2, 2, false));

                Gtk::ComboBoxText& dvd_btn = bd.DVDDevices();
                Gtk::ComboBox& speed_btn   = bd.SpeedBtn();
    
                Gtk::Label& drv_lbl = MakeAuthorLabel(_("DVD drive"), false);
                labels_sg->add_widget(drv_lbl);
                tbl.attach(drv_lbl, 0, 1, 0, 1, Gtk::SHRINK);
                wdg_sg->add_widget(dvd_btn);
                tbl.attach(dvd_btn, 1, 2, 0, 1, Gtk::SHRINK);

                Gtk::Label& speed_lbl = MakeAuthorLabel(_("Writing speed"), false);
                labels_sg->add_widget(speed_lbl);
                tbl.attach(speed_lbl, 0, 1, 1, 2, Gtk::SHRINK);
                wdg_sg->add_widget(speed_btn);
                tbl.attach(speed_btn, 1, 2, 1, 2, Gtk::SHRINK);
    
                if( !dvd_btn.get_model()->children().size() )
                    burn_rb.set_sensitive(false);
            }

            PackHSeparator(mode_box);
            AddAuthoringMode(mode_box, grp, _("_Rendering only"), Author::modRENDERING);
        }

        // *
        {
            Gtk::VBox& box = PackStart(vbox, NewManaged<Gtk::VBox>());
            PackProgressBar(box, es);

            Gtk::Expander& expdr = PackStart(box, NewManaged<Gtk::Expander>(_("Show/_hide Details"), true));
            Gtk::TextView& txt_view = es.detailsView;
            txt_view.set_size_request(0, 200);
            expdr.add(PackDetails(txt_view));
        }

        Gtk::Button& build_btn = Add(PackStart(vbox, MakeNullAlg()), es.ExecButton());
        FillBuildButton(build_btn, true, DVDOperation);

        build_btn.signal_clicked().connect( bb::bind(&Author::OnDVDBuild, boost::ref(ch_btn)) );
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
    return bb::bind(&Gtk::Widget::hide, nbook.get_nth_page(nbook.get_n_pages()-1));
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
    std::string op = (es.mode == modRENDERING) ? _("Rendering") : gettext(Project::DVDOperation.c_str());
    SetExecState(es, is_exec, op);
    if( is_exec )
    {
        Project::SizeStat ss = Project::ProjectStat();
        double trans_ratio = 0.;
        if( PrjSum(ss) )
            trans_ratio = ss.transSum / (double)PrjSum(ss);
        InitStageMap(es.mode, trans_ratio);
    }
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
            //RefPtr<Gdk::Pixbuf> img = DataDirImage((str::stream("button/00") << i+1 << ".ico").str().c_str());
            std::string fname = boost::format("button/00%1%.ico") % (i+1) % bf::stop;
            imgList.push_back(DataDirImage(fname.c_str()));
        }
         
        ForAllWidgets(static_cast<Gtk::Widget&>(es.ExecButton()).gobj(), bb::bind(&FindBurnImage, _1, &imgWdg));
        ASSERT( imgWdg );

        tm.Connect(bb::bind(&DiscSpinning::Turn, this), 150); 
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

const char* BurnOperation = N_("DVD Burning");

static void SetBurningState(ExecState& es, bool is_exec)
{
    SetExecState(es, is_exec, gettext(BurnOperation));
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
        dsc_strm << _("The result is here") << ": <span style=\"italic\">" << TargetPath(mode, dir).string() << "</span>.";
        if( mode == modRENDERING )
            dsc_strm << "\n\n" << _("You can run authoring manually by executing command \"scons\" at " 
                        "the specified folder. Also, see README file for other options over there.");
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
    // Translators: impossible to go on!
    std::string abort_str(_("Authoring is cancelled."));
    abort_str = ". " + abort_str;

    std::string err_str;
    if( !CreateDirs(dir_str, err_str) )
    {
        MessageBox(err_str, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        res = false;
    }
    else if( !fs::is_directory(dir_str) )
    {
        MessageBox(BF_("%1% is not a folder") % dir_str % bf::stop + abort_str,
                   Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        res = false;
    }
    else if( dir_str == g_get_home_dir() )
    {
        // :TRICKY: GtkFileChooser часто в случае проблем (собственных или чужих), 
        // да и по умолчанию, указывает на домашнюю директорию, получая ее с помощью g_get_home_dir()
        // имея несколько неприятных прецендентов, явно проверяем эту возможную проблему
        err_str = BF_("%1% is your home directory") % dir_str % bf::stop + abort_str;
        
#if GTK_CHECK_VERSION(2,24,11) && !GTK_CHECK_VERSION(2,24,15)
        err_str += "\n\n" "There is a known bug in GtkFileChooser, introduced in the GTK+ library 2.24.11 and fixed in 2.24.15: "
            "if user doesn't select a folder clearly then GtkFileChooser output the home directory. Users are encouraged to update to GTK >= 2.24.15.";
#endif
        
        ErrorBox(err_str);
        res = false;
    }
    if( !res )
        return false;

    fs::path dir_path(dir_str);
    ASSERT( fs::is_directory(dir_path) );
    if( !Project::HaveFullAccess(dir_path) )
    {
        MessageBox(BF_("Can't have full access to folder %1% (read, write)") % dir_str % bf::stop + abort_str,
                   Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        res = false;
    }
    else if( !fs::is_empty_directory(dir_path) )
    {
        bool is_empty = false;
        if( Gtk::RESPONSE_YES == 
            MessageBox(BF_("Folder %1% is not empty. We need to remove all files in it before authoring.\n"
                       "Continue?") % dir_str % bf::stop, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO) )
        {
            std::string err_str;
            is_empty = Project::ClearAllFiles(dir_path, err_str);
            if( !is_empty )
                MessageBox(BF_("Error during removing files: %1%") % err_str % bf::stop + abort_str,
                           Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        }
        res = is_empty;
    }
    return res;
}

///////////////////
// Run()

// COPY_N_PASTE_ETALON gtkdialog.c

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA.
 */

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

struct ExtPlayerData
{
    const char* binName;
    const char* title;
};

// см. комментарий про Preferences.player также
ExtPlayerData EPDList[] = 
{
#ifdef _WIN32
    { "vlc",   "VLC"   },
#else
    { "totem", "Totem" },
    { "xine",  "Xine"  },
#endif
};

ExtPlayerData GetEPD()
{
    return EPDList[Prefs().player];
}

static bool CheckAuthoringRes(const std::string& dir_str, std::string& dvd_dir)
{
    bool res = false;

    dvd_dir = fs::path(dir_str) / "dvd" / fs::to_str;
    if( !fs::exists(dvd_dir) )
        ErrorBox("Can't find the result of authoring: " + dvd_dir);
    else
        res = true;
    return res;
}

bool NotForPlay(guint response_id, const std::string& dir_str)
{
    bool res = true; 
    if( response_id == RESPONSE_TOTEM )
    {
        res = false;
        std::string cmd = GetEPD().binName;
        if( Project::IsSConsAuthoring() )
        {
            cmd = boost::format("%1% %2%") % GetSConsName() % cmd % bf::stop;
            Execution::SimpleSpawn(cmd.c_str(), dir_str.c_str());
        }
        else
        {
            std::string dvd_dir;
            if( CheckAuthoringRes(dir_str, dvd_dir) )
            {
                cmd = boost::format("%1% \"dvd://%2%\"") % cmd % dvd_dir % bf::stop;
                Execution::SimpleSpawn(cmd.c_str());
            }
        }
    }
    return res;
}

static std::string MakeOperStatus(boost::format tmpl, ExecState& es)
{
    return (tmpl % es.operationName).str();
}

static void FailureMessageBox(ExecState& es, const std::string& reason)
{
    if( es.eDat.userAbort )
        FinalMessageBox(MakeOperStatus(BF_("%1% cancelled."), es), false, Gtk::MESSAGE_INFO, "");
    else
        FinalMessageBox(MakeOperStatus(BF_("%1% broken."), es), false, Gtk::MESSAGE_ERROR,
                        BF_("The reason is \"%1%\" (see Details)") % reason % bf::stop);
}

static std::string OperationCompleted(ExecState& es)
{
    // Translators: can be tranlated as "Operation "%1%" ..."
    return MakeOperStatus(BF_("%1% successfully completed."), es);
}

bool CheckDVDBlank();
static bool CheckDVDBlankForBurning()
{
    bool res = true;
    if( GetES().mode == modBURN )
        res = CheckDVDBlank();
    return res;
}

static void BurnImpl(const std::string& dir_str)
{
    BurningStateSetter bss(GetES());
    ConsoleOF of;
    if( Project::IsSConsAuthoring() )
    {
        str::stream scons_options;
        FillSconsOptions(scons_options, false);

        ExecuteSconsCmd(dir_str, of, modBURN, scons_options);
    }
    else
    {
        std::string dvd_dir;
        if( CheckAuthoringRes(dir_str, dvd_dir) )
            Project::RunBurnCmd(of, dir_str);
    }
}

static void PostBuildOperation(const std::string& res, const std::string& dir_str)
{
    ExecState& es = GetES();
    if( IsGood(res) )
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

            dlg.add_button(BF_("_Play in %1%") % (Prefs().player == paTOTEM ? "Totem" : "Xine") % bf::stop, RESPONSE_TOTEM);
            dlg.add_button(_("_Burn to DVD"),   RESPONSE_BURN);
            std::string dvd_drive;
            if( /*(es.mode == modBURN) ||*/ !Author::IsBurnerSetup(dvd_drive) )
                dlg.set_response_sensitive(RESPONSE_BURN, false);
            dlg.add_button(Gtk::Stock::OK,   Gtk::RESPONSE_OK);
            dlg.set_default_response(Gtk::RESPONSE_OK);

            while( Run(dlg, false, bb::bind(&NotForPlay, _1, dir_str)) == RESPONSE_BURN )
            {
                dlg.hide();
                if( !CheckDVDBlank() )
                    break;

                // * прожиг
                std::string res = SafeCall(bb::bind(&BurnImpl, dir_str));
                if( IsGood(res) )
                {
                    SetFinalStatus(OperationCompleted(es), true);
                }
                else
                {    
                    FailureMessageBox(es, res);
                    break;
                }

                // * заново
                dlg.set_markup(MakeMessageBoxTitle(OperationCompleted(es)));
                gtk_message_dialog_format_secondary_text(dlg.gobj(), 0); // убираем текст
            }
        }
    }
    else
        FailureMessageBox(es, res);
}

static void AddError(std::string& err_lst, const std::string& err, bool appreciable_error = false)
{
    if( err_lst.size() )
        err_lst += "\n\n";
    err_lst += MarkError("* " + QuoteForGMarkupParser(err), !appreciable_error);
}

ListObj::ArrType& AllMediaObjs(Project::Menu mn)
{
    return GetMenuRegion(mn).List();
}

std::string RectToStr(const Rect& rct)
{
    return boost::format("(%1%, %2%, %3%, %4%)") % rct.lft % rct.top % rct.rgt % rct.btm % bf::stop;
}

std::string ItemName(Comp::MediaObj& obj)
{
    std::string str = RectToStr(obj.Placement());
    if( FrameThemeObj* fto = dynamic_cast<FrameThemeObj*>(&obj) )
        str = Editor::FT2PrintName(fto->Theme()) + " " + str;
    else if( TextObj* t_obj = dynamic_cast<TextObj*>(&obj) )
        str = t_obj->Text() + " " + str;
    return str;
}

void OnDVDBuild(Gtk::FileChooserButton& ch_btn)
{
    ExecState& es = GetES();
    if( !es.isExec )
    {
        // Политика: проверяем все то, что не даст явной ошибки в процессе выполнения,
        // но пользователю явно не подойдет результат работы
        std::string err_lst;
        // * бюджет
        if( Project::ProjectSizeSum() > Project::DVDPayloadSize() )
            AddError(err_lst, _("DVD capacity is exceeded"));
        
        // * нахлест кнопок
        // (по DVD-спекам он вообще не разрешен)
        try
        {
            boost_foreach( Project::Menu mn, Project::AllMenus() )
                boost_foreach( Comp::MediaObj* obj, AllMediaObjs(mn) )
                    if( Project::HasButtonLink(*obj) )
                    {    
                        boost_foreach( Comp::MediaObj* obj2, AllMediaObjs(mn) )
                            if( (obj != obj2) && Project::HasButtonLink(*obj2) 
				&& obj->Placement().Intersects(obj2->Placement()) )
                            {
                                AddError(err_lst, BF_("Items \"%1%\" and \"%2%\" overlap in menu \"%3%\"")
                                         % ItemName(*obj) % ItemName(*obj2) % mn->mdName % bf::stop);
                                throw 0; // до первой ошибки
                            }
                    }
        } catch (int) {}
        
        // * все в одном VTS храним, потому такое ограничение: каждый VTS_01_<N>.VOB <= 1GB,
        // N - однозначное число, от 1 до 9
        if( Project::ProjectStat().videoSum > io::pos(9)*1024*1024*1024 ) // 9GB
            AddError(err_lst, _("9GB limit for video is exceeded (one VTS should be less)"), true);
        
        if( DiscLabel().size() > 32 )
            AddError(err_lst, _("32 character limit for disc label is exceeded"), true);
        
        // :TODO: при наличие красных ошибок вообще не давать продолжать (защита от дурака)
        bool run_authoring = true;
        if( err_lst.size() && (Gtk::RESPONSE_YES != 
            MessageBox(_("Error Report"), Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, err_lst + "\n\n" + _("Continue?"))) )
            run_authoring = false;
        
        if( run_authoring )
        {
            std::string dir_str = ch_btn.get_filename();
            if( CanUseForAuthoring(dir_str) && CheckDVDBlankForBurning() )
            {
                std::string res;
                {
                    ExecStateSetter ess(es);
                    res = Project::AuthorDVD(dir_str);
                }
                PostBuildOperation(res, dir_str);
            }
        }
    }
    else
        es.eDat.StopExecution(es.operationName);
}

} // namespace Author

