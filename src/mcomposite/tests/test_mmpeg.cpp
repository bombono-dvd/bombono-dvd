//
// test_mmpeg.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007 Ilya Murav'jov
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

#include <mcomposite/tests/_pc_.h>

#include <mcomposite/mcomposite.h>
#include <mcomposite/mmpeg_demux.h>
#include <mcomposite/mmedia.h>

#include <mlib/string.h>
#include <string.h>

static bool FillBuffer(MpegPlayer& plr)
{
    bool res = true;
    while( !plr.trkBuf.IsFull() )
        if( !plr.demuxer->Demux(plr) )
        {
            res = false;
            break;
        }
    return res;// plr.trkBuf.IsFull();
}

static bool CmpCutBuffers(MpegPlayer& plr1, MpegPlayer& plr2)
{
    TrackBuf& tb1 = plr1.trkBuf;
    TrackBuf& tb2 = plr2.trkBuf;

    int sz = std::min(tb1.Size(), tb2.Size());

    bool res = memcmp(tb1.Beg(), tb2.Beg(), sz) == 0;

    tb1.CutStart(sz);
    tb2.CutStart(sz);

    return res;
}

static void RegrDemuxTest(const char* fnam)
{
    std::string err_str = (str::stream() << "Regression demuxer' test failed with file \"" << fnam << "\"!").str();

    MpegPlayer old_plr(new io::stream(fnam, iof::in), new Mpeg_legacy::MpegDemuxer);
    MpegPlayer new_plr(new io::stream(fnam, iof::in), new MpegSeqDemuxer);

    bool is_first = true;
    while( FillBuffer(old_plr) )
    {
        if( !FillBuffer(new_plr) )
        {
            is_first = false;
            break;
        }

        BOOST_REQUIRE_MESSAGE( CmpCutBuffers(old_plr, new_plr), err_str );
    }

    bool res = is_first ? FillBuffer(new_plr) : FillBuffer(old_plr) ;
    // и другой должен закончиться
    BOOST_REQUIRE_MESSAGE( (!res && (new_plr.trkBuf.Size() == old_plr.trkBuf.Size())), err_str );

    BOOST_REQUIRE_MESSAGE( CmpCutBuffers(old_plr, new_plr), err_str );
}

BOOST_AUTO_TEST_CASE( TestMpegDemux )
{
    // :TODO: проверять файлы по списку
    RegrDemuxTest("../Autumn.mpg");
    RegrDemuxTest("../transition.mpg");
    RegrDemuxTest("../Rock1.mpg");
    RegrDemuxTest("../Rock2.mpg");
}

