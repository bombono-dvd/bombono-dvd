//
// mdemux/seek.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#include <cmath>  // std::ceil

#include <mlib/string.h>

#include "seek.h"
#include "util.h"

namespace Mpeg {

std::string SecToHMS(double len, bool round_sec)
{
    int min   = (int)floor(len/60.);
    double ss = len - min*60;
    if( round_sec )
        ss = round(ss);
    int hh    = min / 60;
    int mm    = min - hh*60;

    str::stream strss;
    strss << set_hms() << hh << ":"
      << set_hms() << mm << ":" << set_hms() << ss;
    return strss.str();
}

bool MediaInfo::InitBegin(VideoLine& vl)
{
    FrameList& fram_lst = vl.GetFrames();
    if( !fram_lst.IsPlayable() )
    {
        errReason = "Video not found";
        return false;
    }

    begTime = fram_lst.TimeBeg();
    begPos  = 0;
    ASSERT( IsTSValid(begTime) );
    return true;
}

bool MediaInfo::InitEnd(VideoLine& vl)
{
    FrameList& fram_lst = vl.GetFrames();
    if( !fram_lst.IsPlayable() )
    {
        errReason = "Video not found (at end)";
        return false;
    }

    endTime = fram_lst.TimeEnd();
    endPos  = vl.GetParseContext().dmx.ObjStrm().tellg();
    ASSERT( IsTSValid(endTime) );
    return begTime < endTime;
}

bool MediaInfo::TryGetInfo(ParseContext& prs_cont)
{
    VideoLine vl(prs_cont);

    // 0 проверяем, можно ли перемещаться
    io::stream& strm = prs_cont.dmx.ObjStrm();
    if( !strm.is_open() )
    {
        errReason = "File is not open";
        return false;
    }
    if( !strm.good() )
    {
        errReason = "Can't read the file";
        return false;
    }
    if( (io::pos)strm.tellg() == -1 )
    {
        errReason = "File is not seekable";
        return false;
    }

    try
    {
        io::pos sz = StreamSize(strm);
        io::pos work_sz = std::min(sz, (io::pos)BoundDecodeSize);
    
        // 1 читаем первые BoundDecodeSize байт
        prs_cont.dmx.SetStrict(true);
        prs_cont.dcr.Init();
        vl.MakeForPeriod(0, work_sz);
    
        bool at_end = ((io::pos)strm.tellg() == sz);
        vidSeq = prs_cont.dcr.seqInf;
        if( at_end )
        {
            // небольшой файл
            if( !vidSeq.IsInit() )
            {
                errReason = "Not MPEG2 file";
                return false;
            }
            return InitBegin(vl) && InitEnd(vl);
        }
        else if( prs_cont.dmx.IsEnd() )
        {
            errReason = prs_cont.dmx.ErrorReason();
            return false;
        }
        if( !InitBegin(vl) )
            return false;
    
        // 2 читаем конец файла
        prs_cont.dmx.SetStrict(false);
        vl.MakeForPeriod(sz-work_sz, work_sz);
        return InitEnd(vl);
    }
    catch(const std::exception& err)
    {
        errReason = err.what();
    }
    return false;
}

bool MediaInfo::GetInfo(ParseContext& prs_cont)
{
    //ServiceSaver ss(prs_cont.dmx);
    ParseContextSaver ps(prs_cont);
    bool res = TryGetInfo(prs_cont);
    if( res )
    {
        errReason.clear();
        // ровно столько кадров влезает, начиная с нуля
        framesCnt = (int)std::ceil( (endTime - begTime)/FrameLength() );
    }
    else
        Init();
    return res;
}

//////////////////////////////////////////////////////////////////
// SeekService

SeekService::SeekService(Demuxer& dmx)
    : ServiceSaver(dmx)
{
    demuxer.SetService(this);
    demuxer.SetStrict(false);
}

void SeekService::GetData(Demuxer& /*dmx*/, int /*len*/)
{
    if( seekSCR || IsTSValid(demuxer.CurPTS()) )
        foundTS = true;
}

double SeekService::GetTSByPos(io::pos pos, bool seek_scr)
{
    double res_ts = INV_TS;

    foundTS = false;
    seekSCR = seek_scr;
    demuxer.Begin();
    demuxer.ObjStrm().seekg(pos);
    for( ; demuxer.NextState(); )
    {
        if( foundTS )
        {
            res_ts = seekSCR ? demuxer.CurSCR() : demuxer.CurPTS() ;
            ASSERT( IsTSValid(res_ts) );
            break;
        }
    }
    CleanEof(demuxer);

    return res_ts;
}

TSPos SeekService::FindTSPos(double lft_bound, double rgt_bound,
                             TSPos beg, TSPos end, bool& is_good)
{
    is_good = true;
    if( lft_bound <= beg.ts )
        return beg;

    ASSERT( (beg.ts < lft_bound) && (rgt_bound < end.ts) );
    double calc_ts = (lft_bound + rgt_bound)/2.;
    while( end.pos - beg.pos >= 1024 )
    {
        double ratio = (end.pos-beg.pos) / (double)(end.ts-beg.ts);
        io::pos try_pos = beg.pos + io::pos(ratio * (calc_ts-beg.ts));
        double scr = GetTSByPos(try_pos, true);
        if( !IsTSValid(scr) )
        {
            is_good = IsGood(demuxer);
            break;
        }
        // проверка упорядоченности scr
        if( (scr < beg.ts) || (end.ts <= scr) )
        {
            is_good = false;
            break;
        }

        bool is_lft = lft_bound <= scr;
        bool is_rgt = scr < rgt_bound;
        // нашли!
        if( is_lft && is_rgt )
            return TSPos(try_pos, scr);

        if( is_lft )
        {
            end.pos = try_pos;
            end.ts  = scr;
        }
        if( is_rgt )
        {
            beg.pos = try_pos;
            beg.ts  = scr;
        }
    }
    return TSPos();
}

TSPos SeekService::FindForPts(double seek_time, MediaInfo& inf)
{
    if( !inf.IsInit() || (seek_time < inf.begTime) || (seek_time >= inf.endTime) )
        return TSPos();
    double beg_scr = GetTSByPos(inf.begPos, true);
    if( !IsTSValid(beg_scr) || (inf.begTime <= beg_scr) )
        return TSPos();

    // inf.begTime не можем использовать, потому что нам нужно не нарушающее
    // естеств. сортировку scr в потоке
    TSPos beg(inf.begPos, beg_scr);
    TSPos end(inf.endPos, inf.endTime);
    // увеличивая радиус поиска от 0.25 до 60<64, ищем подходящий PTS
    // Doc: System - 2.4.2.6, 2.5.2.3
    bool is_good = true;
    for( double lft_bound = 1., rgt_bound = 0.25; lft_bound<=64.; lft_bound *= 4, rgt_bound *= 4 ) 
    {
        TSPos pair = FindTSPos(seek_time-lft_bound, seek_time-rgt_bound, beg, end, is_good);
        if( !IsTSValid(pair.ts) )
        {
            if( is_good )
                continue;
            else
                break;
        }

        double pts = GetTSByPos(pair.pos, false);
        // нашли!
        if( IsTSValid(pts) && ((pair.pos <= beg.pos) || (pts <= seek_time)) )
            return TSPos(pair.pos, pts);
    }
    return TSPos();
}

TSPos FindForPts(double seek_time, MediaInfo& inf, ParseContext& cont)
{
    SeekService svc(cont.dmx);
    return svc.FindForPts(seek_time, inf);
}

static bool DoMoveForTime(VideoLine& vl, double time)
{
    FrameList& fl      = vl.GetFrames();
    ParseContext& cont = vl.GetParseContext();

    bool done = true;
    for( ; !(time < fl.TimeEnd()); vl.MovePeriod(DecodeBlockSize) )
        if( cont.dmx.IsEnd() )
        {
            done = false;
            break;
        }
    return done;
}

bool MakeForTime(VideoLine& vl, double time, MediaInfo& inf)
{
    // 0 - проверка на вхождение в период
    if( !inf.IsInRange(time) )
        return false;

    // 1 - находим начало, откуда можно проиграть time
    ParseContext& cont = vl.GetParseContext();
    FrameList& fl      = vl.GetFrames();
    bool done          = false;
    // считаем, что I-кадры появляются, в среднем, через 13 кадров
    // а это около 0.5 секунд
    double shift = 0.5; 
    const double max_diff = 8.0;
    for( double seek_time; seek_time=time-shift, shift<=max_diff; shift *= 2 )
    {
        seek_time = std::max(inf.begTime, seek_time);
        // ищем по scr+pts позицию, с которой работать
        TSPos tp = FindForPts(seek_time, inf, cont);
        if( !IsTSValid(tp.ts) )
            return false;

        ASSERT( tp.pos >= 0 );
        // в случае чтения с самого начала первым может быть не первый по времени
        // кадр (с pts); пример - I-кадр в IBBPBB... => BBIBBP...
        ASSERT( (tp.pos == inf.begPos) || (tp.ts <= seek_time) );
        // если уж pts выдали слишком ранний, то убираем холостые циклы 
        double real_shift = time - tp.ts;
        while( shift*2 < real_shift )
            shift *= 2;

        for( vl.MakeForPeriod(tp.pos, DecodeBlockSize); !fl.IsPlayable() ; vl.MovePeriod(DecodeBlockSize) )
            if( cont.dmx.IsEnd() )
                break;

        if( fl.IsPlayable() && fl.TimeBeg() <= time )
        {
            done = true;
            break;
        }
    }

    // не нашли I<time (слишком редко встречаются?)
    if( !done )
        return false;

    // фиксируем начальное положение списка fl, чтоб не укоротили
    FrameList::Itr fix_itr(fl.Beg());
    return DoMoveForTime(vl, time);
}

bool MoveForTime(VideoLine& vl, double time, MediaInfo& inf)
{
    FrameList& fl = vl.GetFrames();
    // 0 - проверка на вхождение в период
    if( !( inf.IsInRange(time) && fl.IsPlayable() && (fl.TimeBeg() <= time) ) )
        return false;

    return DoMoveForTime(vl, time);
}

bool ContinueForTime(VideoLine& vl, double time, MediaInfo& inf)
{
    FrameList& fl = vl.GetFrames();
    FrameList::Itr fix_itr(fl.Beg());

    return MoveForTime(vl, time, inf);
}


} // namespace Mpeg

