//
// mdemux/player.h
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

#ifndef __MDEMUX_PLAYER_H__
#define __MDEMUX_PLAYER_H__

#include <mlib/patterns.h>
#include <mlib/function.h>

#include "videoline.h"
#include "seek.h"

namespace Mpeg
{

class Player: public PlayerData
{
    public:
        typedef MpegDecodec::PlanesType PlanesType;

                         Player();
                         Player(const char* fname); 
     virtual            ~Player() {}

                   bool  Open(const char* fname);
                   bool  IsOpened();
                   void  Close();

                         // альтернативный вариант "проиграть" контент
                         // замечание - Close() не удалит данный fbuf, 
                         // а только вызовет fbuf->close();
                   bool  OpenFBuf(ptr::shared<io::fbuf> fbuf);
                         // полностью удалить ставший ненужным fbuf 
                         // можно (вызовом OpenFBuf(0))
                   bool  CloseFBuf();

              VideoLine& VLine() { return vl;   }
              MediaInfo& MInfo() { return mInf; }

                         // чтобы начать - SetTime()
                         // после этого IsPlaying() == true и можно вызывать Data() 
                   bool  IsPlaying() const { return IsInit(); }
                   bool  SetTime(double time);
                 double  CurTime();
                         // картинка по времени, установленному c SetTime()
     virtual PlanesType  Data() const = 0;

    protected:

            VideoLine  vl;

       FrameList::Itr  curPos;     // текущая позиция (логическая, в vl)
               double  curTime;    // текущее время (в пределах времени текущего кадра)
 
                         // "проиграть" до логической позиции log_pos
          virtual  void  SetPos(FrameList::Itr log_pos) = 0;          


void  Init();
bool  IsInit() const { return !curPos.IsNone(); }
void  CheckState();
void  SetPosByPos(FrameList::Itr log_pos, double time);
void  SetPosByTime(double time);
bool  OpenEx(const ActionFunctor& fnr);
};

// 
// FwdPlayer
// 

class FwdPlayer: public Player, Iterator<Player::PlanesType>
{
    public:
                         FwdPlayer() {}
                         FwdPlayer(const char* fname): Player(fname) {}

     virtual PlanesType  Data() const;
                         // интерфейс итератора
     virtual       void  First();
     virtual       void  Next();
     virtual       bool  IsDone() const { return !IsPlaying(); }
     virtual PlanesType& CurrentItem() const { return const_cast<FwdPlayer&>(*this).DoCurrentItem(); }

    protected:
       FrameList::Itr  lftBasePos; // позиции не B-кадров, образующих диапазон вроде IBBP
       FrameList::Itr  rgtBasePos; //   lftBasePos <= curPos <= rgtBasePos 

          virtual  void  SetPos(FrameList::Itr log_pos);

void  PlayBaseFrames(const FrameList::Itr& beg, const FrameList::Itr& end, bool is_reset = false);
void  PlayCurFrame();
bool  IsBoundFrame(const FrameList::Itr& log_pos);
FrameList::Itr  FindRgtNonBFrame(const FrameList::Itr& log_pos);

    private:
           PlanesType  dat;
PlanesType& DoCurrentItem() { dat = Data(); return dat; }
};


//
// Cтандартные данные
// 

const double PAL_SECAM_FRAME_FPS  = 25.0;           // PAL/SECAM FILM
const double NTSC_VIDEO_FRAME_FPS = 30000.0/1001.0; // NTSC VIDEO

inline double GetFrameFPS(Player& plyr)
{
    // по умолчанию ставим PAL
    return plyr.IsOpened() ? plyr.MInfo().FrameFPS() : PAL_SECAM_FRAME_FPS ;
}

// длина в секундах
inline double GetMediaSize(Player& plyr)
{
    if( !plyr.IsOpened() )
        return 0.;
    MediaInfo& inf = plyr.MInfo();
    return inf.endTime - inf.begTime;
}

inline Point GetAspectRatio(Player& plyr)
{
    if( !plyr.IsOpened() )
        return Point(4, 3);
    return plyr.MInfo().vidSeq.SizeAspect();
}

} // namespace Mpeg

void CheckOpen(Mpeg::Player& plyr, const std::string& fname);

#endif // __MDEMUX_PLAYER_H__

