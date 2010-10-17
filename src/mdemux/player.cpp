//
// mdemux/player.cpp
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

#include "player.h"
#include "util.h"

#include <mlib/lambda.h>

namespace Mpeg
{

Player::Player()
    : vl(prsCont)
{
    prsCont.dmx.SetStrict(false);
    Init();
}

Player::Player(const char* fname)
    : vl(prsCont)
{
    Init();
    Open(fname);
}

void Player::Close()
{
    prsCont.Init();

    Init();
}

bool Player::OpenEx(const ActionFunctor& fnr)
{
    Close();
    fnr();
    if( !srcStrm.good() )
        return false;

    return mInf.GetInfo(prsCont);
}

static void OpenStream(io::stream& strm, const char* fname)
{
    strm.open(fname, iof::in);
}

bool Player::Open(const char* fname)
{
    //Close();
    //srcStrm.open(fname, iof::in);
    //if( !srcStrm.good() )
    //    return false;
    //
    //return mInf.GetInfo(prsCont);
    return OpenEx(boost::lambda::bind(&OpenStream, boost::ref(srcStrm), fname));
}

bool Player::OpenFBuf(ptr::shared<io::fbuf> fbuf)
{
    return OpenEx(boost::lambda::bind(&io::stream::init_buf, &srcStrm, fbuf));
}

bool Player::CloseFBuf()
{
    return !OpenFBuf(ptr::shared<io::fbuf>());
}

bool Player::IsOpened()
{
    //return srcStrm.good() && mInf.IsInit();
    return srcStrm.is_open() && mInf.IsInit();
}

void Player::Init()
{ 
    curPos.SetNone();
    curTime = INV_TS;
}

double Player::CurTime()
{
    return curTime;
}

void Player::CheckState()
{
    if( IsInit() )
    {
        FrameList& frm_lst = vl.GetFrames();
        ASSERT( frm_lst.IsPlayable() && 
                (frm_lst.Beg() <= curPos) && (curPos < frm_lst.End()) );
        UNUSED_VAR(frm_lst);
    }
}

void Player::SetPosByPos(FrameList::Itr log_pos, double time)
{
    // сохраняем, потому что последующее декодирование в SetPos()
    // изменит позицию (и dmx будет рассогласован для послед. SetTime())
    StreamPosSaver sps(srcStrm);

    SetPos(log_pos);
    curTime = time;
}

void Player::SetPosByTime(double time)
{
    FrameList& frm_lst = vl.GetFrames();
    FrameList::Itr log_pos = frm_lst.PosByTime(time);
    ASSERT( !log_pos.IsNone() );

    SetPosByPos(log_pos, time);
}

bool Player::SetTime(double time)
{
    ASSERT( IsOpened() );
    if( !mInf.IsInRange(time) )
        return false;

    if( !IsInit() )
    {
        if( !MakeForTime(vl, time, mInf) )
            return false;
        SetPosByTime(time);
    }
    else
    {
        CheckState();

        FrameList& frm_lst = vl.GetFrames();
        double beg_time    = frm_lst.TimeBeg();
        double end_time    = frm_lst.TimeEnd();
        if( time < beg_time )
        {
            Init();
            return SetTime(time);
        }
        else if( end_time <= time )
        {
            if( end_time + MaxContinueTime < time )
            {
                Init();
                return SetTime(time);
            }

            if( !MoveForTime(vl, time, mInf) )
                return false;
            SetPosByTime(time);
        }
        else
            SetPosByTime(time);
    }
    return true;
}

Player::PlanesType FwdPlayer::Data() const
{
    ASSERT( IsInit() );
    FrameDecType fdt = fdtCURRENT;
    if( curPos == rgtBasePos )
        fdt = fdtRIGHT;
    else if( curPos == lftBasePos )
    {
        // см. PlayBaseFrames()
        ASSERT( lftBasePos != rgtBasePos );
        fdt = fdtLEFT;
    }

    return prsCont.m2d.FrameData(fdt);
}

static FrameList::Itr FindGopPos(FrameList& lst, const FrameList::Itr& log_pos)
{
    FrameList::Itr pos;
    for( pos=log_pos; pos>=lst.Beg(); --pos )
        if( pos->typ == ptI_FRAME )
            break;
    if( pos < lst.Beg() )
        pos = lst.Beg();
    return pos;
}

void FwdPlayer::PlayBaseFrames(const FrameList::Itr& beg, const FrameList::Itr& end, bool is_reset)
{
    if( is_reset )
        prsCont.m2d.Init(false);

    bool left_reset = is_reset;
    for( FrameList::Itr i=beg; i<end; ++i )
    {
        FrameData& fram = *i;
        if( fram.typ != ptB_FRAME )
        {
            prsCont.m2d.ReadFrame(fram, prsCont.dmx.ObjStrm());
            if( left_reset )
                // в случае переноса (reset) в левый базовый
                // сносим тоже, что и в правый, но один раз
                // (самый первый раз)
                // Пример: переход на I-кадр в середине,
                // (lftBasePos = curPos = rgtBasePos)
                lftBasePos = i;
            else
                lftBasePos = rgtBasePos;
            rgtBasePos = i;
        }
        else
        {
            if( left_reset )
            {
                ASSERT( i == vl.GetFrames().Beg() );
                lftBasePos.SetNone();
            }
        }
        left_reset = false;
    }
}

bool FwdPlayer::IsBoundFrame(const FrameList::Itr& log_pos)
{
    return (log_pos == lftBasePos) || (log_pos == rgtBasePos);
}

void FwdPlayer::PlayCurFrame()
{
    ASSERT( IsInit() && lftBasePos <= curPos && curPos <= rgtBasePos );
    if( IsBoundFrame(curPos) )
        ; // уже готово
    else
    {
        FrameData& fram = *curPos;
        ASSERT( fram.typ == ptB_FRAME );

        prsCont.m2d.ReadFrame(fram, prsCont.dmx.ObjStrm());
    }
}

FrameList::Itr FwdPlayer::FindRgtNonBFrame(const FrameList::Itr& log_pos)
{
    FrameList::Itr pos;
    for( FrameList::Itr i=log_pos, end=vl.GetFrames().DataEnd(); i<end; ++i )
        if( i->typ != ptB_FRAME )
        {
            pos = i;
            break;
        }
    ASSERT( !pos.IsNone() );
    return pos;
}

void FwdPlayer::SetPos(FrameList::Itr log_pos)
{
    if( !IsInit() )
    {
        lftBasePos.SetNone();
        rgtBasePos.SetNone();
    }
    else
    {
        ASSERT( !rgtBasePos.IsNone() );
        if( curPos == log_pos )
            return;
    }

    if( (lftBasePos <= log_pos) && (log_pos <= rgtBasePos)  )
        ; // все уже готово
    else
    {
        FrameList& frm_lst = vl.GetFrames();
        FrameList::Itr r_pos = FindRgtNonBFrame(log_pos);

        if( r_pos <= lftBasePos )
            PlayBaseFrames(FindGopPos(frm_lst, log_pos), r_pos+1, true);
        else
        {
            ASSERT( rgtBasePos < r_pos );
            FrameList::Itr l_pos = FindGopPos(frm_lst, log_pos);

            if( l_pos <= rgtBasePos )
                PlayBaseFrames(rgtBasePos+1, r_pos+1);
            else
                PlayBaseFrames(l_pos, r_pos+1, true);
        }
    }

    curPos = log_pos;
    PlayCurFrame();
}

void FwdPlayer::First()
{
    if( !IsOpened() )
        return;
    SetTime(mInf.begTime);
}

void FwdPlayer::Next()
{
    if( IsDone() )
        return;

    // парсим до след. кадра, если надо
    FrameList& frm_lst = vl.GetFrames();
    for( ; curPos+1 >= frm_lst.End(); vl.MovePeriod(DecodeBlockSize) )
        if( prsCont.dmx.IsEnd() )
        {
            Init();
            return;
        }

    FrameList::Itr new_pos = curPos+1;
    SetPosByPos(new_pos, new_pos->pts);
}

} // namespace Mpeg

void CheckOpen(Mpeg::Player& plyr, const std::string& fname)
{
    bool is_open = plyr.Open(fname.c_str());
    ASSERT_OR_UNUSED( is_open );
}


