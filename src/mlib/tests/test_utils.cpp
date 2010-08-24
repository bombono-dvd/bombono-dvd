//
// mlib/tests/test_utils.cpp
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

#include <mlib/tests/_pc_.h>

#include "test_common.h"

#include <mlib/sdk/misc.h>
#include <mlib/filesystem.h>
#include <mlib/string.h>

template<typename T>
void CheckDefaultPoint(PointT<T> pnt)
{
    BOOST_CHECK( pnt.x == 0 && pnt.y == 0 );
}

BOOST_AUTO_TEST_CASE( TestPointT )
{
    Point i_pnt;
    CheckDefaultPoint(i_pnt);

    DPoint d_pnt;
    CheckDefaultPoint(d_pnt);

    Point pnt(0, 1);
    BOOST_CHECK( !pnt.IsNull() );
    BOOST_CHECK( IsNullSize(pnt) );
}


BOOST_AUTO_TEST_CASE( TestSetLocation )
{
    Rect rct(0, 0, 1, 2);
    Point lct(1, 3);

    Rect copy_rct(rct);
    SetLocation(copy_rct, lct);
    BOOST_CHECK( rct.Size() == copy_rct.Size() );

    ShiftTo00(copy_rct);
    BOOST_CHECK_EQUAL( rct, copy_rct );

}

static void CheckConvert(MpegDP& dp, AspectFormat af)
{
    dp.GetAF() = af;

    MpegDP dp2(afUNKNOWN, Point(1, 1));
    BOOST_CHECK( TryConvertParams(dp2, dp) ); 
    BOOST_CHECK( dp2 == dp ); 
}

BOOST_AUTO_TEST_CASE( TestDisplayParams )
{
    // PAL
    MpegDP dp(af4_3, Point(720, 576));
    BOOST_CHECK_EQUAL( dp.DisplayAspect(), Point(4, 3) ); 
    BOOST_CHECK_EQUAL( dp.PixelAspect(),   Point(16, 15) );

    dp.GetAF() = af16_9;
    BOOST_CHECK_EQUAL( dp.DisplayAspect(), Point(16, 9) ); 
    BOOST_CHECK_EQUAL( dp.PixelAspect(),   Point(64, 45) );

    // NTSC
    dp.GetSize() = Point(720, 480);
    dp.GetAF() = af4_3;
    BOOST_CHECK_EQUAL( dp.DisplayAspect(), Point(4, 3) ); 
    BOOST_CHECK_EQUAL( dp.PixelAspect(),   Point(8, 9) );

    dp.GetAF() = af16_9;
    BOOST_CHECK_EQUAL( dp.DisplayAspect(), Point(16, 9) ); 
    BOOST_CHECK_EQUAL( dp.PixelAspect(),   Point(32, 27) );

    // 1x1
    dp.GetAF() = afSQUARE;
    BOOST_CHECK_EQUAL( dp.DisplayAspect(), Point(3, 2) ); 
    BOOST_CHECK_EQUAL( dp.PixelAspect(),   Point(1, 1) );

    // *
    CheckConvert(dp, afSQUARE);
    CheckConvert(dp, af4_3);
    CheckConvert(dp, af16_9);
    CheckConvert(dp, af221_100);
}

BOOST_AUTO_TEST_CASE( TestFilesystem )
{
    // файлы в utf-8
    {
        const char* str = "Авторинг";

        std::string path("../");
        path += str;
        path += ".txt";

        fs::path author_path(path);
        std::string base = get_basename(author_path);
        BOOST_CHECK_EQUAL(str, base);
    }

    // is_complete
    {
        fs::path pth("../some_file");
        BOOST_CHECK( !pth.is_complete() );
        BOOST_CHECK( fs::current_path().is_complete() );

        fs::path apth = Project::MakeAbsolutePath(pth);
        //LOG_INF << "Making abs path: " << pth.string() << " => " << apth.string() << io::endl;

        BOOST_CHECK( Project::MakeAbsolutePath(pth, "/").is_complete() );
        BOOST_CHECK( !Project::MakeAbsolutePath(pth, "./").is_complete() );
    }

    // MakeRelativeToDir
    {
        fs::path dir("/a/b/c/d/e/f");
        fs::path pth("/a/b/c/r/s/t/u");
        fs::path res("../../../r/s/t/u");
        BOOST_CHECK( Project::MakeRelativeToDir(pth, dir) );
        BOOST_CHECK_MESSAGE( pth == res, pth.string() << " != " << res.string() );

        dir  = "/";
        pth = "/usr";
        BOOST_CHECK( Project::MakeRelativeToDir(pth, dir) && (pth == fs::path("usr")) );

        dir  = "/usr/lib/ttt";
        pth = "/usr";
        BOOST_CHECK( Project::MakeRelativeToDir(pth, dir) && (pth == fs::path("../..")) );

        dir  = "/usr/ff/../yyy";
        pth = "/ttt";
        BOOST_CHECK( Project::MakeRelativeToDir(pth, dir) && (pth == fs::path("../../ttt")) );
    }

    // 
    //BOOST_CHECK( !fs::exists("/root/.config") );
}

BOOST_AUTO_TEST_CASE( TestStringstream )
{
    str::stream strm("rrrr");
    strm << "tttt";
    BOOST_CHECK_EQUAL( strm.str(), "rrrrtttt" );
    strm.str("aaa");
    BOOST_CHECK_EQUAL( strm.str(), "aaa" );
}

