
#include <mgui/_pc_.h>

#include "prefs.h"

#include <mgui/sdk/packing.h>
#include <mgui/sdk/widget.h>
#include <mgui/dialog.h>
#include <mgui/mguiconst.h>

#include <mbase/project/archieve.h>
#include <mbase/project/archieve-sdk.h>
#include <mbase/resources.h>

#include <mlib/gettext.h>
#include <mlib/sdk/logger.h>

#include <boost/lexical_cast.hpp>

template<class GtkObj>
GtkObj& AddWidget(RefPtr<Gtk::SizeGroup> wdg_sg, GtkObj& wdg)
{
    wdg_sg->add_widget(wdg);
    return wdg;
}

DialogVBox& AddHIGedVBox(Gtk::Dialog& dlg)
{
    // :KLUDGE: почему-то set_border_width() на dlg.get_vbox() не действует, поэтому использовать
    // MakeBoxHIGed() нельзя
    Gtk::VBox& box = *dlg.get_vbox();

    //return Add(PackStart(box, NewPaddingAlg(10, 10, 10, 10), Gtk::PACK_EXPAND_WIDGET), NewManaged<Gtk::VBox>(false, 10));
    DialogVBox& vbox = PackStart(box, NewManaged<DialogVBox>(false, 10), Gtk::PACK_EXPAND_WIDGET);
    vbox.set_border_width(10);
    return vbox;
}

void AppendWithLabel(DialogVBox& vbox, Gtk::Widget& wdg, const char* label)
{
    Gtk::HBox& hbox = PackStart(vbox, NewManaged<Gtk::HBox>());
    Gtk::Label& lbl = NewManaged<Gtk::Label>(label, true);
    SetAlign(lbl);
    lbl.set_mnemonic_widget(wdg);
    Add(PackStart(hbox, NewPaddingAlg(0, 0, 0, 5)), AddWidget(vbox.labelSg, lbl));

    PackStart(hbox, wdg, Gtk::PACK_EXPAND_WIDGET);
}

void SetDialogStrict(Gtk::Dialog& dlg, int min_wdh, int min_hgt);

void Preferences::Init()
{
    isPAL  = true;
    player = paTOTEM;
}

const int PREFS_VERSION = 1;

void SavePrefs(Project::ArchieveFnr afnr, const std::string& fname)
{
    xmlpp::Document doc;
    xmlpp::Element* root_node = doc.create_root_node("BmD");
    root_node->set_attribute("Version", boost::lexical_cast<std::string>(PREFS_VERSION));
    root_node->add_child_comment("Preferences for Bombono DVD");

    Project::DoSaveArchieve(root_node, afnr);
    doc.write_to_file_formatted(fname);
}

void SerializePrefs(Project::Archieve& ar)
{
    ar("PAL",    Prefs().isPAL  )
      ("Player", Prefs().player );
}

std::string PrefsPath()
{
    return GetConfigDir() + "/preferences.xml";
}

void LoadPrefs()
{
    std::string cfg_path = PrefsPath();
    try
    {
        Project::DoLoadArchieve(cfg_path, &SerializePrefs, "BmD");
    }
    catch (const std::exception& err)
    {
        LOG_ERR << "Couldn't load preferences from " << cfg_path << ": " << err.what() << io::endl;
        Prefs().Init();
    }
}

void ShowPrefs(Gtk::Window* win)
{
    Gtk::Dialog dlg(_("Bombono DVD Preferences"), false, true);
    if( win )
        dlg.set_transient_for(*win);
    SetDialogStrict(dlg, 400, 200);

    Gtk::ComboBoxText& tv_cmb = NewManaged<Gtk::ComboBoxText>();
    Gtk::ComboBoxText& pl_cmb = NewManaged<Gtk::ComboBoxText>();
    {
        DialogVBox& vbox = AddHIGedVBox(dlg);

        tv_cmb.append_text("PAL/SECAM");
        tv_cmb.append_text("NTSC");
        tv_cmb.set_active(Prefs().isPAL ? 0 : 1);
        AppendWithLabel(vbox, tv_cmb, _("_Default Project Type"));

        pl_cmb.append_text("Totem");
        pl_cmb.append_text("Xine");
        pl_cmb.set_active(Prefs().player);
        AppendWithLabel(vbox, pl_cmb, _("_Play Authoring Result in"));

        CompleteDialog(dlg, true);
    }

    dlg.run();
    //if( Gtk::RESPONSE_OK == dlg.run() )
    {
        Prefs().isPAL  = tv_cmb.get_active_row_number() == 0;
        Prefs().player = (PlayAuthoring)pl_cmb.get_active_row_number();

        SavePrefs(&SerializePrefs, PrefsPath());
    }
}

