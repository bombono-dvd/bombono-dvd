//
// mdemux/videoline.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2010 Ilya Murav'jov
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

#include "videoline.h"
#include "util.h"       // StreamPosSaver

#include <mlib/sdk/logger.h>

#include <iterator>     // back_inserter
#include <algorithm>    // lower_bound
#include <stdexcept>    // std::runtime_error()

namespace Mpeg {

void ParseContext::Init()
{
    io::stream& strm = dmx.ObjStrm();
    if( strm.is_open() )
        strm.close();
    strm.clear();

    dmx.Begin();
    dcr.Init();
}

const FLIterator FLIterator::None;

FLIterator::FLIterator(): owner(0), pos(-1) 
{}

FLIterator::FLIterator(const FLIterator& it)
    : pos(it.pos)
{
    SetOwner(it.owner);
}

FLIterator::FLIterator(int p, FrameList* own)
    : pos(p)
{
    SetOwner(own);
}

FLIterator::~FLIterator()
{
    SetNone();
}

bool FLIterator::IsNone() const
{ return pos == -1; }

void FLIterator::SetOwner(FrameList* own)
{
    owner = own;
    if( owner )
        owner->Bind(this);
}

void FLIterator::SetNone()
{
    if( owner )
        owner->UnBind(this);
    pos = -1;
}

FLIterator& FLIterator::operator =(const FLIterator& it)
{
    if( owner != it.owner )
    {
        SetNone();
        SetOwner(it.owner);
    }
    pos = it.pos;
    return *this;
}

FLIterator::Ref FLIterator::operator*() const
{
    return owner->At(pos);
}

FLIterator::Pointer FLIterator::operator->() const
{
    return &owner->At(pos);
}

FLIterator::Ref FLIterator::operator[](DiffType n) const
{
    return *(*this + n);
}

// 
// FLState
// 

void FLState::ChangeUpdateState(FLState& stt, FrameList& flist)
{
    flist.flStt = &stt;
    stt.UpdateState(flist);
}

void RawFL::UpdateState(FrameList& flist)
{
    // удаляем все до первого I-кадра, как невозможное проиграть
    int pos = flist.FindIFrame(0, true);
    if( pos != -1 )
    {
        flist.CutByIFrame(pos);

        ChangeUpdateState(IRoundedFL::Instance(), flist);
    }
}

void IRoundedFL::UpdateState(FrameList& flist)
{
    ASSERT( flist.IsInit() );
    // на последний кадр (!=B) рассчитывать нельзя,
    // так как физически следующие за ним B поменяют его порядок
    if( flist.isEOF || (flist.idxLst.back() != 0) )
        ChangeUpdateState(PlayableFL::Instance(), flist);
}

void PlayableFL::CutOverLimit(FrameList& flist)
{
    Data& dat = flist;
    if( dat.firstCutTry )
    {
        // в первый раз (только) даем возможность закрепить клиентам playBeg
        dat.firstCutTry = false;
        return;
    }

    FrameList::IndexList idx_lst = flist.idxLst;
    if( idx_lst.size() > (unsigned int)MaxFrameListLength )
    {
        // минимум на M/10 = 40 кадров обрубаем
        int cut_pos = std::max(MaxFrameListLength/10, int(idx_lst.size() - MaxFrameListLength));

        for( FrameList::IterList::iterator it = flist.iterLst.begin(), end = flist.iterLst.end(); 
             it != end; ++it )
        {
            FrameList::Itr& itr = **it;
            cut_pos = std::min(cut_pos, itr.Pos());
        }

        cut_pos = flist.FindIFrame(cut_pos, false);
        if( cut_pos != -1 )
            flist.CutByIFrame(cut_pos);
    }
}

void PlayableFL::UpdateState(FrameList& flist)
{
    // обнуляем устаревшие значения начала и конца
    flist.playBeg.SetNone();
    flist.playEnd.SetNone();

    // удаляем все кадры сверх лимита MaxFrameListLength
    CutOverLimit(flist);
}

void PlayableFL::Clear(FrameList& flist)
{
    FrameList::IterList& iter_lst = flist.iterLst;
    // так удаляем все итераторы
    while( !iter_lst.empty() )
        iter_lst.back()->SetNone();

    Data& dat = flist;
    dat.firstCutTry = true;
}

void PlayableFL::MoveIterator(int old_pos, int new_pos, FrameList& flist)
{
    // подправляем позиции всех итераторов, указывающих на old_pos -> new_pos
    if( (old_pos != -1) && (new_pos != old_pos) )
    {
        ASSERT( new_pos != -1 );
        for( FrameList::IterList::iterator l_it = flist.iterLst.begin(), end = flist.iterLst.end();
             l_it != end; ++l_it )
        {
            FrameList::Itr& itr = **l_it;
            if( itr.Pos() == old_pos )
                itr += new_pos-old_pos;
        }
    }
}

void PlayableFL::ShiftIterators(int shift, FrameList& flist)
{
    for( FrameList::IterList::iterator it = flist.iterLst.begin(), end = flist.iterLst.end();
         it != end; ++it )
    {
        FrameList::Itr& itr = **it;

        itr -= shift;
        ASSERT( itr.Pos() >= 0 );
    }
}

// 
// FrameList
// 

void FrameList::Init()   
{ 
    timeShift = 0.;
    clear();
    idxLst.clear();

    if( flStt )
        flStt->Clear(*this);
    flStt = &RawFL::Instance();
}

void FrameList::Setup( SequenceData& seq )
{
    // смена последовательности - плохой признак
    if( IsInit() )
    {
        // DVBcut_seq_change.mpg - смена с прогрессивной на чересстрочную (DVBcut разрешает
        // соединять подобное без перекодирования, что само по себе нехорошо);
        // TMPG Author принял молча, без возражений => выдаем только предупреждение

        // :TODO: размеры, sar и частота кадров - слишком чувствительны для смены =>
        // (запрещать в VideoLine::TagData(), собственно пред. prevSeq хранить в Decoder)
        LOG_WRN << "Sequence change: (wdh, hgt) = " << Point(seq.wdh, seq.hgt) 
            << "; sample aspect ratio = " << seq.sarCode 
            << "; frame rate = " << seq.framRat 
            << "; vbvBufSz = " << seq.vbvBufSz 
            << "; profile = " << seq.plId
            << "; is progressive = " << seq.isProgr
            << "; chroma format = " << seq.chromaFrmt
            << "; lowDelay = " << seq.lowDelay
            << io::endl;
    }

    if( seq.IsInit() )
        timeShift = seq.framRat.y/(double)seq.framRat.x;
}

void FrameList::Bind(Itr* it)
{
    it->itrPos = iterLst.insert(iterLst.end(), it);
    it->owner  = this;
}

void FrameList::UnBind(Itr* it)
{
    ASSERT( it->owner == this );

    iterLst.erase(it->itrPos);
    it->itrPos = IterList::iterator();
    it->owner  = 0;
}

static void PushNonB(FrameList::IndexList& idxLst, int last_non_b, int& new_pos)
{
    if( last_non_b != -1 )
    {
        idxLst.push_back(last_non_b);
        if( new_pos == -1 )
            new_pos = idxLst.size()-1;
    }
}

int FrameList::UpdateIndex()
{
    int beg = idxLst.size();
    int sz  = PhisSize();
    ASSERT( sz>=beg );

    int old_pos = beg-1; // старая и новая позиция послед. не-B-кадра
    int new_pos = -1;

    int last_non_b = -1; // индекс последнего не B-кадра
    if( beg != 0 )
    {
        int idx = idxLst.back();
        FrameData& dat = PhisAt(idx);
        if( dat.typ != ptB_FRAME )
        {
            last_non_b = idx;
            idxLst.pop_back();
        }
        else
            old_pos = -1;
    }

    // не B-кадр должен пропустить перед собой все B-кадры
    for( int i=beg; i<sz; i++ )
    {
        FrameData& dat = PhisAt(i);
        if( dat.typ == ptB_FRAME )
            idxLst.push_back(i);
        else
        {
            PushNonB(idxLst, last_non_b, new_pos);
            last_non_b = i;
        }
    }
    PushNonB(idxLst, last_non_b, new_pos);

    flStt->MoveIterator(old_pos, new_pos, *this);

    return beg ? beg-1 : 0 ;
}

void FrameList::UpdateTimeIndex(bool is_eof)
{
    LOG_DBG << "Begin UpdateTimeIndex()" << io::endl;

    isEOF = is_eof;
    if( !IsInit() )
        return;

    double cur_pts = INV_TS;
    char prev_len  = 2;

    // 0 обновляем индекс
    int beg = UpdateIndex();
    if( beg )
    {
        FrameData& calced_frame = At(beg-1);
        if( IsTSValid(calced_frame.pts) )
        {
            cur_pts  = calced_frame.pts;
            prev_len = calced_frame.len;
        }

        LOG_DBG << "begin with (beg, raw_pos, pts, len, typ): " << beg-1 << ", " << idxLst.at(beg-1) << ", "
                   << cur_pts << ", " << (int)prev_len << ", " << calced_frame.typ << io::endl;
    }

    // 1 время
    for( int i=beg, sz=idxLst.size(); i<sz; i++ )
    {
        FrameData& frame = At(i);
        if( frame.opt&fdDEF_PTS )
        {
            ASSERT( IsTSValid(frame.pts) );
            if( IsTSValid(cur_pts) && frame.pts<cur_pts )
                // плохо записанное видео может иметь испорчен. pts,-
                // приходится упорядочивать вручную
                frame.pts = cur_pts;
            else
                cur_pts = frame.pts;
        }
        else if( IsTSValid(cur_pts) )
        {
            cur_pts  += FrameDuration(timeShift, prev_len);
            frame.pts = cur_pts;
        }
        prev_len = frame.len;

        LOG_DBG << "frame :" << i << ", " << idxLst.at(i) << ", " 
                   << cur_pts << ", " << (int)prev_len << ", " << frame.typ << io::endl;
    }

    // 2 состояние
    flStt->UpdateState(*this);

    LOG_DBG << "End UpdateTimeIndex()" << io::endl;
}

// bool FrameList::IsIRounded()
// {
//     return idxLst.size() && (PhisAt(0).typ == ptI_FRAME);
// }

// bool FrameList::IsPlayable()
// {
//     if( IsIRounded() )
//     {
//         ASSERT( IsInit() );
//         // на последний кадр (!=B) рассчитывать нельзя,
//         // так как физически следующие за ним B поменяют его порядок
//         if( isEOF || (idxLst.back() != 0) )
//             return true;
//     }
//     return false;
// }

bool FrameList::IsPlayable()
{
    return flStt->IsPlayable();
}

const FrameList::Itr& FrameList::Beg()
{
    ASSERT( flStt->IsPlayable() );
    if( !playBeg.IsNone() )
        return playBeg;

    FrameData& i_frame = PhisAt(0);
    int log_pos = -1;
    if( !(i_frame.opt&fdGOP_CLOSED) )
        log_pos = FindIFrame(0, true);
    else
    { 
        // впереди есть B-кадры, которые можно декодировать
        for( int i=0, sz=idxLst.size(); i<sz; i++ )
        {
            int pos = idxLst.at(i);
            FrameData& frame = PhisAt(pos);

            if( IsTSValid(frame.pts) )
            {
                log_pos = i;
                break;
            }
        }
    }

    ASSERT( log_pos != -1 );
    playBeg = Itr(log_pos, this);

    return playBeg;
}

const FrameList::Itr& FrameList::End()
{
    ASSERT( flStt->IsPlayable() );
    if( !playEnd.IsNone() )
        return playEnd;

    int log_pos = idxLst.size();
    if( !isEOF ) // причину см. в IsPlayable()
        log_pos--;

    ASSERT( log_pos != -1 );
    playEnd = Itr(log_pos, this);

    return playEnd;
}

FrameList::Itr FrameList::DataEnd()
{ 
    return Itr(idxLst.size(), this); 
}

double FrameList::TimeBeg()
{
    double pts = Beg()->pts;
    ASSERT( IsTSValid(pts) );

    return pts;
}

double FrameList::TimeEnd()
{
    FrameData& last_frame = *(End()-1);
    double pts = last_frame.pts;
    ASSERT( IsTSValid(pts) );

    if( isEOF )
        pts += FrameDuration(timeShift, last_frame.len);
    return pts;
}

struct LessFrameTime
{
    FrameList& fLst;
       double  targetTime;

