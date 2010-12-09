//
// mgui/timeline/select.cpp
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

#include "service.h"

#include <mgui/project/add.h>

#include <mgui/project/handler.h>
#include <mgui/project/thumbnail.h>
#include <mgui/key.h>
#include <mgui/sdk/entry.h>
#include <mgui/sdk/menu.h>  // Popup()
#include <mgui/sdk/window.h>
#include <mgui/sdk/widget.h>
#include <mgui/execution.h>
#include <mgui/dialog.h> // ChooseFileSaveTo()
#include <mgui/gettext.h>

#include <mbase/project/table.h>
#include <mlib/filesystem.h>
#include <mlib/sigc.h>
#include <cmath> // std::ceil()


namespace Timeline
{

void TLState::ChangeState(TrackLayout& trk, TLState& stt)
{
    trk.ChangeState(&stt);
}

static void DoHookAction(ptr::shared<HookAction> act)
{
    HookSvc svc(act);
    svc.FormLayout();
    svc.Action()->Process();
}

// контекстное меню шкалы
class ContextMenuHook: public HookAction
{
    typedef HookAction MyParent;
    public:

                        ContextMenuHook(TrackLayout& trk_lay, GdkEventButton* ev)
                            : MyParent(trk_lay, DPoint(ev->x, ev->y)), event(ev) {}

          virtual void  Process();
          virtual void  AtScale();
          virtual void  AtDVDMark(int idx);

