//
// test_mcomposite.cpp
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
#include <mcomposite/mmedia.h>

#include <mlib/tests/test_common.h>

#include <string.h> // memcmp()

using namespace Planed;

void ComparePlanes(Plane& pl1, Plane& pl2)
{
    BOOST_CHECK_EQUAL(pl1.x, pl2.x);
    BOOST_CHECK_EQUAL(pl1.y, pl2.y);

    BOOST_CHECK( memcmp(pl1.data, pl2.data, pl1.Size()) == 0 );
}

// тест преобразования yuv->Image->yuv
BOOST_AUTO_TEST_CASE( YuvToImageToYuv )
{
    std::string path = GetTestFileName("flower.yuv");

    Comp::YuvMedia md(path.c_str());
    BOOST_REQUIRE( md.inFd != NO_HNDL );

    BOOST_CHECK( md.Begin() );
    BOOST_CHECK( md.GetFrame() );

    // 1 yuv->Image
    int wdh = md.Width(), hgt = md.Height();
    Magick::Image img;
    InitImage(img, wdh, hgt);
    YuvContextIter iter(md);
    TransferToImg(img, wdh, hgt, iter, false);
    //img.display();

    // 2 Image->yuv
    int out_fd = NO_HNDL;
    //out_fd = CmdOptions::OpenFileAsArg("tttt", false);
    //BOOST_REQUIRE(out_fd != NO_HNDL);

    OutYuvContext out_cont(out_fd);
    out_cont.SetInfo(md.Info());

    CopyImageToPlanes(out_cont, img);
    //out_cont.PutFrame();

    // 3 сравнение результатов
    ComparePlanes(md.YPlane(), out_cont.YPlane());
    ComparePlanes(md.UPlane(), out_cont.UPlane());
    ComparePlanes(md.VPlane(), out_cont.VPlane());
}