            LessFrameTime(FrameList& fl, double time): fLst(fl), targetTime(time)
            {}
      bool  operator ()(const int lft, const int rgt);
};

bool LessFrameTime::operator ()(const int lft, const int rgt)
{
    double lft_time = (lft == -1) ? targetTime : fLst.PhisAt(lft).pts;
    double rgt_time = (rgt == -1) ? targetTime : fLst.PhisAt(rgt).pts;
    return lft_time < rgt_time;
}

FrameList::Itr FrameList::PosByTime(double time)
{
    LOG_DBG << "FrameList::PosByTime with time: " << time << io::endl;

    int pos = -1;
    int beg_pos = Beg().Pos();
    int end_pos = End().Pos();
    ASSERT( beg_pos != end_pos );

    IndexList::iterator beg = idxLst.begin() + beg_pos;
    IndexList::iterator end = idxLst.begin() + end_pos;

    IndexList::iterator it = std::lower_bound(beg, end, -1, LessFrameTime(*this, time) );
    if( it == end )
        --it;
    else
    {
        double pts_time = PhisAt(*it).pts;
        if( pts_time != time )
        {
            ASSERT( (time < pts_time) && (it != beg) );
            --it;
        }
    }
    pos = it-idxLst.begin();

    if( LogFilter->IsEnabled(::Log::Debug) )
    {
        std::stringstream ss_log;
        ss_log << ::Log::Prepender("FrameList values: ");
        for( int i=beg_pos; i<end_pos; i++ )
            ss_log << At(i).pts << " ";

        LOG_DBG << ss_log.str() << io::endl;
        LOG_DBG << "PosByTime returns: " << pos << "; range is " 
                   << At(beg_pos).pts << " - " << At(end_pos-1).pts << io::endl;
    }

    return Itr(pos, this);
}

// получить указатель за конец последнего полного (!) кадра
FrameList::iterator FrameList::PhisEnd()
{
    iterator be = begin(), en = end();
    if( be == en ) // пусто
        return en;
    return (en-1)->IsFull() ? en : en-1;
}

int FrameList::FindIFrame(int log_pos, bool is_fwd)
{
    int res = -1;
    if( is_fwd )
    {
        for( int i=log_pos, sz=idxLst.size(); i<sz; i++ )
            if( At(i).typ == ptI_FRAME )
            {
                res = i;
                break;
            }
    }
    else
    {
        for( int i=log_pos; i>=0; i-- )
            if( At(i).typ == ptI_FRAME )
            {
                res = i;
                break;
            }
    }
    return res;
}

void FrameList::CutByIFrame(int log_pos)
{
    // в случае физ. IB...B = лог. B...BI
    int phis_pos = idxLst.at(log_pos);
    ASSERT( (log_pos == phis_pos) || (idxLst.at(phis_pos) == phis_pos+1) );
    if( phis_pos )
    {
        erase(begin(), begin()+phis_pos);
        // отрезаем индекс и уменьшаем оставшиеся значения, так как ведущий
        // I-кадр изменился
        idxLst.erase(idxLst.begin(), idxLst.begin()+phis_pos);
        for( IndexList::iterator it = idxLst.begin(), end = idxLst.end(); it != end; ++it )
            *it -= phis_pos;

        flStt->ShiftIterators(phis_pos, *this);
    }
}

////////////////////////////////////////////////////////////////////////
// VideoLine

VideoLine::VideoLine(ParseContext& cont)
    : prsCont(cont), errStt(false)
{
    prsCont.dmx.SetService(this);
    prsCont.dcr.SetService(this);
}

VideoLine::~VideoLine()
{
    if( prsCont.dmx.GetService() == this )
        prsCont.dmx.SetService(0);
    if( prsCont.dcr.GetService() == this )
        prsCont.dcr.SetService(0);
}

void VideoLine::Clear()
{
    prsCont.dmx.Begin();
    prsCont.dcr.Begin(0); // начинаем с 0, как в AddChunk() первый блок 

    prevPts = INV_TS;
    chkLst.clear();
    chkOptLst.clear();

    framLst.Init();
    framLst.Setup(prsCont.dcr.seqInf);
}

void VideoLine::AddChunk(io::pos ext_pos, int len, double pts)
{
    io::pos int_pos = 0;
    if( !chkLst.empty() )
    {
        Chunk& chk = chkLst.back();
        ChunkOpt& chk_opt = chkOptLst.back();

        int_pos = chk_opt.intPos + chk.len;
    }

    Chunk chk(ext_pos, len);
    chkLst.push_back(chk);

    ChunkOpt chk_opt(int_pos, pts);
    chkOptLst.push_back(chk_opt);

    //return int_pos;
}

void VideoLine::RemoveChunks(int cnt)
{
    chkLst.erase(chkLst.begin(), chkLst.begin()+cnt);
    chkOptLst.erase(chkOptLst.begin(), chkOptLst.begin()+cnt);
}

void VideoLine::GetData(Mpeg::Demuxer& dmx, int len)
{
    double pts = dmx.CurPTS();
    double scr = dmx.CurSCR();
    ASSERT_OR_UNUSED_VAR( IsTSValid(scr), scr );

    // пока не дошли до блока с pts не начинаем
    if( !IsTSValid(pts) )
        return;

    io::stream& strm = dmx.ObjStrm();
    AddChunk(strm.tellg(), len, pts != prevPts ? pts : INV_TS );
    prevPts = pts;

    Decoder& dcr = prsCont.dcr;
    dcr.FeedFromStream(strm, len);
    while( dcr.NextState() != dtBUFFER )
        ;
}

bool LessChunkOpt(const ChunkOpt& co1, const ChunkOpt& co2)
{
    return co1.intPos < co2.intPos;
}

void VideoLine::FillFrameData(FrameData::ChunkList* chk_lst, io::pos int_pos)
{
    ChunkOpt co(int_pos, INV_TS);
    ChunkOptList::iterator beg = chkOptLst.begin(), end = chkOptLst.end(); 

    ChunkOptList::iterator iter = std::lower_bound(beg, end, co, LessChunkOpt);
    int cnt = iter-beg;
    ASSERT( cnt>=1 ); // кадр не может быть пустым

    // 1 - заполняем кадр
    if( (iter != end) && iter->intPos == int_pos )
    {
        // новый кадр выровнен по границе блока iter
        if( chk_lst )
            std::copy(chkLst.begin(), chkLst.begin()+cnt, std::back_inserter(*chk_lst));
    }
    else
    {
        if( chk_lst )
            std::copy(chkLst.begin(), chkLst.begin()+(cnt-1), std::back_inserter(*chk_lst));

        Chunk&    chk     = chkLst[cnt-1];
        ChunkOpt& chk_opt = chkOptLst[cnt-1];

        int rest_len = int_pos - chk_opt.intPos;
        ASSERT( rest_len>0 );

        if( chk_lst )
        {
            Chunk end_chk(chk.extPos, rest_len);
            chk_lst->push_back(end_chk);
        }

        //
        // Проверяем синхронизацию chk(Opt)Lst и декодера (int_pos)
        // Если int_pos (rest_len) больше чем реально данных в chk(Opt)Lst,
        // то значит мы запросили для кадра больше данных, чем прочитали => рассинхронизация
        // 
        // Единственный вариант, когда это сейчас возможно,- в процессе обработки ранее этого
        // момента полностью очищали данные в RemoveChunks(), из-за чего AddChunk() обнуляет
        // внутреннюю нумерацию; последнее происходит при наличие "видео"-окончаний внутри
        // потока (слайдшоу и т.д.). Равенство chk.len == rest_len допускается, например,
        // при vtEND мы можем быть в конце блока.
        // 
        // См. проблематику в "непросматриваемый пример" в разделе "Демиксер MPEG1,2".
        // 
        
        //ASSERT( chk.len>=rest_len );
        if( chk.len < rest_len )
            throw std::runtime_error("Multiple video ends");
        if( chk.len-rest_len>0 )
        {
            chk.extPos     += rest_len;
            chk.len        -= rest_len;
            chk_opt.intPos += rest_len;

            cnt--;
        }
    }
    // 2 удаляем 
    RemoveChunks(cnt);
}

// если были в ошибочном состоянии, то
// кадр пропадает,- начинаем с нового
void VideoLine::ClearErrorState(FrameData& fd, io::pos int_pos)
{
    if( errStt )
    {
        errStt = false;
        fd.Init();

        FillFrameData(0, int_pos);
    }
}

void VideoLine::FillFrame(FrameData& fd, io::pos int_pos)
{
    // 1 формируем pts
    fd.pts = chkOptLst[0].pts;
    if( IsTSValid(fd.pts) )
      fd.opt |= fdDEF_PTS;

    // 2 - заполняем кадр
    FillFrameData(&fd.dat, int_pos);
}

static void Fill(FrameData& dat, Picture& pic)
{
    dat.typ = pic.type;
    dat.len = pic.len;
}

FrameData& VideoLine::MakeGetCurFrame(VideoTag tag)
{
    Decoder& dcr = prsCont.dcr;
    FrameListCont& frame_cont = framLst.GetContainer();

    int cnt = frame_cont.size();
    if( !cnt )
        frame_cont.push_back( FrameData() );

    FrameData& fd = frame_cont.back();
    io::pos int_pos = dcr.DatPosForTag(tag);//dcr.DatPos();
    ClearErrorState(fd, int_pos);

    if( fd.IsInit() )
    {
        // заполняем кадр до тек. позиции и создаем новый
        FillFrame(fd, int_pos);

        frame_cont.push_back( FrameData() );
    }

    FrameData& fram = frame_cont.back();
    if( tag == vtFRAME_FOUND )
        Fill(fram, dcr.pic);

    return fram;
}

void VideoLine::FinishFrame()
{
    FrameListCont& frame_cont = framLst.GetContainer();
    ASSERT( !frame_cont.empty() );
    FrameData& fd = frame_cont.back();
    io::pos int_pos = prsCont.dcr.DatPos(); // не включая код окончания

    if( errStt )
        ClearErrorState(fd, int_pos);
    else if( fd.IsInit() )
    {
        FillFrame(fd, int_pos);

        // чтоб след. MakeGetCurFrame не продолжил заполнять этот
        frame_cont.push_back( FrameData() );
    }
    else
    {
        // ошибок нет, кадр не инициализирован и найдено окончание - не может быть
        ASSERT(0);
    }

    // удаляем код окончания
    FillFrameData(0, int_pos+4);
}

void VideoLine::TagData(Decoder&, VideoTag tag)
{
    switch( tag )
    {
    case vtHEADER:
        {
            FrameData& frm = MakeGetCurFrame(tag);
            frm.opt |= fdHEADER;
        }
        break;
    case vtGOP:
        {
            FrameData& frm = MakeGetCurFrame(tag);
            frm.opt |= fdGOP;
            if( prsCont.dcr.isGOPClosed )
                frm.opt |= fdGOP_CLOSED;
        }
        break;
    case vtFRAME_FOUND:
        MakeGetCurFrame(tag);
        break;
    case vtHEADER_COMPLETE:
        {
            framLst.Setup( prsCont.dcr.seqInf );

            io::stream& strm = prsCont.dmx.ObjStrm();
            MpegDecodec& m2d = prsCont.m2d;

            // инициализируем распаковщик
            StreamPosSaver sps(strm);
            m2d.Init(true);
            for( ChunkList::iterator it = chkLst.begin(), end = chkLst.end();
                 (it != end) && !m2d.IsInit(); ++it )
                m2d.ReadForInit(*it, strm);
            ASSERT( m2d.IsInit() );
        }
        break;
    case vtERROR:
        errStt = true;
        break;
    case vtEND:
        FinishFrame();
        break;
    default:
        break;
    }
}

void CleanEof(Demuxer& dmx)
{
    ::CleanEof(dmx.ObjStrm());
}

bool IsGood(Demuxer& dmx)
{
    return dmx.ObjStrm().good();
}

void VideoLine::ProcessPeriod(io::pos to_pos)
{
    Demuxer& dmx     = prsCont.dmx;
    io::stream& strm = dmx.ObjStrm();
    for( ; dmx.NextState(); )
    {
        if( strm.tellg() >= to_pos )
            break;
    }
    framLst.UpdateTimeIndex( dmx.IsEnd() || (strm.peek() == EOF) );
    // peek() тоже может выставлять eof, потому чистим после
    CleanEof(dmx);
}

void VideoLine::MakeForPeriod(io::pos beg, io::pos len)//, double req_ts)
{
    Clear();
    prsCont.dmx.ObjStrm().seekg(beg);

    ProcessPeriod(beg+len);
}

void VideoLine::MovePeriod(io::pos len)
{
    io::pos beg = prsCont.dmx.ObjStrm().tellg();

    ProcessPeriod(beg+len);
}

void VideoLine::ContinuePeriod(io::pos len)
{
    FrameList::Itr fix_itr;
    if( framLst.IsPlayable() )
        fix_itr = framLst.Beg();

    MovePeriod(len);
}


} // namespace Mpeg