    protected:
          RefPtr<Gtk::ActionGroup> popupActions;
                   GdkEventButton* event;
};

void RedrawDVDMark(TrackLayout& trk, int idx)
{
    DVDLabelCoverSvc svc(trk, idx);
    svc.FormLayout();
}

static void DeleteDVDMark(TrackLayout& /*trk*/, int idx)
{
    //RedrawDVDMark(trk, idx);
    Project::DeleteChapter(DVDMarks(), DVDMarks().begin()+idx);
}

static void DeleteAllDVDMarks(TrackLayout& trk)
{
    while( DVDMarks().size() )
        DeleteDVDMark(trk, DVDMarks().size()-1);
}

// вернуть по расширению тип, в котором сохранять картинку
static std::string GetFormatByExt(const char* ext, bool jpeg_on_break = false)
{
    if( strcmp("jpg", ext) == 0 )
        ext = "jpeg";

    typedef Gdk::Pixbuf::SListHandle_PixbufFormat Formats;
    Formats formats = Gdk::Pixbuf::get_formats();
    for( Formats::iterator it = formats.begin(); it != formats.end(); ++it )
    {
        Gdk::PixbufFormat frmt = *it;
        if( strcmp(frmt.get_name().c_str(), ext) == 0 )
        {
            if( !frmt.is_writable() )
                break;
            return ext;
        }
    }
    return std::string( jpeg_on_break ? "jpeg" : "" );
}

static std::string& SaveFrameDir()
{
    static std::string dir;
    return dir;
}

static void OnSaveFrameDialog(Gtk::FileChooserDialog& dlg, Gtk::CheckButton& btn)
{
    if( !SaveFrameDir().empty() )
        dlg.set_current_folder(SaveFrameDir());
    dlg.set_extra_widget(btn);
}

static void SaveFrame(DAMonitor& mon)
{
    time4_t t4 = FramesToTime(mon.CurPos()>=0 ? mon.CurPos() : 0, FrameFPS(mon.GetViewer()));
    str::stream strm;
    strm << "frame";
    if( t4.hh != 0 )
        strm << Mpeg::set_hms() << t4.hh;
    strm << Mpeg::set_hms() << t4.mm;
    strm << Mpeg::set_hms() << t4.ss;
    strm << Mpeg::set_hms() << t4.ff;
    strm << ".jpeg";

    std::string fnam = strm.str();
    Gtk::CheckButton& add_btn = NewManaged<Gtk::CheckButton>(_("A_dd to project"), true);
    RefPtr<Gtk::CheckButton> add_ref(MakeRefPtr(&add_btn)); 

    if( ChooseFileSaveTo(fnam, _("Save Frame..."), mon, bb::bind(&OnSaveFrameDialog, _1, boost::ref(add_btn))) )
    {
        // находим расширение и по нему сохраняем
        int i = fnam.rfind('.');
        Glib::ustring ext = (i >= 0) ? Glib::ustring(fnam, i+1, fnam.size()) : "" ;
        ext = GetFormatByExt(ext.c_str(), true);

        mon.FramePixbuf()->save(fnam, ext);

        SaveFrameDir() = fs::path(fnam).branch_path().string();
        if( add_btn.get_active() )
            Project::TryAddMediaQuiet(fnam, "SaveFrame");
    }
}

static int LastPos(TrackLayout& trk_lay)
{
    return (int)std::ceil(trk_lay.GetFramesLength());
}

static void InsertDVDMarkAtPos(TrackLayout& trk_lay, int pos);

static void InsertChapters(TrackLayout& trk_lay)
{
    Gtk::Dialog add_dlg(_("Add Chapter Points at Intervals"), *GetTopWindow(trk_lay), true);
    Gtk::SpinButton*  btn  = 0;
    Gtk::CheckButton* cbtn = 0;
    {
        Gtk::VBox& vbox = AddHIGedVBox(add_dlg);

        Gtk::HBox& hbox = PackStart(vbox, NewManaged<Gtk::HBox>());
        Add(PackStart(hbox, NewPaddingAlg(0, 0, 0, 40)), NewManaged<Gtk::Label>(_("Interval between Chapters:")));
        btn = &PackStart(hbox, NewManaged<Gtk::SpinButton>());
        ConfigureSpin(*btn, 5, 1000); // 5 мин. по умолчанию

        Gtk::Label& lbl = PackStart(hbox, NewManaged<Gtk::Label>(_("min.")));
        lbl.set_padding(2, 0);

        cbtn = &PackStart(vbox, NewManaged<Gtk::CheckButton>(_("Remove Existing Chapters")));
    }

    if( CompleteAndRunOk(add_dlg) )
    {
        int intr = btn->get_value_as_int() * 60; // секунды
        if( cbtn->get_active() )
            DeleteAllDVDMarks(trk_lay);

        int last_pos = LastPos(trk_lay);
        for( int i = 1, pos; pos = TimeToFrames(i*intr, trk_lay.FrameFPS()), pos < last_pos; i++ )
            InsertDVDMarkAtPos(trk_lay, pos);
    }
}

static void PlayInTotem(TrackLayout& trk_lay)
{
    uint64_t msec = uint64_t(trk_lay.CurPos()/trk_lay.FrameFPS()*1000);
    // уже запущенный Totem не воспринимает --seek, поэтому сначала закрываем его
    std::string cmd = boost::format("totem --quit; totem --seek %1% %2%") % msec % GetFilename(*CurrVideo) % bf::stop;
    Execution::SimpleSpawn(cmd.c_str());
}

void ContextMenuHook::AtScale()
{
    popupActions = Gtk::ActionGroup::create("Actions");

    // Add
    popupActions->add( Gtk::Action::create("Add Chapter", _("Add Chapter Point")),
                       bb::bind(&InsertDVDMark, boost::ref(trkLay)) );
    // Delete
    RefPtr<Gtk::Action> act = Gtk::Action::create("Delete Chapter", _("Delete Chapter Point"));
    act->set_sensitive(false);
    popupActions->add( act );
    // Delete All
    act = Gtk::Action::create("Delete All", _("Delete All Chapter Points"));
    if( DVDMarks().size() == 0 )
        act->set_sensitive(false);
    popupActions->add( act, bb::bind(&DeleteAllDVDMarks, boost::ref(trkLay)) );
    // Add at Intervals
    popupActions->add( Gtk::Action::create("Add at Intervals", DOTS_("Add Chapter Points at Intervals")),
                       bb::bind(&InsertChapters, boost::ref(trkLay)) );

    // Save
    ActionFunctor save_fnr = boost::function_identity; // bl::constant(0); // если не mon, то пустой
    DAMonitor* mon = dynamic_cast<DAMonitor*>(&trkLay.GetMonitor());
    if( mon )
        save_fnr = bb::bind(&SaveFrame, boost::ref(*mon));
    popupActions->add( Gtk::Action::create("Save Frame", Gtk::Stock::SAVE, DOTS_("Save Current Frame")),
                       save_fnr );
    popupActions->add( Gtk::Action::create("Play in Totem", Gtk::Stock::MEDIA_PLAY, 
                                           BF_("_Play in %1%") % "Totem" % bf::stop),
                       bb::bind(&PlayInTotem, boost::ref(trkLay)) );
}

void ContextMenuHook::AtDVDMark(int idx)
{
    RefPtr<Gtk::Action> act;
    act = popupActions->get_action("Add Chapter");
    act->set_sensitive(false);

    act = popupActions->get_action("Delete Chapter");
    act->set_sensitive(true);
    act->signal_activate().connect( bb::bind(&DeleteDVDMark, boost::ref(trkLay), idx) );
}

void ContextMenuHook::Process()
{
    if( popupActions )
    {
        RefPtr<Gtk::UIManager> mngr = Gtk::UIManager::create();
        mngr->insert_action_group(popupActions);
        //add_accel_group(m_refUIManager->get_accel_group());

        Glib::ustring ui_info = 
        "<ui>"
        "  <popup  name='ScaleMenu'>"
        "    <menuitem action='Add Chapter'/>"
        "    <menuitem action='Delete Chapter'/>"
        "    <menuitem action='Delete All'/>"
        "    <menuitem action='Add at Intervals'/>"
        "    <separator/>"
        "    <menuitem action='Save Frame'/>"
        "    <menuitem action='Play in Totem'/>"
        "  </popup>"
        "</ui>";
        mngr->add_ui_from_string(ui_info);

        Gtk::Menu* menu = dynamic_cast<Gtk::Menu*>(mngr->get_widget("/ScaleMenu"));
        ASSERT( menu );

        // вообще-то menu удалится как только будет удален mngr, однако,
        // раз оно появилось из-за popup(), то доп. ссылку видно держит сам GTK (как для окон
        // верхнего уровня), поэтому прокатывает
        Popup(*menu, event);
    }
}

void NormalTL::OnMouseDown(TrackLayout& trk, GdkEventButton* event)
{
    if( IsLeftButton(event) )
        DoHookAction(new LeftMouseHook(trk, DPoint(event->x, event->y)));

    if( IsRightButton(event) )
        DoHookAction(new ContextMenuHook(trk, event));
}

// установка курсора мыши
class CursorSetterHook: public HookAction
{
    typedef HookAction MyParent;
    public:

