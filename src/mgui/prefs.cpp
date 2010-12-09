//
// mgui/prefs.cpp
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

#include "prefs.h"
#include "design.h"

#include <mgui/sdk/packing.h>
#include <mgui/sdk/widget.h>
#include <mgui/sdk/window.h>
#include <mgui/dialog.h>
#include <mgui/mguiconst.h>

#include <mbase/project/archieve.h>
#include <mbase/project/archieve-sdk.h>
#include <mbase/project/srl-common.h> // Serialize(Archieve& ar, Point& pnt)

#include <mbase/resources.h>

#include <mlib/gettext.h>
#include <mlib/filesystem.h>
#include <mlib/sdk/logger.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/convenience.hpp> // fs::create_directories()

static std::string PreferencesPath(const char* fname)
{
    return GetConfigDir() + "/" + fname;
}

void SavePrefs(Project::ArchieveFnr afnr, const char* fname, int version)
{
    xmlpp::Document doc;
    xmlpp::Element* root_node = doc.create_root_node("BmD");
    root_node->set_attribute("Version", boost::lexical_cast<std::string>(version));
    root_node->add_child_comment("Preferences for Bombono DVD");

    Project::DoSaveArchieve(root_node, afnr);
    doc.write_to_file_formatted(PreferencesPath(fname));
}

const int PREFS_VERSION = 2;

void SerializePrefs(Project::Archieve& ar)
{
    int load_ver = PREFS_VERSION;
    if( ar.IsLoad() )
        ar("Version", load_ver);


    ar("PAL",    Prefs().isPAL  )
      ("Player", Prefs().player );

    if( ar.IsSave() || load_ver >= 2 )
        ar("DefAuthorPath", Prefs().authorPath);
}

static bool LoadPrefs(const char* fname, const Project::ArchieveFnr& fnr)
{
    bool res = false;
    std::string cfg_path = PreferencesPath(fname);
    try
    {
        if( fs::exists(cfg_path) )
        {
            Project::DoLoadArchieve(cfg_path, fnr, "BmD");
            res = true;
        }
    }
    catch (const std::exception& err)
    {
        LOG_WRN << "Couldn't load preferences from " << cfg_path << ": " << err.what() << io::endl;
    }
    return res;
}

//
// Preferences
// 

void Preferences::Init()
{
    isPAL  = true;
    player = paTOTEM;
    authorPath = (fs::path(Glib::get_user_cache_dir()) / "bombono-dvd-video").string();
}

const char* PrefsName = "preferences.xml";

void LoadPrefs()
{
    if( !LoadPrefs(PrefsName, &SerializePrefs) )
        Prefs().Init();
}

void TrySetDirectory(Gtk::FileChooser& fc, const std::string& dir_path)
{
    try
    {
        if( !fs::exists(dir_path) )
            fs::create_directories(dir_path);
    }
    catch (const std::exception& err)
    {
        LOG_WRN << "TrySetDirectory(" << dir_path << "): " << err.what() << io::endl;
    }
    fc.set_filename(dir_path);
}

void ShowPrefs(Gtk::Window* win)
{
    Gtk::Dialog dlg(_("Bombono DVD Preferences"), false, true);
    AdjustDialog(dlg, 450, 200, win, false);

    Gtk::ComboBoxText& tv_cmb = NewManaged<Gtk::ComboBoxText>();
    Gtk::ComboBoxText& pl_cmb = NewManaged<Gtk::ComboBoxText>();
    Gtk::FileChooserButton& a_btn = NewManaged<Gtk::FileChooserButton>("Select output folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    {
        DialogVBox& vbox = AddHIGedVBox(dlg);

        tv_cmb.append_text("PAL/SECAM");
        tv_cmb.append_text("NTSC");
        tv_cmb.set_active(Prefs().isPAL ? 0 : 1);
        AppendWithLabel(vbox, tv_cmb, _("_Default project type"));

        TryDefaultAuthorPath(a_btn);
        AppendWithLabel(vbox, a_btn, _("Default _folder for authoring"));

        pl_cmb.append_text("Totem");
        pl_cmb.append_text("Xine");
        pl_cmb.set_active(Prefs().player);
        AppendWithLabel(vbox, pl_cmb, _("_Play authoring result in"));

        CompleteDialog(dlg, true);
    }

    dlg.run();
    //if( Gtk::RESPONSE_OK == dlg.run() )
    {
        Prefs().isPAL  = tv_cmb.get_active_row_number() == 0;
        Prefs().player = (PlayAuthoring)pl_cmb.get_active_row_number();
        Prefs().authorPath = a_btn.get_filename();

        SavePrefs(&SerializePrefs, PrefsName, PREFS_VERSION);
    }
}

//
// UnnamedPreferences
// 

void UnnamedPreferences::Init()
{
    isLoaded = false; // appPos неопределен

    appSz    = CalcBeautifulRect(APPLICATION_WDH);
    fbWdh     = FCW_WDH;
    mdBrw1Wdh = BROWSER_WDH;
}

const int UNNAMED_PREFS_VERSION = 1;

void SerializeUnnamedPrefs(Project::Archieve& ar)
{
    //int load_ver = UNNAMED_PREFS_VERSION;
    //if( ar.IsLoad() )
    //    ar("Version", load_ver);
    UnnamedPreferences& up = UnnamedPreferences::Instance();

    ar( "AppSizes",         up.appSz  )
      ( "AppPosition",      up.appPos )
      ( "FileBrowserWidth", up.fbWdh  )
      ( "MDBrowserWidth",   up.mdBrw1Wdh );
}

const char* UnnamedPrefsName = "unnamed_preferences.xml";

UnnamedPreferences& UnnamedPrefs()
{
    UnnamedPreferences& up = UnnamedPreferences::Instance();

    static bool first_time = true;
    if( first_time )
    {
        first_time = false;
        if( LoadPrefs(UnnamedPrefsName, &SerializeUnnamedPrefs) )
            up.isLoaded = true;
        else
            up.Init();
    }

    return up;
}

void SaveUnnamedPrefs()
{
    SavePrefs(&SerializeUnnamedPrefs, UnnamedPrefsName, UNNAMED_PREFS_VERSION);
}

static void UpdatePosition(Gtk::HPaned& hpaned, int& saved_pos)
{
    saved_pos = hpaned.get_position();
}

void SetUpdatePos(Gtk::HPaned& hpaned, int& saved_pos)
{
    hpaned.set_position(saved_pos);
    hpaned.signal_hide().connect(bb::bind(&UpdatePosition, b::ref(hpaned), b::ref(saved_pos)));
}

