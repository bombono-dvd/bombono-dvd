//
// mgui/mux.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#include "mux.h"
#include "execution.h" // ExecuteAsync()

#include <mgui/sdk/textview.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/widget.h>

#include <mgui/execution.h>
#include <mgui/gettext.h>
#include <mgui/dialog.h>

#include <mlib/filesystem.h>

#include <gtk/gtkwindow.h> // gtk_window_set_geometry_hints()

static void OnNewText(Gtk::TextView& txt_view, const char* dat, int sz, bool is_out,
                      const ReadReadyFnr& add_fnr, const std::string& prefix)
{
    AppendNewText(txt_view, prefix + std::string(dat, sz), is_out);
    if( add_fnr )
        add_fnr(dat, sz, is_out);
}

ReadReadyFnr TextViewAppender(Gtk::TextView& txt_view, const ReadReadyFnr& add_fnr,
                              const std::string& prefix)
{
    return bb::bind(&OnNewText, boost::ref(txt_view), _1, _2, _3, add_fnr, prefix);
}

static void OnResponse(Execution::Data& edat, int resp)
{
    if( resp == Gtk::RESPONSE_CANCEL)
    {
        // повтороне нажатие - закрытие диалога
        if( !edat.userAbort )
            edat.StopExecution(_("muxing"));
    }
}

void SetDialogStrict(Gtk::Dialog& dlg, int min_wdh, int min_hgt, bool set_resizable)
{
    dlg.set_resizable(set_resizable); // чтоб при закрытии экспандера диалог уменьшался
    // размер окна пошире
    GdkGeometry geom;
    geom.min_width  = min_wdh;
    geom.min_height = min_hgt;
    gtk_window_set_geometry_hints(static_cast<Gtk::Window&>(dlg).gobj(), 0, &geom, GDK_HINT_MIN_SIZE); 
}

static bool RunMuxing(const std::string& dest_path, const std::string& args)
{
    Gtk::Dialog dlg(BF_("Muxing \"%1%\"") % fs::name_str(dest_path) % bf::stop);
    SetDialogStrict(dlg, 400, -1);

    Gtk::TextView& txt_view = NewManaged<Gtk::TextView>();
    Execution::Data edat;
    Gtk::ProgressBar& prg_bar = NewManaged<Gtk::ProgressBar>();

    {
        Gtk::VBox& box = *dlg.get_vbox();
        PackStart(box, prg_bar);

        Gtk::Expander& expdr = PackStart(box, NewManaged<Gtk::Expander>(_("Show/_hide Details"), true));
        txt_view.set_editable(false);
        txt_view.set_size_request(0, 200);
        expdr.add(PackDetails(txt_view));

        dlg.get_action_area()->set_layout(Gtk::BUTTONBOX_CENTER);
        dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        dlg.signal_response().connect(bb::bind(&OnResponse, boost::ref(edat), _1));

        dlg.show_all();
    }

    //dlg.run();
    ExitData ed;
    {
        Execution::Pulse pls(prg_bar);
        std::string cmd = boost::format("mplex -f 8 -o %1% %2%") % dest_path % args % bf::stop;
        AppendCommandText(txt_view, cmd);
        ed = ExecuteAsync(0, cmd.c_str(), TextViewAppender(txt_view), &edat.pid);
    }

    if( !ed.IsGood() && !edat.userAbort )
    {
        MessageBox(_("Muxing error"), Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, 
                   BF_("The reason is \"%1%\" (see Details)") % ExitDescription(ed) % bf::stop);
        dlg.run();
    }
    return ed.IsGood();
}

struct SaveChooser: public Gtk::Table
{
                Gtk::Entry& ent;
    Gtk::FileChooserButton& fcb;

    SaveChooser(const char* type);
};

