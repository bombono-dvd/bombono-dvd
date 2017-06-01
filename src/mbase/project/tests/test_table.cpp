//
// mbase/project/tests/test_table.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#include <mbase/tests/_pc_.h>

//#include <libxml/tree.h>

#include <mlib/sdk/logger.h>
#include <mlib/sdk/misc.h>

#include <mbase/project/archieve.h>
#include <mbase/project/table.h>
#include <mbase/project/srl-common.h>

#include <mlib/tests/test_common.h>
#include "menu-utils.h"

#include <mlib/filesystem.h>

using namespace Project;

// rest_counts - сколько должно быть ссылок на объект
// (по умолчанию - одна в массиве + 1 на внешнем стеке)
void CheckRefCounts(MediaItem mi, int rest_counts = 2)
{
    // одна ссылка на этом стеке
    int count = mi->use_count()-1;
    BOOST_CHECK_MESSAGE( count == rest_counts, "for Media: " << mi->mdName << 
                         " " << count  << " != " << rest_counts );
}

BOOST_AUTO_TEST_CASE( TestMediaList )
{
    DBCleanup db_cleanup;

    MediaList& ml = AData().GetML();
    // заполняем
    {
        // 1
        VideoItem autumn_vd(new VideoMD);
        ml.Insert(autumn_vd);
        autumn_vd->MakeByPath("../Autumn.mpg");

        ChapterItem chp;
        chp = VideoChapterMD::CreateChapter(autumn_vd.get(), 30);
        BOOST_CHECK_EQUAL( chp->mdName, "Chapter 1" );

        VideoChapterMD::CreateChapter(autumn_vd.get(), 20);
        chp = VideoChapterMD::CreateChapter(autumn_vd.get(), 10);

        autumn_vd->OrderByTime();
        BOOST_CHECK( chp == autumn_vd->List()[0] );

        // 2
        boost::intrusive_ptr<StillImageMD> pict(new StillImageMD);
        ml.Insert(pict);
        pict->MakeByPath("../AcerTX.jpg");
        BOOST_CHECK( pict->mdName == "AcerTX" );

        // 3
        boost::intrusive_ptr<StillImageMD> pict2(new StillImageMD);
        ml.Insert(pict2);
        pict2->MakeByPath(GetTestFileName("flower.jpg"));

        // проверка индексации
        ml.Index();
        BOOST_CHECK_EQUAL( ml.Find(autumn_vd), 0 );
        BOOST_CHECK_EQUAL( ml.Find(pict),      1 );
        BOOST_CHECK_EQUAL( ml.Find(pict2),     2 );
    }

    TmpFileNames tmp_names;
    //tmp_names.ToRemove() = false;
    std::string tmp_name = tmp_names.CreateName("medias.xml"); // "../medias.xml";
    // сохраняем
    SaveProjectMedias(tmp_name);

    // загружаем
    {
        AData().Clear();
        BOOST_CHECK( ml.Size() == 0 );
        try
        {
            LoadProjectMedias(tmp_name);
        }
        catch (const std::exception& err)
        {
            BOOST_CHECK(0);
            io::cout << "Catch error: " << err.what() << " !" << io::endl;
            AData().Clear();
        }

        BOOST_CHECK( ml.Size() == 3 );

        VideoItem autumn_vd = IsVideo(ml[0]);
        BOOST_CHECK( autumn_vd );
        BOOST_CHECK_EQUAL( autumn_vd->mdName, "Autumn" );

        ChapterItem chp = IsChapter(autumn_vd->List()[0]);
        BOOST_CHECK( chp );
        BOOST_CHECK_EQUAL( chp->mdName, "Chapter 3" );
        BOOST_CHECK_EQUAL( chp->chpTime, 10 );
        // пример удаления
        DeleteMedia(chp); // главы явно удаляются
        CheckRefCounts(chp, 1);

        boost::intrusive_ptr<StillImageMD> pict = ptr::dynamic_pointer_cast<StillImageMD>(ml[1]);
        BOOST_CHECK( pict );
        BOOST_CHECK_EQUAL( pict->mdName, "AcerTX" );

        // проверка чтения/записи путей
        boost::intrusive_ptr<StillImageMD> pict2 = ptr::dynamic_pointer_cast<StillImageMD>(ml[2]);
        fs::path pth = pict2->GetPath();
        BOOST_CHECK( pth.is_absolute() );
        BOOST_CHECK( fs::exists(pth) );

        // проверка ссылок
        CheckRefCounts(pict2);
    }
}

// void TryOpenFilename(std::string fname, const char* enc)
// {
//     io::stream strm(fname.c_str());
//     if( strm.is_open() )
//     {
//         io::cout << "Open " << enc << " version filename" << io::endl;
//         io::cout << "Contents: ";
//         for( char c; strm >> c, strm; )
//             io::cout << c;
//     }
// }


////////////////////////////////////////////////////////////
// TestMenuList - тест сериализации меню

static void CheckMenusSize(int i)
{
    BOOST_CHECK_EQUAL( (int)AData().GetMN().Size(), i );
}

void CheckOneItem(Menu mn, const char* name, const Rect& plc)
{
    BOOST_CHECK( (int)mn->List().size() == 1 );
    MenuItem mi = mn->List()[0];
    BOOST_CHECK_EQUAL( mi->mdName, name );
    BOOST_CHECK_MESSAGE( mi->Placement() == plc, "for Menu: " << mn->mdName );
}

static void CheckMenuItemRef(Menu mn, const char* ref_name)
{
    BOOST_CHECK_EQUAL( (int)mn->List().size(), 1 );
    MenuItem itm = mn->List()[0];
    BOOST_CHECK( itm->Ref() );
    BOOST_CHECK_EQUAL( itm->Ref()->mdName, ref_name );
}

