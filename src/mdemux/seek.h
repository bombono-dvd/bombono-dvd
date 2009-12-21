//
// mdemux/seek.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#ifndef __MDEMUX_SEEK_H__
#define __MDEMUX_SEEK_H__

#include <mlib/stream.h>

#include "videoline.h"
#include "service.h"

namespace Mpeg { 

class ParseContextSaver
{
    public:
            ParseContextSaver(ParseContext& cont): dmxSaver(cont.dmx), dcrSaver(cont.dcr) {}
    protected:
            ServiceSaver  dmxSaver;
           VServiceSaver  dcrSaver;
};

struct MediaInfo
{
        double  begTime;  // начало и конец видео по времени
        double  endTime; 
       io::pos  begPos;   // по данным
       io::pos  endPos;
  SequenceData  vidSeq; // параметры видео

                    MediaInfo() { Init(); }
            
              void  Init()   { begTime = INV_TS; }
              bool  IsInit() { return begTime != INV_TS; }
            
              bool  GetInfo(ParseContext& p_cont);
        const char* ErrorReason() 
                    {
                        if( errReason.empty() )
                            return "Unknown stream error";
                        else
                            return errReason.c_str();
                    }

              bool  IsInRange(double time) 
                    {
                        return IsInit() && (begTime <= time) && (time < endTime);
                    }

                    // "статистические" данные
            double  FrameLength() { return vidSeq.framRat.y/(double)vidSeq.framRat.x; }
            double  FrameFPS()    { return vidSeq.framRat.x/(double)vidSeq.framRat.y; }
               int  FramesCount() { return framesCnt; }
            double  FrameTime(int fram_num, bool phis_time = true) 
                    {
                        double time = fram_num * FrameLength();
                        if( phis_time )
                            time += begTime;
                        return time; 
                    }

    protected:

   std::string  errReason;
           int  framesCnt;

bool TryGetInfo(ParseContext& p_cont);
bool InitBegin(VideoLine& vl);
bool InitEnd(VideoLine& vl);
};

struct TSPos
{
    io::pos  pos;
     double  ts;

        TSPos(): ts(INV_TS), pos(-1) {}
        TSPos(io::pos p, double t): ts(t), pos(p) {}
};

class SeekService: public FirstVideoSvc, ServiceSaver
{
    public:
                    SeekService(Demuxer& dmx);

             TSPos  FindForPts(double seek_time, MediaInfo& inf);
      virtual void  GetData(Demuxer& dmx, int len);
    protected:

double GetTSByPos(io::pos pos, bool seek_scr);
TSPos  FindTSPos(double lft_bound, double rgt_bound,
                 TSPos beg, TSPos end, bool& is_good);

    private:
            bool  seekSCR; // GetTSByPos() - ищем первый SCR, иначе PTS
            bool  foundTS;
};

// разницу между вызовами см. в VideoLine
bool MakeForTime(VideoLine& vl, double time, MediaInfo& inf);
bool MoveForTime(VideoLine& vl, double time, MediaInfo& inf);
bool ContinueForTime(VideoLine& vl, double time, MediaInfo& inf);

struct PlayerData
{
    io::stream  srcStrm;
  ParseContext  prsCont;
     MediaInfo  mInf;

     PlayerData(): prsCont(srcStrm) {}
};

inline bool GetInfo(PlayerData& pd, const char* fname)
{
    pd.srcStrm.open(fname, iof::in);
    return pd.mInf.GetInfo(pd.prsCont);
}

} // namespace Mpeg

#endif // __MDEMUX_SEEK_H__