SaveChooser::SaveChooser(const char* type): 
    ent(NewManaged<Gtk::Entry>()), 
    fcb(NewManaged<Gtk::FileChooserButton>(_("Select a folder"), Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER))
{
    // вариант с виджетом - слишком большой, хоть и более функциональный
    //Gtk::FileChooserWidget& fcw = NewManaged<Gtk::FileChooserWidget>(Gtk::FILE_CHOOSER_ACTION_SAVE);
    //Gtk::Frame& frm = PackStart(vbox, NewManagedFrame(Gtk::SHADOW_ETCHED_IN, " Output: "), Gtk::PACK_EXPAND_WIDGET);
    //Add(Add(frm, NewPaddingAlg(0, 5, 5, 5)), fcw);

    set_col_spacings(5);
    Gtk::Label& o_lbl = NewManaged<Gtk::Label>(type);
    SetAlign(o_lbl, true);
    attach(o_lbl, 0, 1, 0, 1, Gtk::SHRINK|Gtk::FILL);
    attach(ent, 1, 2, 0, 1);
    Gtk::Label& i_lbl = NewManaged<Gtk::Label>(_("in"));
    SetAlign(i_lbl, false);
    attach(i_lbl, 0, 1, 1, 2, Gtk::SHRINK|Gtk::FILL);
    attach(fcb, 1, 2, 1, 2);
}

fs::path GetFilename(Gtk::FileChooser& fc)
{
    // fs::path v3 не понимает ustring
    // однако в gtkmm >= будет возвращать std::string, и это не понадобится
    return fs::path(fc.get_filename().raw());
}

std::string GetFilename(SaveChooser& sc)
{
    std::string fname = sc.ent.get_text();
    if( !fname.empty() )
        fname = (GetFilename(sc.fcb)/fname).string();
    
    return fname;
}

static void OnVideoSelected(Gtk::FileChooserButton& v_btn, Gtk::FileChooserButton& a_btn, SaveChooser& sc)
{
    fs::path pth = GetFilename(v_btn);
    if( pth.empty() )
        return;
    std::string folder = pth.parent_path().string();

    if( a_btn.get_filename().empty() )
        a_btn.set_current_folder(folder);

    if( GetFilename(sc).empty() )
    {
        sc.fcb.set_current_folder(folder);
        sc.ent.set_text(get_basename(pth) + ".mpg");
    }
}

bool MuxStreams(std::string& dest_fname, const std::string& src_fname)
{
    Gtk::Dialog dlg(_("Mux streams"));
    SetDialogStrict(dlg, 400, -1);
    SaveChooser& sc = NewManaged<SaveChooser>(SMCLN_("Output"));
    Gtk::FileChooserButton& v_btn = NewManaged<Gtk::FileChooserButton>(_("Select elementary video"), Gtk::FILE_CHOOSER_ACTION_OPEN);
    Gtk::FileChooserButton& a_btn = NewManaged<Gtk::FileChooserButton>(_("Select audio"),            Gtk::FILE_CHOOSER_ACTION_OPEN);
    {
        DialogVBox& vbox = AddHIGedVBox(dlg);

        AppendWithLabel(vbox, v_btn, SMCLN_("Video"));
        {
            Gtk::FileFilter f;
            f.set_name(_("MPEG2 elementary video (m2v)"));
            f.add_pattern("*.m2v");
            v_btn.add_filter(f);
        }

        AppendWithLabel(vbox, a_btn, SMCLN_("Audio"));
        {
            Gtk::FileFilter f;
            f.set_name(_("Audio for DVD") + std::string(" (mp2/mpa, ac3, dts or 16bit lpcm)"));
            FillSoundFilter(f);
            a_btn.add_filter(f);
        }

        PackStart(vbox, sc);

        CompleteDialog(dlg);
    }

    v_btn.signal_selection_changed().connect(bb::bind(&OnVideoSelected, boost::ref(v_btn), boost::ref(a_btn), boost::ref(sc)));
    if( !src_fname.empty() )
    {
        if( get_extension(src_fname) == "m2v" )
            v_btn.set_filename(src_fname);
        else
            a_btn.set_filename(src_fname);
    }

    bool res = false;
    for( ; res = Gtk::RESPONSE_OK == dlg.run(), res; )
    {
        dest_fname = GetFilename(sc);

        if( v_btn.get_filename().empty() )
            ErrorBox(_("Elementary video file is not selected."));
        else if( a_btn.get_filename().empty() )
            ErrorBox(_("Audio file is not selected."));
        else if( dest_fname.empty() )
            ErrorBox(_("Output file name is empty."));
        else if( CheckKeepOrigin(dest_fname) )
            ;
        else
            break;
    }

    if( res )
    {
        dlg.hide();
        res = RunMuxing(dest_fname, boost::format("%1% %2%") % v_btn.get_filename() % a_btn.get_filename() % bf::stop );
    }

    return res;
}