Menu InsertMenu(MenuList& ml, const std::string& name = std::string())
{
    Menu mn = name.empty() ? MakeMenu(ml.Size()) : MakeMenu(name);
    ml.Insert(mn);
    return mn;
}

BOOST_AUTO_TEST_CASE( TestMenuList )
{
    DBCleanup db_cleanup;

    MediaList& mds = AData().GetML();
    MenuList&  mns = AData().GetMN();
    const Rect plc(1, 2, 3, 4);
    //Menu menu;

    // * создаем список
    {
        Menu mn = InsertMenu(mns);
        MenuItem itm = new TextItemMD(mn.get());
        itm->mdName = "Text-Menu 3";
        itm->Placement() = plc;

        Menu mn2  = InsertMenu(mns);
        MenuItem itm2 = new FrameItemMD(mn2.get());
        itm2->mdName = "Frame-Autumn";
        itm2->Placement() = plc;

        Menu mn3 = InsertMenu(mns);

        // сделаем ссылки
        VideoItem autumn_vd(new VideoMD);
        mds.Insert(autumn_vd);
        autumn_vd->MakeByPath("Autumn.mpg");
        ChapterItem chp = VideoChapterMD::CreateChapter(autumn_vd.get(), 10);

        itm->Ref()   = mn3;
        itm2->Ref()  = autumn_vd;
        mn3->BgRef() = chp;
    }

    TmpFileNames tmp_names;
    //tmp_names.ToRemove() = false;
    std::string tmp_name = tmp_names.CreateName("menus.xml"); // "../menus.xml";
    // *
    AData().SaveAs(tmp_name);
    CheckMenusSize(3);

    // * загружаем
    AData().Load(tmp_name);
    CheckMenusSize(3);

    BOOST_CHECK_EQUAL( mns[0]->mdName, "Menu 1" );
    BOOST_CHECK_EQUAL( mns[1]->mdName, "Menu 2" );
    BOOST_CHECK_EQUAL( mns[2]->mdName, "Menu 3" );

    Menu mn0 = mns[0];
    Menu mn1 = mns[1];
    // пункты
    CheckOneItem(mn0, "Text-Menu 3",  plc);
    CheckOneItem(mn1, "Frame-Autumn", plc);

    // одна ссылка в таблице + 1 локальная (ссылок нет)
    CheckRefCounts(mn1, 2);

    // проверяем ссылки
    VideoItem vi = IsVideo(mds[0]);

    CheckMenuItemRef(mn0, "Menu 3");
    CheckMenuItemRef(mn1, "Autumn");
    MediaItem bg_mi = mns[2]->BgRef();
    BOOST_CHECK( bg_mi );
    BOOST_CHECK_EQUAL( bg_mi->mdName, "Chapter 1" );
}

/*

DVD-Video can consist of either MPEG-2 at up to 9.8 Mbit/s (9800 kbit/s) or MPEG-1 at up to 1.856 Mbit/s (1856 kbit/s).


The following formats are allowed for MPEG-2 video:

    * At 25 fps (usually used in regions where PAL is standard):

    720 × 576 pixels MPEG-2 (Called full D1)
    704 × 576 pixels MPEG-2
    352 × 576 pixels MPEG-2 (Called Half-D1, same as the China Video Disc standard)
    352 × 288 pixels MPEG-2

    * At 29.97 or 23.976 fps (usually used in regions where NTSC is standard):

    720 × 480 pixels MPEG-2 (Called full D1)
    704 × 480 pixels MPEG-2
    352 × 480 pixels MPEG-2 (Called Half-D1, same as the China Video Disc standard)
    352 × 240 pixels MPEG-2


The following formats are allowed for MPEG-1 video:

    * 352 × 288 pixels MPEG-1 at 25 fps (Same as the VCD Standard)
    * 352 × 240 pixels MPEG-1 at 29.97 fps (Same as the VCD Standard)

*/

Point VideoToPixelAspect(Point sz, Point vasp)
{
    vasp.x *= sz.y;
    vasp.y *= sz.x;

    ReducePair(vasp);
    return vasp;
}

void PrintPairs(const Point& sz, const Point& vasp, bool is_pal)
{
    io::cout << (is_pal ? "PAL: " : "NTSC: ") << PointToStr(sz) << " res, \t" 
             << PointToStr(vasp) << " dsp aspect, \t"
             << PointToStr(VideoToPixelAspect(sz, vasp)) << " pix aspect" << io::endl;
}

void PrintPairsForPALVAsp(const Point& vasp)
{
    io::cout << io::endl << PointToStr(vasp) << io::endl;
    PrintPairs(Point(720, 576), vasp, true);
    PrintPairs(Point(704, 576), vasp, true);
    PrintPairs(Point(352, 576), vasp, true);
    PrintPairs(Point(352, 288), vasp, true);
}

void PrintPairsForNTSCVAsp(const Point& vasp)
{
    io::cout << io::endl << PointToStr(vasp) << io::endl;
    PrintPairs(Point(720, 480), vasp, false);
    PrintPairs(Point(704, 480), vasp, false);
    PrintPairs(Point(352, 480), vasp, false);
    PrintPairs(Point(352, 240), vasp, false);
}

BOOST_AUTO_TEST_CASE( TestAspects )
{
    return;

    PrintPairsForPALVAsp(Point(4,3));
    PrintPairsForPALVAsp(Point(16,9));

    io::cout << "----" << io::endl;
    PrintPairsForNTSCVAsp(Point(4,3));
    PrintPairsForNTSCVAsp(Point(16,9));
}