                        CursorSetterHook(TrackLayout& trk_lay, const DPoint& lct_)
                            : MyParent(trk_lay, lct_), isDef(true), dvdIdx(NO_HNDL) {}

          virtual void  Process();
          virtual void  AtBigLabel();
          // :TODO: добавить тултипы к меткам глав - показывать их названия
          //virtual void  AtDVDMark(int idx);

    protected:
                bool  isDef;
                 int  dvdIdx;
};

void CursorSetterHook::AtBigLabel()
{
    isDef = false;
    Gdk::Cursor curs(Gdk::HAND2);
    SetCursorForWdg(trkLay, &curs);
}

void CursorSetterHook::Process()
{
    if( isDef )
        SetCursorForWdg(trkLay);
}

void NormalTL::OnMouseMove(TrackLayout& trk, GdkEventMotion* event)
{
    DoHookAction(new CursorSetterHook(trk, DPoint(event->x, event->y)));
}

// static bool operator<(const DVDMark& m1, const DVDMark& m2)
// {
//     return m1.pos < m2.pos;
// }

static bool TimelineComparer(const DVDMark& m1, const DVDMark& m2)
{
    return GetMarkData(m1).pos < GetMarkData(m2).pos;
}

static DVDMark MakeChapterForSearch(int frame_pos)
{
    DVDMark tmp_mrk(Project::MakeEmptyChapter());
    GetMarkData(tmp_mrk).pos = frame_pos;
    return tmp_mrk;
}

void SetPointerAtDVDMark(TrackLayout& trk, bool is_left)
{
    int cur_pos = trk.CurPos();
    ASSERT( cur_pos >= 0 );
    DVDArrType::iterator beg = DVDMarks().begin(), end = DVDMarks().end();
    if( beg == end )
        return;

    DVDMark tmp_mrk = MakeChapterForSearch(cur_pos);
    if( is_left )
    {
        DVDArrType::iterator it = std::lower_bound(beg, end, tmp_mrk, TimelineComparer);
        if( it != beg )
            SetPointer(GetMarkData(*(it-1)).pos, trk);
    }
    else // right
    {
        DVDArrType::iterator it = std::upper_bound(beg, end, tmp_mrk, TimelineComparer);
        if( it != end )
            SetPointer(GetMarkData(*it).pos, trk);
    }
}

static void DeleteDVDMarkUnderPointer(TrackLayout& trk)
{
    int cur_pos = trk.CurPos();
    ASSERT( cur_pos >= 0 );
    DVDArrType::iterator beg = DVDMarks().begin(), end = DVDMarks().end();
    DVDArrType::iterator it = std::upper_bound(beg, end, MakeChapterForSearch(cur_pos), 
                                               TimelineComparer);
    if( it != beg )
    {
        it -= 1;
        //if( it->pos == cur_pos )
        if( GetMarkData(*it).pos == cur_pos )
            DeleteDVDMark(trk, it-beg);
    }
}

void NormalTL::OnKeyPressEvent(TrackLayout& trk, GdkEventKey* event)
{
    int curPos = trk.CurPos();
    if( curPos<0 )
        return;

    // :TODO: "key_binding" - см. meditor_text.cpp
    if( CanShiftOnly(event->state) )
    {
        switch( event->keyval )
        {
        case GDK_Left:  case GDK_KP_Left:
            if( curPos>0 )
                SetPointer(curPos - 1, trk);
            break;
        case GDK_Right: case GDK_KP_Right:
            SetPointer(curPos + 1, trk);
            break;
        case GDK_Page_Up:    case GDK_KP_Page_Up:
            SetPointerAtDVDMark(trk, true);
            break;
        case GDK_Page_Down:  case GDK_KP_Page_Down:
            SetPointerAtDVDMark(trk, false);
            break;
        case GDK_Home:  case GDK_KP_Home:
            SetPointer(0, trk);
            break;
        case GDK_End:  case GDK_KP_End:
            {
                int last_pos = LastPos(trk);
                SetPointer( std::max(0, last_pos), trk );
            }
            break;
        case GDK_Insert:  case GDK_KP_Insert:
            InsertDVDMark(trk);
            break;
        case GDK_Delete:  case GDK_KP_Delete:
            DeleteDVDMarkUnderPointer(trk);
            break;
        default:
            break;
        }
    }
}

void MoverTL::OnMouseUp(TrackLayout& trk, GdkEventButton* event)
{
    if( !IsLeftButton(event) )
        return;

    OnDragEnd(trk);
    ChangeState(trk, NormalTL::Instance());
}

void MoverTL::SetCursorPos(TrackLayout& trk, const DPoint& phis_pos, const DPoint& user_pos)
{
    Data& dat = trk;
    dat.phisCoord = phis_pos;
    dat.userCoord = user_pos;
}

static int GetFramePos(double pos_x, TrackLayout& trk)
{
    // ограничения
    // :TODO: сделать "передвигаемое" перемещение
    // (=> правильную проверку на сдвиг вправо)
    pos_x = std::max(pos_x, 0.); // (double)trk.GetShift().x
    return Round(pos_x / trk.FrameScale());
}

int MoverTL::CalcFrame(double pos_x, TrackLayout& trk)
{
    Data& dat = trk;
    return GetFramePos(trk.FramesSz(dat.startPos) + (pos_x - dat.phisCoord.x), trk);
}

void LeftMouseHook::Process()
{
    if( actStt )
    {
        SetCursorForWdg(trkLay, 0);
        actStt->BeginState(trkLay);
    }
}

bool SetPointer(int new_pos, TrackLayout& trk)
{
    PointerCoverSvc svc(trk);
    svc.FormLayout();

    trk.SetPos(new_pos);

    svc.FormLayout();
    return true;
}

void PointerMoverTL::OnMouseMove(TrackLayout& trk, GdkEventMotion* event)
{
    SetPointer(CalcFrame(event->x, trk), trk);
}

void PointerMoverTL::BeginState(TrackLayout& trk)
{
    Data& dat = trk;
    if( dat.atPointer )
        dat.startPos = trk.CurPos();
    else
    {
        dat.startPos = GetFramePos(dat.userCoord.x+trk.GetShift().x, trk);
        SetPointer(CalcFrame(dat.phisCoord.x, trk), trk);
    }
    ChangeState(trk, *this);
}

void DVDLabelMoverTL::SetDVDIdx(TrackLayout& trk, int idx)
{
    Data& dat    = trk;
    dat.dvdIdx   = idx;
    dat.startPos = GetMarkData(idx).pos;
}

// все элементы должны быть упорядочены, кроме, может быть idx 
static int OrderDVDMark(int idx)
{
    DVDArrType::iterator beg = DVDMarks().begin(), end = DVDMarks().end();
    DVDArrType::iterator it = DVDMarks().begin()+idx;
    ASSERT( it != end );

    DVDArrType::iterator new_it;
    if( it != beg )
    {
        if( GetMarkData(*(it-1)).pos > GetMarkData(*it).pos )
        {
            new_it = std::lower_bound(beg, it, *it, TimelineComparer);
            std::rotate(new_it, it, it+1);
            return new_it-beg;
        }
    }

    if( it != end-1 )
    {
        if( GetMarkData(*it).pos > GetMarkData(*(it+1)).pos )
        {
            new_it = std::lower_bound(it+1, end, *it, TimelineComparer);
            std::rotate(it, it+1, new_it);
            return new_it-beg-1;
        }
    }

    // уже упорядочено
    return idx;
}

static void SetDVDLabel(int new_pos, int& idx, TrackLayout& trk)
{
    DVDLabelCoverSvc svc(trk, idx);
    svc.FormLayout();

    DVDMarkData& mrk = GetMarkData(idx);
    mrk.pos      = new_pos;
    ClearRefPtr(mrk.thumbPix);
    idx = OrderDVDMark(idx);
    svc.SetIndex(idx);

    svc.FormLayout();
}

void DVDLabelMoverTL::OnMouseMove(TrackLayout& trk, GdkEventMotion* event)
{
    Data& dat = trk;
    SetDVDLabel(CalcFrame(event->x, trk), dat.dvdIdx, trk);
}

void DVDLabelMoverTL::BeginState(TrackLayout& trk)
{
    // сохраняем начальное положение главы
    int idx = ((Data&)trk).dvdIdx;
    DVDMarks()[idx]->GetData<int>("ChapterMoveIndex") = idx;

    ChangeState(trk, *this);
}

static double GetTimeByPos(Project::ChapterItem ci, TrackLayout& trk)
{
    return GetMarkData(ci).pos/trk.FrameFPS();
}

void DVDLabelMoverTL::OnDragEnd(TrackLayout& trk)
{
    Data& dat = trk;
    Project::ChapterItem ci = DVDMarks()[dat.dvdIdx];
    double new_time = GetTimeByPos(ci, trk);
    if( new_time != ci->chpTime )
    {
        ci->chpTime = new_time;
        SetDirtyCacheShot(ci);
        Project::InvokeOnChange(ci);
    }
}

class BigLabelLctSvc: public SimpleCalcSvc
{
    typedef SimpleCalcSvc MyParent;
    public:

        Rect blLct;

                    BigLabelLctSvc(TrackLayout& trk_lay)
                        : MyParent(trk_lay) {}

    protected:
        
    virtual   void  Process() { FormBigLabel(); }
    virtual   void  ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos);
};

void BigLabelLctSvc::ProcessBigLabel(RefPtr<Pango::Layout> lay, const Point& pos)
{
    DRect drct(RectASz(DPoint(pos), CalcTextSize(lay)));
    blLct = CeilRect(CR::UserToDevice(cont, drct));
}

Rect GetBigLabelLocation(TrackLayout& trk)
{
    BigLabelLctSvc svc(trk);
    svc.FormLayout();

    return svc.blLct;
}

static bool OnFocusOutBigLabel(TrackLayout& trk) //, GdkEventFocus*)
{
    EditBigLabelTL::Instance().EndState(trk, false);
    return true;
}

static bool OnKeyPressBigLabel(TrackLayout& trk, GdkEventKey* event)
{
    if( event->keyval == GDK_Escape )
        EditBigLabelTL::Instance().EndState(trk, false);
    else if( event->keyval == GDK_Up || event->keyval == GDK_Down )
        EditBigLabelTL::Instance().EndState(trk, true);
    return true;
}

namespace
{

const char* AllowedChars = "0123456789.;:";
const char* DigitChars   = "0123456789";
//const char* SepChars     = ".;:";

static int Ceil(double val)
{
    return int(std::ceil(val));
}

bool TimeToFrames(long& pos, long nums[4], double fps)
{
    const int MAX_HOURS = 500;
    double max_nums[4] = { MAX_HOURS, 60., 60., fps };
    double dpos = 0;

    double mult = 1.0;
    for( int i=4-1; i>=0; mult *= max_nums[i], i-- )
    {
        if( nums[i] >= Ceil(max_nums[i]) )
            return false;
        dpos += nums[i]*mult;
    }
    pos = Ceil(dpos);
    return true;
}

} // namespace


