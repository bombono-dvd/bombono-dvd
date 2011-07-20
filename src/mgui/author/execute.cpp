//
// mgui/author/execute.cpp
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

#include "execute.h"
#include "output.h" // Project::FillAuthorLabel()
#include "script.h" // Project::ForeachVideo()

#include <mgui/win_utils.h>
#include <mgui/gettext.h>
#include <mgui/sdk/textview.h>
#include <mgui/project/handler.h> // FFmpegSizeForDVD()
#include <mgui/project/video.h> // AllVideos()

namespace Author
{

void ExecState::Init()
{
    Set(false);
    mode      = modFOLDER;
    detailsView.set_editable(false);
    Clean();

    // чтобы сбросить старые обработчики
    RenewPtr(execBtn);
}

void ExecState::Clean()
{
    eDat.Init(); 
    detailsView.get_buffer()->set_text("");

    SetStatus();
    SetIndicator(0);
}

Gtk::Label& ExecState::SetStatus(const std::string& name)
{
    return Project::FillAuthorLabel(prgLabel, _("Status: ") + name);
}

static void InitFoundStageTag(RefPtr<Gtk::TextTag> tag)
{
    tag->property_foreground() = "darkgreen";
}

re::pattern DVDAuthorRE(RG_CMD_BEG"dvdauthor"RG_EW ".*-x"RG_EW RG_SPS RG_BW"DVDAuthor\\.xml"RG_EW);
re::pattern MkIsoFsRE(RG_CMD_BEG"mkisofs"RG_EW ".*-dvd-video"RG_EW ".*>" RG_SPS RG_BW"dvd.iso"RG_EW);
re::pattern GrowIsoFsRE(RG_CMD_BEG"growisofs"RG_EW ".*-dvd-compat"RG_EW ".*-dvd-video"RG_EW); 

//static void PrintMatchResults(const re::match_results& what)
//{
//    for( int i=1; i<(int)what.size(); i++ )
//        io::cout << "what[" << i << "] = \"" <<  what.str(i) << "\""<< io::endl;
//}

static bool ApplyStage(const std::string& line, const re::pattern& re, 
                       Stage stg, const TextIterRange& tir, OutputFilter& of)
{
    bool res = re::search(line, re);
    if( res )
    {
        ApplyTag(of.GetTV(), tir, "Stage", InitFoundStageTag);
        of.SetStage(stg);
    }
    return res;
}

class MkIsoFsPP: public ProgressParser
{
    typedef ProgressParser MyParent;
    public:
                  MkIsoFsPP(OutputFilter& of_): MyParent(of_) {}
    virtual void  Filter(const std::string& line);
};

re::pattern MkIsoFsPercent_RE( RG_FLT"?% done");

void MkIsoFsPP::Filter(const std::string& line)
{
    re::match_results what;
    if( re::search(line.begin(), line.end(), what, MkIsoFsPercent_RE) )
    {
        ASSERT( what[1].matched );
        double val;
        if( ExtractDouble(val, what) )
            of.SetProgress(val);
    }
}

const double DVDAuthorRel = 0.9; // 90% времени на VOBU-этап
// расчет индикатора строится на предположениях:
// - 1 titleset, потому что по каждому проходит пара этапов "VOBU"/"fixing"
// - размер dvd_sz равен сумме всех исходников, влкючая меню
class DVDAuthorPP: public ProgressParser
{
    typedef ProgressParser MyParent;
    public:
                  // размер в Mb
                  DVDAuthorPP(OutputFilter& of_, int dvd_sz)
                    : MyParent(of_), dvdSz(dvd_sz), fixStage(false) {}

