//
// mgui/tests/test_redivide.cpp
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

#include <mgui/tests/_pc_.h>

#include <mgui/redivide.h>
#include <mgui/render/rgba.h>

#include <mlib/string.h>
#include <mlib/tests/test_common.h>

BOOST_AUTO_TEST_CASE( test_dividerects )
{
    using namespace DividePriv;
    Rect rct1;
    Rect rct2;
    Rect tmp_rct;
    Rect rct1_, rct2_, tmp_rct_;

    // а)
    rct1 = Rect(0, 0, 1, 1);
    rct2 = Rect(1, 1, 2, 2);

    rct1_ = rct1;
    rct2_ = rct2;

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );

    // а)-2
    rct1 = Rect(0, 1, 1, 2);
    rct2 = Rect(1, 0, 2, 1);

    rct1_ = rct1;
    rct2_ = rct2;

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );

    // б)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 1, 3, 5);

    rct1_ = rct1;
    rct2_ = Rect(2, 1, 3, 2);
    tmp_rct_ = Rect(2, 4, 3, 5);

    BOOST_CHECK( rctINS_NEW == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );
    BOOST_CHECK( tmp_rct == tmp_rct_ );

    // в)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 1, 5, 3);

    rct1_ = rct1;
    rct2_ = Rect(2, 1, 5, 2);
    tmp_rct_ = Rect(4, 2, 5, 3);

    BOOST_CHECK( rctINS_NEW == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );
    BOOST_CHECK( tmp_rct == tmp_rct_ );

    // г)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 3, 5, 5);

    rct1_ = Rect(1, 2, 4, 3);
    rct2_ = rct2;
    tmp_rct_ = Rect(1, 3, 2, 4);

    BOOST_CHECK( rctINS_NEW == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );
    BOOST_CHECK( tmp_rct == tmp_rct_ );

    // г)-2
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 2, 5, 5);

    rct1_ = Rect(1, 2, 2, 4);
    rct2_ = rct2;

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );

    // д)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 1, 3, 3);

    rct1_ = rct1;
    rct2_ = Rect(2, 1, 3, 2);

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );

    // е)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 2, 5, 3);

    rct1_ = rct1;
    rct2_ = Rect(4, 2, 5, 3);

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );

    // е)-2
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 2, 4, 3);

    rct1_ = rct1;

    BOOST_CHECK( rctREMOVE == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );

    // ж)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 3, 3, 5);

    rct1_ = rct1;
    rct2_ = Rect(2, 4, 3, 5);

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );

    // ж)-2
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 3, 3, 4);

    rct1_ = rct1;

    BOOST_CHECK( rctREMOVE == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );

    // з)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 2, 3, 3);

    rct1_ = rct1;

    BOOST_CHECK( rctREMOVE == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );

    // и)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(1, 3, 5, 5);

    rct1_ = Rect(1, 2, 4, 3);
    rct2_ = rct2;

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );

    // к)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(2, 1, 5, 5);

    rct1_ = Rect(1, 2, 2, 4);
    rct2_ = rct2;

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );

    // л)=ж)
    rct1 = Rect(1, 2, 4, 4);
    rct2 = Rect(1, 2, 4, 4);

    rct1_ = rct1;

    BOOST_CHECK( rctREMOVE == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );

    // "впритык"
    rct1 = Rect(1, 4, 4, 5);
    rct2 = Rect(3, 1, 4, 4);

    rct1_ = rct1;
    rct2_ = rct2;

    BOOST_CHECK( rctNOTHING == DivideRects(rct1, rct2, tmp_rct) );
    BOOST_CHECK( rct1 == rct1_ );
    BOOST_CHECK( rct2 == rct2_ );
}

BOOST_AUTO_TEST_CASE( test_BRectLess )
{
    using namespace DividePriv;
    BRect brct1(Rect(0, 0, 1, 1), false);
    BRect brct2(Rect(1, 1, 2, 2), true);

    BOOST_CHECK( !BRectLess(brct1, brct1) );
    BOOST_CHECK( !BRectLess(brct1, brct2) );
    BOOST_CHECK(  BRectLess(brct2, brct1) );
    BOOST_CHECK( !BRectLess(brct2, brct2) );

    brct1.isEnabled = true;
    BOOST_CHECK(  BRectLess(brct1, brct2) );
    BOOST_CHECK( !BRectLess(brct2, brct1) );

    brct1.r = Rect(0, 0, 1, 1);
    brct2.r = Rect(0, 0, 2, 1);
    BOOST_CHECK(  BRectLess(brct2, brct1) );
}