int TimeToFrames(double sec, double fps)
{
    return Ceil(sec*fps);
}

// разбираем "hh:mm:ss;ff" в номер кадра
bool ParsePointerPos(long& pos, const char* str, double fps)
{
    int len = strlen(str);
    if( (int)strspn(str, AllowedChars) != len )
        return false;
    if( (int)strspn(str, DigitChars) == len ) // только цифры => номер кадра
        return Str::GetLong(pos, str);

    // разбор
    long nums[4] = { 0, 0, 0, 0 };
    int num = 0;
    bool frames_end = false;

    bool is_semicolon = false;
    std::string num_str;
    const char *cur = str, *end = str + len;
    for( num=0; num<4 && cur<end; num++ )
    {
        int cnt = strspn(cur, DigitChars);
        num_str.assign(cur, cur+cnt);
        if( !Str::GetLong(nums[num], num_str.c_str()) )
            return false;

        bool is_end  = (cur+cnt == end);
        frames_end   = is_semicolon && is_end;
        is_semicolon = !is_end && (cur[cnt] == ';');
        cur += cnt + 1;
    }

    if( cur < end || num == 0 )
        return false;

    if( frames_end )
        std::rotate(nums, nums+num%4, nums+4);
    else
    {
        // последнее число - секунды
        if( num == 4 )
            nums[0] = 0; // сутки не считаем
        std::rotate(nums, nums+(num+1)%4, nums+4);
    }
    return TimeToFrames(pos, nums, fps);
}