    virtual void  Filter(const std::string& line);
    protected:
             int  dvdSz;
            bool  fixStage;
};

re::pattern DVDAuthorVOB_RE( "^STAT: VOBU "RG_NUM" at "RG_NUM"MB"); 
re::pattern DVDAuthorFix_RE( "^STAT: fixing VOBU at "RG_NUM"MB \\("RG_NUM"/"RG_NUM", "RG_NUM"%\\)"); 

void DVDAuthorPP::Filter(const std::string& line)
{
    double p = 0.;

    re::match_results what;
    bool is_fix = re::search(line.begin(), line.end(), what, DVDAuthorFix_RE);
    if( !(fixStage || is_fix) )
    {
        if( re::search(line.begin(), line.end(), what, DVDAuthorVOB_RE) )
        {
            int sz = Project::AsInt(what, 2);
            p = sz/(double)dvdSz * DVDAuthorRel;
        }
    }
    else
    {
        fixStage = true;
        if( is_fix )
        {
            p = Project::AsInt(what, 4)/100.;
            p = DVDAuthorRel + p*(1-DVDAuthorRel);
        }
    }

    p *= 100.;
    if( p )
        of.SetProgress(p);
    
    static re::pattern ch_error_re("ERR:.*Cannot jump to chapter "RG_NUM" of title "RG_NUM", only "RG_NUM" exist");
    if( re::search(line, what, ch_error_re) )
    {
        std::string& err_str = of.firstError;
        if( !err_str.size() )
        {
            int tnum = Project::AsInt(what, 2);
            boost_foreach( Project::VideoItem vi, Project::AllVideos() )
                if( Project::GetAuthorNumber(vi) == tnum )
                {
                    // находим первую, самую короткую главу - проблема в ней
                    Project::ChapterItem prev_ci;
                    Project::ChapterItem res_ci;
                    double duration = -1;
                    boost_foreach( Project::ChapterItem ci, vi->List() )
                    {
                        if( prev_ci )
                        {
                            double dur = ci->chpTime - prev_ci->chpTime;
                            if( (duration == -1) || (dur < duration) )
                            {
                                duration = dur;
                                res_ci = prev_ci;
                            }
                        }
                        prev_ci = ci;
                    }
                    ASSERT_RTL( res_ci );
                    err_str = BF_("chapter \"%1%\" in \"%2%\" is too short (%3% sec)")
                        % res_ci->mdName % vi->mdName % duration % bf::stop;
                    
                    break;
                }
        }
    }
}

void OutputFilter::SetParser(ProgressParser* pp)
{
    curParser = pp;
    OnSetParser();
}

void OutputFilter::OnGetLine(const char* dat, int sz, bool is_out)
{
    Gtk::TextView& txt_view = GetTV();
    std::string line(dat, sz);
    TextIterRange tir = AppendNewText(txt_view, line, is_out);

    using namespace Author;
    if( ApplyStage(line, DVDAuthorRE, stDVDAUTHOR, tir, *this) )
        SetParser(new DVDAuthorPP(*this, GetDVDSize()));
    if( ApplyStage(line, MkIsoFsRE,   stMK_ISO, tir, *this) ||
        ApplyStage(line, GrowIsoFsRE, stBURN,   tir, *this) )
        SetParser(new MkIsoFsPP(*this));

    if( curParser )
        curParser->Filter(line);
}

void BuildDvdOF::SetStage(Stage stg)
{
    ::Author::SetStage(stg);
}

static bool MenuSize(Project::Menu mn, io::pos& sz)
{
    if( Project::IsMotion(mn) )
        sz += FFmpegSizeForDVD(Project::MenuDuration(mn));
    return true;
}

io::pos MenusSize()
{
    io::pos sz = 0;
    Project::ForeachMenu(bb::bind(&MenuSize, _1, b::ref(sz)));
    return sz;
}

int BuildDvdOF::GetDVDSize()
{
    return Round(Project::ProjectSizeSum() / (double)(1024*1024));
}

void BuildDvdOF::SetProgress(double percent)
{
    SetStageProgress(percent, true);
}

//static void PrintRect(const Gdk::Rectangle& rct)
//{
//    io::cout.precision(8);
//    io::cout << rct.get_x() << " " << rct.get_y() << " " << rct.get_width() << " " << rct.get_height() << io::endl;
//}

} // namespace Author

