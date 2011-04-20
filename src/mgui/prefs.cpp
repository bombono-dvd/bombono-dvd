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
#include <mgui/win_utils.h> // SetTip()

#include <mbase/project/archieve.h>
#include <mbase/project/archieve-sdk.h>
#include <mbase/project/srl-common.h> // Serialize(Archieve& ar, Point& pnt)

#include <mbase/resources.h>

#include <mlib/gettext.h>
#include <mlib/filesystem.h>
#include <mlib/sdk/logger.h>

#include <mlib/sigc.h>

#include <boost/lexical_cast.hpp>

std::string PreferencesPath(const char* fname)
{
    return GetConfigDir() + "/" + fname;
}

void SavePrefs(Project::ArchieveFnr afnr, const char* fname, int version)
{
    Project::XMLSave xs("BmD");
    xs.rootNode->add_child_comment("Preferences for Bombono DVD");
    Project::SaveXS(xs, afnr, version, PreferencesPath(fname));
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

const int PREFS_VERSION = 6;

void SerializePrefs(Project::Archieve& ar)
{
    ar("PAL",    Prefs().isPAL  );
    if( CanSrl(ar, 6) )
        ar("RemMyTVChoice", Prefs().remMyTVChoice);
    
    ar("Player", Prefs().player );

    if( CanSrl(ar, 2) )
        ar("DefAuthorPath", Prefs().authorPath);
    if( CanSrl(ar, 3) )
        ar("ShowSrcFileBrowser", Prefs().showSrcFileBrowser);
    if( CanSrl(ar, 4) )
        ar("MaxCPUWorkload", Prefs().maxCPUWorkload);
}

void Preferences::Init()
{
    isPAL  = true;
    remMyTVChoice = false;
    player = paTOTEM;
    authorPath = (fs::path(Glib::get_user_cache_dir()) / "bombono-dvd-video").string();
    showSrcFileBrowser = false;
    maxCPUWorkload = 1;
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
    //fc.set_filename(dir_path);
    SetFilename(fc, dir_path);
}

static void NotifyToRestart()
{
    MessageBox(_("You need to restart the application for the changes to take place"), Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
}

// максимальное кол-во работ, которое можно выполнять одновременно
// = кол-ву процессоров/ядер
int MaxCPUWorkload()
{
#ifdef _WIN32
#error "TODO"
#else
    // кол-во работающих процессоров, а не всего, _SC_NPROCESSORS_CONF 
    // (система может использовать не все)
    int res = sysconf(_SC_NPROCESSORS_ONLN);
    if( res <= 0 ) // может возвращать -1, если не осилит
        res = 1;
    return res;
#endif
}

// реализация дискретного ползунка (по целым числам)
// если использовать property_draw_value() == true, то с помощью
// атрибута round_digits включается встроенный механизм дискретизации
// (видно сложилось исторически);
// в ином случае нужно переопределять сигнал "change-value", как тут
static bool OnIntChangeVal(Gtk::HScale& hs, double val)
{
    hs.set_value(Round(val));
    // не пропускаем оригинальный обработчик GtkRange
    return true;
}

void SavePrefs()
{
    SavePrefs(&SerializePrefs, PrefsName, PREFS_VERSION);
}

void ShowPrefs(Gtk::Window* win)
{
    Gtk::Dialog dlg(_("Bombono DVD Preferences"), false, true);
    AdjustDialog(dlg, 450, 200, win, false);

    // :TODO: стоит ставить двоеточия и здесь (автоматом добавлять в
    // AppendWithLabel())
    Gtk::ComboBoxText& tv_cmb = NewManaged<Gtk::ComboBoxText>();
    Gtk::ComboBoxText& pl_cmb = NewManaged<Gtk::ComboBoxText>();
    Gtk::FileChooserButton& a_btn = NewManaged<Gtk::FileChooserButton>("Select output folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    Gtk::CheckButton& fb_btn = NewManaged<Gtk::CheckButton>(_("Show File Browser"));
    int max_val = MaxCPUWorkload();
    Gtk::HScale& wl_hs = NewManaged<Gtk::HScale>(CreateAdj(Prefs().maxCPUWorkload, max_val));
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

        fb_btn.set_active(Prefs().showSrcFileBrowser);
        fb_btn.signal_toggled().connect(&NotifyToRestart);
        PackStart(vbox, fb_btn);
        
        SetScaleSecondary(wl_hs);
        sig::connect(wl_hs.signal_change_value(), bb::bind(&OnIntChangeVal, b::ref(wl_hs), _2));
        for( int i=1; i<=max_val; i++ )
            wl_hs.add_mark(i, Gtk::POS_TOP, Int2Str(i));
        AppendWithLabel(vbox, wl_hs, _("Multi-core CPU support"));
        SetTip(wl_hs, _("Make use of multi-core CPU for transcoding videos quickly; 1 is not to use multi-coreness, safe minimum (no possible CPU overheat)"));

        CompleteDialog(dlg, true);
    }

    dlg.run();
    //if( Gtk::RESPONSE_OK == dlg.run() )
    {
        Prefs().isPAL  = tv_cmb.get_active_row_number() == 0;
        Prefs().player = (PlayAuthoring)pl_cmb.get_active_row_number();
        Prefs().authorPath = a_btn.get_filename();
        Prefs().showSrcFileBrowser = fb_btn.get_active();
        Prefs().maxCPUWorkload = wl_hs.get_value();

        SavePrefs();
    }
}

//
// UnnamedPreferences
// 

void UnnamedPreferences::Init()
{
    isLoaded = false; // appPos неопределен

    appSz    = CalcBeautifulRect(APPLICATION_WDH);
    srcBrw1Wdh = FCW_WDH;
    srcBrw2Wdh = BROWSER_WDH;
    fcMap.clear();
    
#ifdef _WIN32
    ihsNum = 1; // Desktop Nexus
#else
    ihsNum = 0; // GNOME Backgrounds
#endif
}

const int UNNAMED_PREFS_VERSION = 3;

static void SerializeFC(Project::Archieve& ar, FCState& fc, std::string& name)
{
    ar( "Name",    name          )
      ( "LastDir", fc.lastDir    )
      ( "LastFlt", fc.lastFilter );
}

static void LoadFCState(Project::Archieve& ar)
{
    std::string name;
    FCState fc;
    SerializeFC(ar, fc, name);
    UnnamedPreferences::Instance().fcMap[name] = fc;
}

void SerializeUnnamedPrefs(Project::Archieve& ar)
{
    UnnamedPreferences& up = UnnamedPreferences::Instance();

    ar( "AppSizes",         up.appSz  )
      ( "AppPosition",      up.appPos )
      ( "FileBrowserWidth", up.srcBrw1Wdh )
      ( "MDBrowserWidth",   up.srcBrw2Wdh );

    if( CanSrl(ar, 2) )
    {
        Project::ArchieveStackFrame asf(ar, "FCMap");
        if( ar.IsLoad() )
            Project::LoadArray(ar, &LoadFCState, "Item");
        else
            boost_foreach( FCMap::reference ref, up.fcMap )
            {
                Project::ArchieveStackFrame asf(ar, "Item");
                std::string name = ref.first;
                SerializeFC(ar, ref.second, name);
            }
    }

    if( CanSrl(ar, 3) )
        ar("ImageHostingNumber", up.ihsNum);
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

bool PrefToBool(const std::string& str)
{
    return str == "1";
}