bool TimeToFrames(long& pos, int hh, int mm, int ss, int ff, double fps)
{
    long nums[4] = { hh, mm, ss, ff };
    return TimeToFrames(pos, nums, fps);
}

void EditBigLabelTL::BeginState(TrackLayout& trk)
{
    Data& dat = trk;
    dat.edt = RefPtr<Gtk::Entry>(&NewManaged<Gtk::Entry>());

    Rect lct(GetBigLabelLocation(trk));
    Gtk::Entry& ent = *UnRefPtr(dat.edt);

    // свойства
    ent.set_name("BigLabelEntry");
    ent.property_has_frame() = false;
    ent.set_text(CurPointerStr(trk));

    // положение
    Gtk::Requisition req = ent.size_request();
    Point sz(lct.Width() + req.height-lct.Height()+8, req.height);
    ent.set_size_request(sz.x, sz.y);
    ent.set_alignment(Gtk::ALIGN_CENTER);

    // сигналы
    ent.signal_activate().connect( bb::bind(&EditBigLabelTL::EndState, this, boost::ref(trk), true) );
    ent.signal_focus_out_event().connect( 
        wrap_return<bool>(bb::bind(&OnFocusOutBigLabel, boost::ref(trk))) );
    ent.signal_key_press_event().connect(
        wrap_return<bool>(bb::bind(&OnKeyPressBigLabel, boost::ref(trk), _1)) );
    LimitTextInput(ent, AllowedChars);

    sz = FindAForCenteredRect(sz, lct);
    trk.put(ent, sz.x, sz.y);

    // фокус устанавливаем после вставки, иначе не сработает
    ent.show();
    ent.grab_focus();

    ChangeState(trk, *this);
}

void EditBigLabelTL::EndState(TrackLayout& trk, bool accept)
{
    Data& dat = trk;
    // можем входить рекурсивно, потому используем как признак
    if( !dat.edt )
        return;
    RefPtr<Gtk::Entry> edt = dat.edt;
    ClearRefPtr(dat.edt);

    if( accept )
    {
        long new_pos;
        if( ParsePointerPos(new_pos, edt->get_text().c_str(), trk.FrameFPS()) )
            SetPointer(new_pos, trk);
    }

    trk.remove(*UnRefPtr(edt));
    trk.grab_focus();
    ChangeState(trk, NormalTL::Instance());
}

void EditBigLabelTL::OnMouseDown(TrackLayout& trk, GdkEventButton* event)
{
    // фокус остается на поле ввода, если мы нажимаем на его родителе;
    // в идеале нужно не отслеживать потерю фокуса OnFocusOutBigLabel(),
    // а явно захватить управление мышью,- но это дорогое удовольствие ради такой мелочи
    EndState(trk, false);
    NormalTL::Instance().OnMouseDown(trk, event);
}

static void InsertDVDMarkAtPos(TrackLayout& trk_lay, int pos)
{
    if( pos >= 0 )
    {
        Project::ChapterItem ci = PushBackDVDMark(pos);
        RedrawDVDMark(trk_lay, OrderDVDMark(DVDMarks().size()-1));

        // обновляем
        ci->chpTime = GetTimeByPos(ci, trk_lay);
        Project::InvokeOnInsert(ci);
    }
}

void InsertDVDMark(TrackLayout& trk_lay)
{
    InsertDVDMarkAtPos(trk_lay, trk_lay.CurPos());
}

} // namespace Timeline