namespace 
{

typedef std::vector<Rect>::const_iterator c_iter_typ;
struct RectCheck
{
    std::vector<bool> arr;
                Rect  rgn;
                 int  area;

                RectCheck(): area(0) { }
                RectCheck(const std::vector<Rect>& r_arr);

                // инициализировать новым массивом
          void  Init(const std::vector<Rect>& r_arr);
                // проверим, содержится ли прямоугольник в области
          bool  IsRectIn(const Rect& rct);
                // сохранить в файле как изображение (arr)
          void  Save(const char* name_prefix);

};

RectCheck::RectCheck(const std::vector<Rect>& r_arr): area(0)
{
    Init(r_arr);
}

struct RegionInit 
{
    bool  isInit;
    Rect  rgn;
           RegionInit(): isInit(false) { }

     void  operator()(const Rect& rct)
           {
               BOOST_CHECK( rct.IsValid() );
               if( !isInit )
               {
                   isInit = true;
                   rgn = rct;
               }
               else
                   rgn = Union(rgn, rct);
           }
};

struct RegionSum
{
    int sum;
            RegionSum(): sum(0) {}

      void  operator()(const bool val)
            {
                if( val ) sum++;
            }
};

typedef std::vector<bool>::reference bit_ref;
typedef boost::function<bool(bit_ref)> PointFnr;

static bool ForAllPoints(RectCheck& rc, Rect rct, const PointFnr& fnr)
{
    Rect& rgn = rc.rgn;
    int row_sz = rgn.Width();
    rct += Point(-rgn.lft, -rgn.top);

    bool force_break = false;
    for( int j=rct.top, row=j*row_sz; j<rct.btm; j++, row += row_sz )
    {
        for( int i=rct.lft; i<rct.rgt; i++ )
            if( !fnr(rc.arr[i+row]) )
            {
                force_break = true;
                break;
            }
        if( force_break )
            break;
    }
    return !force_break;
}

static bool FillTrue(bit_ref ref)
{
    ref = true;
    return true;
}

void RectCheck::Init(const std::vector<Rect>& r_arr)
{
    // 1 находим область
    rgn = std::for_each(r_arr.begin(), r_arr.end(), RegionInit()).rgn; 

    // 2 инициализируем массив
    arr.clear();
    Point sz = rgn.Size();
    arr.insert(arr.begin(), sz.x*sz.y, false);


    Point add_pnt(-rgn.lft, -rgn.top);
    for( c_iter_typ cur = r_arr.begin(), end = r_arr.end(); cur != end; ++cur )
    {
        //Rect rct(*cur);
        //rct += add_pnt;
        //for( int j=rct.top, row=j*sz.x; j<rct.btm; j++, row += sz.x )
        //    for( int i=rct.lft; i<rct.rgt; i++ )
        //        arr[i+row] = true;
        ForAllPoints(*this, *cur, &FillTrue);
    }

    // 3 находим площадь
    area = std::for_each(arr.begin(), arr.end(), RegionSum()).sum;
}

static bool CheckTrue(bit_ref ref)
{
    return ref;
}

bool RectCheck::IsRectIn(const Rect& rct)
{
    if( rct != Intersection(rgn, rct) )
        return false;
    return ForAllPoints(*this, rct, &CheckTrue);
}

void RectCheck::Save(const char* name_prefix)
{
    const char* fnam = TmpNam(0, name_prefix);
    io::stream strm(fnam, iof::def|iof::trunc);

    Point sz = rgn.Size();
    strm << "P6\n" << sz.x << " " << sz.y << "\n255\n";
    char w_c = 0xff;
    char b_c = 0;
    for( int i=0, cnt=arr.size(); i<cnt; i++ )
        if( arr[i] )
            strm << b_c << b_c << b_c;
        else
            strm << w_c << w_c << w_c;
}

void CheckRectIntegrity(const std::vector<Rect>& r_arr, RectCheck& rc)
{
    int area_sum = 0;
    for( c_iter_typ cur = r_arr.begin(), end = r_arr.end(); cur != end; ++cur )
    {
        // 1 проверим на непересекаемость
        BOOST_CHECK( !cur->IsNull() );
        for( c_iter_typ cur2 = cur+1; cur2 != end; ++cur2 )
            BOOST_CHECK( !cur->Intersects( *cur2 ) );

        // 2 входит в область
        BOOST_CHECK( rc.IsRectIn(*cur) );

        Point sz = cur->Size();
        area_sum += sz.x*sz.y;
    }
    // 3
    BOOST_CHECK_EQUAL( rc.area, area_sum );
    // 4 
    if( rc.area != area_sum )
    {
        const char* prefix = "redivide_";
        void* id_ = (void*)&r_arr;
        rc.Save( (str::stream() << prefix << id_ << "_src_").str().c_str() );

        RectCheck dst_rc(r_arr);
        dst_rc.Save( (str::stream() << prefix << id_ << "_dst_").str().c_str() );
    }
}

} // namespace


