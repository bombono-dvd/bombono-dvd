//
// mdemux/tests/log.h
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

#ifndef __MDEMUX_TESTS_LOG_H__
#define __MDEMUX_TESTS_LOG_H__

#include <mdemux/player.h>

namespace Mpeg { namespace Log {

///////////////////////////////////////////////////
// Демультиплексирование

void LogScr(double scr);
void LogPts(double pts);

void PrintStats();

// статистическая инфо о метке времени в потоке
struct TSInfo
{
    bool  isInit; 
  double  prevTs;

  double  firstTs;
  double  minDist;
  double  maxDist;
     int  tsCnt;

    bool  isReverse;
     int  disorderCnt;
  double  maxNegTs;

     int  maxDisorderCnt;
  double  maxNegDist;

            TSInfo(): isInit(false), isReverse(false), prevTs(-1.) {}

    double  CurTS() { return prevTs; }

      void  LogTs(double ts);
      void  PrintTsStats(double max_dist = 0.);
};


struct TimestampStats
{
     TSInfo  ptsInf;
     TSInfo  scrInf;

       bool  disorderScrPts;
     double  maxScrPtsDist;
     double  maxScrPtsDisorder;

            TimestampStats() { Init(); }
      void  Init() 
            { 
                disorderScrPts = false; 
                maxScrPtsDist = 0.;

                ptsInf.isInit = false;
                scrInf.isInit = false;
            }
};

extern TimestampStats TsStats;

} } // namespace Log Mpeg

#endif // #ifndef __MDEMUX_TESTS_LOG_H__

