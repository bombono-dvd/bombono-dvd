//
// mdemux/tests/log.cpp
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

#include <math.h>

#include <mlib/string.h>
#include <mlib/sdk/logger.h>

#include <mdemux/mpeg.h>
#include <mdemux/mpeg_video.h>
#include <mdemux/util.h>

#include "log.h"

namespace Mpeg { namespace Log {

TimestampStats TsStats;

void TSInfo::LogTs(double ts)
{
    if( !isInit )
    {
        isInit = true;

        firstTs = ts;
        minDist = 100;  // по стандарту 0 <= dist <= 0.7
        maxDist = 0;
        tsCnt = 0;

        maxDisorderCnt = 0;
        maxNegDist = 0.;

    }
    else
    {
        if( prevTs > ts )
        {
            isReverse = true;
            if( !disorderCnt )
            {
                disorderCnt = 1;
                maxNegTs = prevTs;
            }
        }
        else
        {
            minDist = std::min(minDist, ts - prevTs);
            maxDist = std::max(maxDist, ts - prevTs);
        }

        if( disorderCnt && maxNegTs > ts )
        {
            maxNegDist = std::max(maxNegDist, maxNegTs - ts);
            disorderCnt++;
        }
        else
            disorderCnt = 0;

        maxDisorderCnt = std::max(maxDisorderCnt, disorderCnt);
    }
    prevTs = ts;
    tsCnt++;
}

void TSInfo::PrintTsStats(double max_dist)
{
    if( !isInit )
    {
        LOG_INF << "No ts found!" << io::endl;
        return;
    }

    if( isReverse )
    {
        LOG_INF << "There is greater ts before one!" << io::endl;
        LOG_INF << "Max disorder group (count) : " << maxDisorderCnt << io::endl;
        LOG_INF << "Max negative distance (sec): " << maxNegDist     << io::endl;

        LOG_INF << io::endl;
    }

    LOG_INF << "First ts: \t\t" << firstTs << io::endl;
    LOG_INF << "Last  ts: \t\t" << prevTs  << io::endl;
    double len = prevTs - firstTs;
    LOG_INF << "Time length (by ts): \t" << SecToHMS(len) << " (" << len << " s)" << io::endl << io::endl;

    LOG_INF << "Min distance b/w ts: \t" << minDist << io::endl;
    LOG_INF << "Max distance b/w ts: \t" << maxDist << io::endl;
    if( max_dist != 0. && max_dist < maxDist )
        LOG_INF << "Permitted max distance (" << max_dist << " sec) is violated!" << io::endl;
    if( tsCnt >= 2 )
        LOG_INF << "Avg distance b/w ts: \t" << len / (tsCnt-1) << io::endl;
    LOG_INF << "Total ts number: " << tsCnt << io::endl;
}

void LogPts(double pts)
{
    TsStats.ptsInf.LogTs(pts);

    ASSERT( TsStats.scrInf.isInit );
    double len = pts - TsStats.scrInf.prevTs;
    if( len<0 )
    {
        TsStats.maxScrPtsDisorder = std::max( TsStats.disorderScrPts ? TsStats.maxScrPtsDisorder : 0., 
                                              -len );
        TsStats.disorderScrPts = true;
    }
    else
        TsStats.maxScrPtsDist = std::max(TsStats.maxScrPtsDist, len);
}

void LogScr(double scr)
{
    TsStats.scrInf.LogTs(scr);
}

void PrintStats()
{
    LOG_INF << "\tPTS Stats." << io::endl;
    LOG_INF << "\t----------" << io::endl << io::endl;
    TsStats.ptsInf.PrintTsStats(0.7);

    LOG_INF << io::endl; 
    LOG_INF << "\tSCR Stats." << io::endl;
    LOG_INF << "\t----------" << io::endl << io::endl;
    TsStats.scrInf.PrintTsStats(0.7);

    LOG_INF << io::endl; 
    LOG_INF << "\tSCR vs. PTS Stats." << io::endl;
    LOG_INF << "\t--------------" << io::endl << io::endl;

    LOG_INF << "Max decode delay: " << TsStats.maxScrPtsDist << " sec" << io::endl;
    // Doc: 2.4.2.6, D.0.1
    if( TsStats.maxScrPtsDist>1. )
        LOG_INF << "Permitted delay (1 sec, if not still pictures) is violated!" << io::endl;
    if( TsStats.disorderScrPts )
    {
        LOG_INF << "There is scr greater pts!" << io::endl;
        LOG_INF << "Max negative value is: " << TsStats.maxScrPtsDisorder << " sec" << io::endl;
    }
}

#if 0
void FillImage(Magick::Image& img, Mpeg::Player& plyr)
{
    Mpeg::SequenceData& seq = plyr.MInfo().vidSeq;
    BOOST_ASSERT( plyr.IsPlaying() );
    BOOST_ASSERT( seq.chromaFrmt == Mpeg::ct420 );

    Point sz(seq.wdh, seq.hgt);
    if( sz != Point(img.columns(), img.rows()) )
        InitImage(img, sz.x, sz.y);

    MpegDecodec::PlanesType buf = plyr.Data();

    using namespace Planed;
    Plane y_p(sz.x, sz.y, buf[0]);
    Plane u_p(sz.x/2, sz.y/2, buf[1]);
    Plane v_p(sz.x/2, sz.y/2, buf[2]);
    YuvContextIter iter(y_p, u_p, v_p);

    TransferToImg(img, sz.x, sz.y, iter);
}
#endif

} } // namespace Temp Mpeg