BOOST_AUTO_TEST_CASE( test_redivide )
{
    std::vector<Rect> r_arr;
    // 1
    r_arr.clear();
    r_arr.push_back(Rect(1, 2, 4, 4));
    r_arr.push_back(Rect(1, 2, 5, 4));

    ReDivideRects(r_arr);
    BOOST_CHECK( r_arr.size() == 1 );
    BOOST_CHECK( r_arr[0] == Rect(1, 2, 5, 4) );

    RectCheck rc;
    // 2
    r_arr.clear();
    r_arr.push_back(Rect(1, 2, 4, 4));
    r_arr.push_back(Rect(2, 3, 5, 5));

    rc.Init(r_arr);
    ReDivideRects(r_arr);
    CheckRectIntegrity(r_arr, rc);
    
    // 2-обратно
    r_arr.clear();
    r_arr.push_back(Rect(2, 3, 5, 5));
    r_arr.push_back(Rect(1, 2, 4, 4));

    rc.Init(r_arr);
    ReDivideRects(r_arr);
    CheckRectIntegrity(r_arr, rc);

    // 3
    r_arr.clear();
    r_arr.push_back(Rect(0, 0, 3, 3));
    r_arr.push_back(Rect(1, 1, 4, 4));
    r_arr.push_back(Rect(2, 2, 5, 5));

    rc.Init(r_arr);
    ReDivideRects(r_arr);
    CheckRectIntegrity(r_arr, rc);

    // 3-2
    r_arr.clear();
    r_arr.push_back(Rect(2, 2, 5, 5));
    r_arr.push_back(Rect(1, 1, 4, 4));
    r_arr.push_back(Rect(0, 0, 3, 3));

    rc.Init(r_arr);
    ReDivideRects(r_arr);
    CheckRectIntegrity(r_arr, rc);

    // 4
    r_arr.clear();
    r_arr.push_back(Rect(3, 0, 6, 3));
    r_arr.push_back(Rect(1, 1, 4, 4));
    r_arr.push_back(Rect(2, 2, 5, 5));

    rc.Init(r_arr);
    ReDivideRects(r_arr);
    CheckRectIntegrity(r_arr, rc);
}

struct RedivideFuxture
{
       std::vector<Rect> r_arr;
                   Rect  rct;
   RGBA::RectListDrawer  drw;
              RectCheck  rc;

                RedivideFuxture(): rct(0, 0, 20, 20), drw(r_arr) { }
               ~RedivideFuxture()
                {
                    int sz_ = r_arr.size();

                    rc.Init(r_arr);
                    ReDivideRects(r_arr);

                    int sz_2 = r_arr.size();

                    BOOST_CHECK( sz_ && sz_2 );
                    CheckRectIntegrity(r_arr, rc);
                }
};

BOOST_FIXTURE_TEST_CASE( test_redivide_frame, RedivideFuxture )
{
    // область рамки
    DrawGrabFrame(drw, rct);
}

BOOST_FIXTURE_TEST_CASE( test_redivide_frame_rect, RedivideFuxture )
{
    // область рамки с прямоугольником
    r_arr.push_back(rct);
    DrawGrabFrame(drw, rct);
}

BOOST_FIXTURE_TEST_CASE( test_redivide_frame_rect_copy, RedivideFuxture )
{
    // рамка с прямоугольником со смещенной копией
    r_arr.push_back(rct);
    DrawGrabFrame(drw, rct);

    rct += Point(10, 10);
    r_arr.push_back(rct);
    DrawGrabFrame(drw, rct);
}

BOOST_FIXTURE_TEST_CASE( test_redivide_bug, RedivideFuxture )
{
    // после первого разбиения текущий прямоугольник
    // станет "меньше" третьего - случай л)
    r_arr.push_back(Rect(0, 0, 5, 1));
    r_arr.push_back(Rect(3, -1, 7, 2));
    r_arr.push_back(Rect(0, 0, 4, 1));
}

