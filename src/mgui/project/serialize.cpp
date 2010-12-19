//
// mgui/project/serialize.cpp
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

#include "serialize.h"

#include "mconstructor.h"
#include "menu-render.h"

#include "menu-actions.h"
#include "add.h"
#include "handler.h"

#include <mgui/dialog.h>
#include <mgui/gettext.h>
#include <mgui/prefs.h>
#include <mgui/sdk/widget.h>
#include <mgui/sdk/packing.h>

#include <mbase/project/table.h>
#include <mbase/resources.h>

#include <mlib/lambda.h> // bl::var
#include <mlib/filesystem.h>

namespace Project
{

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
        return "untitled.bmd";

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
    if( ChooseFileSaveTo(fname, _("Save Project As..."), for_wdg) )
    {
        fname = Project::ConvertPathToUtf8(fname);

        ASSERT( !fname.empty() );
        AData().SetProjectFName(fname);
        res = true;
    }

    return res;
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
        MessageBox(BF_("Can't open project file \"%1%\"") % prj_file_name % bf::stop, Gtk::MESSAGE_ERROR, 
                   Gtk::BUTTONS_OK, err_str);
    }
}

void SetAppTitle(bool clear_change_flag)
{
    ConstructorApp& app = Application();
    if( clear_change_flag )
        app.isProjectChanged = false;

    const char* ch_flag = app.isProjectChanged ? "*" : "" ;
    app.win.set_title(ch_flag + MakeProjectTitle(true) + " - " APROJECT_NAME);
}

void LoadApp(const std::string& fname)
{
    LoadProjectInteractive(ConvertPathToUtf8(fname));

    AStores& as = GetAStores();
    ADatabase& db = AData();

    PublishMediaStore(as.mdStore);
    PublishMenuStore(as.mnStore, db.GetMN());
    UpdateDVDSize();

    db.SetOut(true);
    SetAppTitle();
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

static void OnSaveAsProject();
static void OnSaveProject();

// предложить пользователю сохранить измененный проект перед закрытием
bool CheckBeforeClosing(ConstructorApp& app)
{
    bool res = true;
    if( app.isProjectChanged )
    {
    
        Gtk::MessageDialog dlg("<span weight=\"bold\" size=\"large\">" +
                               BF_("Save changes to \"%1%\"?") % MakeProjectTitle() % bf::stop + 
                               "</span>", true,  Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
        dlg.add_button(_("Close _without Saving"), Gtk::RESPONSE_CLOSE);
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
            OnSaveProject();
    }
    return res;
}

static Gtk::RadioButton& TVSelectionButton(bool &is_pal, bool val, Gtk::RadioButtonGroup& grp)
{
    const char* name = val ? "_PAL/SECAM" : "_NTSC" ;
    Gtk::RadioButton& btn = *Gtk::manage(new Gtk::RadioButton(grp, name, true));
    if( is_pal == val )
        btn.set_active();
    SetForRadioToggle(btn, bl::var(is_pal) = val);
    return btn;
}

static void OnNewProject(ConstructorApp& app)
{
    bool is_pal = Prefs().isPAL;
    Gtk::Dialog new_prj_dlg(_("New Project"), app.win, true, true);
    new_prj_dlg.set_name("NewProject");
    new_prj_dlg.set_resizable(false);
    {
        AddCancelDoButtons(new_prj_dlg, Gtk::Stock::OK);
        Gtk::VBox& dlg_box = *new_prj_dlg.get_vbox();
        PackStart(dlg_box, NewManaged<Gtk::Image>((fs::path(GetDataDir())/"cap400.png").string()));
        Gtk::VBox& vbox = Add(PackStart(dlg_box, NewPaddingAlg(10, 40, 20, 20)), NewManaged<Gtk::VBox>());
        
        PackStart(vbox, NewManaged<Gtk::Label>(_("Please select a Television standard for your project:"),
                                               0.0, 0.5, true));
        {
            Gtk::VBox& vbox2 = Add(PackStart(vbox, NewPaddingAlg(10, 10, 0, 0)), NewManaged<Gtk::VBox>());

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
        SetAppTitle();
    }
}

static void OnOpenProject(ConstructorApp& app)
{
    Gtk::FileChooserDialog dialog(_("Open Project"), Gtk::FILE_CHOOSER_ACTION_OPEN);
    BuildChooserDialog(dialog, true, app.win);

    Gtk::FileFilter prj_filter;
    prj_filter.set_name(_("Project files (*.bmd)"));
    prj_filter.add_pattern("*.bmd");
    // старые проекты
    prj_filter.add_pattern("*.xml");
    dialog.add_filter(prj_filter);

    Gtk::FileFilter all_filter;
    all_filter.set_name(_("All Files (*.*)"));
    all_filter.add_pattern("*");
    dialog.add_filter(all_filter);

    if( Gtk::RESPONSE_OK == dialog.run() )
    {
        // в процессе загрузки не нужен
        dialog.hide();
        IteratePendingEvents();

        NewProject();
        AData().SetOut(false);

        LoadApp(dialog.get_filename());
    }
}

static void SaveProject()
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

    SetAppTitle();
}

static void OnSaveProject()
{
    if( AData().IsProjectSet() )
        SaveProject();
    else
        OnSaveAsProject();
}

static void OnSaveAsProject()
{
    if( SaveProjectAs(Application().win) )
        SaveProject();
}

typedef boost::function<void(ConstructorApp&)> ConstructorAppFnr;
static void OnOtherProject(ConstructorAppFnr& fnr)
{
    ConstructorApp& app = Application();
    if( CheckBeforeClosing(app) )
        fnr(app);
}

ActionFunctor MakeOnOtherPrjFunctor(const ConstructorAppFnr& fnr)
{
    return bb::bind(OnOtherProject, fnr);
}

void AddSrlActions(RefPtr<Gtk::ActionGroup> prj_actions)
{
    prj_actions->add( Gtk::Action::create("New",   Gtk::Stock::NEW,  _("_New Project")),
                      Gtk::AccelKey("<control>N"), MakeOnOtherPrjFunctor(OnNewProject) );
    prj_actions->add( Gtk::Action::create("Open",   Gtk::Stock::OPEN, _("_Open...")),
                      Gtk::AccelKey("<control>O"), MakeOnOtherPrjFunctor(OnOpenProject) );
    prj_actions->add( Gtk::Action::create("Save",   Gtk::Stock::SAVE, _("_Save")),
                      Gtk::AccelKey("<control>S"), &OnSaveProject );
    prj_actions->add( Gtk::Action::create("SaveAs", Gtk::Stock::SAVE_AS, _("Save _As...")),
                      &OnSaveAsProject );
}

} // namespace Project

