//
// mgui/redivide.cpp
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

#include <algorithm>

#include <mlib/tech.h>

#include "redivide.h"

namespace DividePriv
{

// для получения крайних точек прямоугольника включительно
inline int Lft(Rect& rct) { return rct.lft; }
inline int Top(Rect& rct) { return rct.top; }
inline int Rgt(Rect& rct) { return rct.rgt-1; }
inline int Btm(Rect& rct) { return rct.btm-1; }

// Варианты пересечения двух прямоугольников с условием rct1 < rct2 по верхей левой вершине:
// 0. В rct1 содержится 0 вершин rct2:
//  а) (не пересекаются)     б) крест-накрест
//       1----  2----              2---     
//       |   |  |   |              |  |
//       -----  -----            1_______
//                               | |  | | 
//                               | |  | |
//                               ________
//                                 |  |
//                                 ____
//
// 1. В rct1 содержится 1 вершина rct2:
//   в)                 г)
//        2_____           1______
//     1__|___ |           |  2__|___  
//     |  |  | |           |  |  |  |
//     |  ---|--           |  |  |  |
//     -------             ---|---  |
//                            -------
// 2. В rct1 содержатся 2 вершины rct2:
//   д)               e)              ж)
//       2___           1______          1_______
//     1_|__|__         |  2__|___       | 2--- |  
//     | |  | |         |  |  |  |       | |  | |
//     | |  | |         |  ---|---       --|--|--
//     | ---- |         -------            ----
//     --------
// 
// 4. В rct1 содержатся все вершины rct2:
//   з)
//    1______
//    | 2__ |
//    | | | |
//    | --- |
//    -------
// -2. В rct2 содержатся 2 вершины rct1:
//   и)                 к)
//    1_____                2------
//    2____|__            1_|___  |
//    |    | |            | |  |  |
//    |    | |            | |  |  |
//    ------ |            --|---  |
//    |______|              -------
// 
// -4. В rct2 содержатся все вершины rct1:
//   л)
//    1_______
//    |    | |
//    |    | |
//    |    | |
//    ------ |
//    |______2

// разобраться с взаимным расположением rct1 и rct2, переразбить их
// в rct1, rct2, tmp_rct и вернуть результат - сколько прямоугольников
// в итоге (1, 2 или 3) стало
RectsChangeType DivideRects(Rect& rct1, Rect& rct2, Rect& tmp_rct)
{
    Point pnt(Lft(rct2), Top(rct2));
    if( !rct1.Contains(pnt) ) // а),б),в),д),к)
    {
        pnt.y = Btm(rct2);
        if( !rct1.Contains(pnt) ) // а),б),к)
        {
            pnt.x = Rgt(rct1);
            pnt.y = Top(rct1);
            if( !rct2.Contains(pnt) ) // а),б)
            {
                if( rct2.lft<rct1.rgt && rct2.top<rct1.top &&
                    rct2.btm>rct1.btm ) // б)
                {
                    tmp_rct = rct2;
                    tmp_rct.top = rct1.btm;

                    rct2.btm = rct1.top;
                    return rctINS_NEW;
                }
                else // а)
                {
                    return rctNOTHING;
                }
            }
            else // к)
            {
                rct1.rgt = rct2.lft;
                return rctNOTHING;
            }
        }
        else // в),д)
        {
            pnt.x = Rgt(rct2);
            if( !rct1.Contains(pnt) ) // в)
            {
                tmp_rct.lft = rct1.rgt;
                tmp_rct.top = rct1.top;
                tmp_rct.rgt = rct2.rgt;
                tmp_rct.btm = rct2.btm;

                rct2.btm = rct1.top;
                return rctINS_NEW;
            }
            else // д)
            {
                rct2.btm = rct1.top;
                return rctNOTHING;
            }
        }
    }
    else                      // остальные 6)
    {
        pnt.x = Rgt(rct1);
        pnt.y = Btm(rct1);
        if( rct2.Contains(pnt) ) // г),и),л)
        {
            if( rct2.lft == rct1.lft ) // и),л)
            {
                if( rct2.top == rct1.top ) // л)
                {
                    // за время разбиения rct1 мог стать меньше, поэтому это общий случай
                    //ASSERT( (rct1.rgt == rct2.rgt) && (rct1.btm == rct2.btm) );
                    //return rctREMOVE;
                    if( rct1.btm < rct2.btm )
                    {
                        rct2.top = rct1.btm;
                        if( rct1.rgt < rct2.rgt )
                        {
                            tmp_rct.lft = rct1.rgt;
                            tmp_rct.top = rct1.top;
                            tmp_rct.rgt = rct2.rgt;
                            tmp_rct.btm = rct1.btm;
                            return rctINS_NEW;
                        }
                        return rctNOTHING;
                    }
                    else 
                    {
                        if( rct1.rgt < rct2.rgt )
                        {
                            rct2.lft = rct1.rgt;
                            return rctNOTHING;
                        }
                        return rctREMOVE;
                    }
                }
                else // и)
                {
                    rct1.btm = rct2.top;
                    return rctNOTHING;
                }
            }
            else // г)
            {
                if( rct1.top == rct2.top )
                {
                    // вырожденный случай
                    rct1.rgt = rct2.lft;
                    return rctNOTHING;
                }

                tmp_rct.lft = rct1.lft;
                tmp_rct.top = rct2.top;
                tmp_rct.rgt = rct2.lft;
                tmp_rct.btm = rct1.btm;

                rct1.btm = rct2.top;
                return rctINS_NEW;
            }
        }
        else // е),ж),з)
        {
            pnt.x = Rgt(rct2);
            pnt.y = Btm(rct2);
            if( rct1.Contains(pnt) ) // з)
            {
                return rctREMOVE;
            }
            else // е),ж)
            {
                pnt.x = Lft(rct2);
                if( rct1.Contains(pnt) ) // е)
                {
                    if( rct2.rgt == rct1.rgt )
                    {
                        // вырожденный случай
                        return rctREMOVE;
                    }
                    rct2.lft = rct1.rgt;
                    return rctNOTHING;
                }
                else // ж)
                {
                    if( rct2.btm == rct1.btm )
                    {
                        // вырожденный случай
                        return rctREMOVE;
                    }
                    rct2.top = rct1.btm;
                    return rctNOTHING;
                }
            }
        }
    }
    ASSERT(0);
    return rctNOTHING;
}

typedef std::vector<Rect>::iterator itr_typ;
typedef std::vector<BRect>::iterator bitr_typ;

void BRectCopy(std::vector<BRect>& brct_arr, std::vector<Rect>& rct_arr)
{
    brct_arr.reserve(rct_arr.size());
    for( itr_typ cur = rct_arr.begin(), end = rct_arr.end(); cur != end; ++cur )
        if( !cur->IsNull() )
            brct_arr.push_back(BRect(*cur));
}

bool BRectLess(const BRect& brct1, const BRect& brct2)
{
    if( !brct1.isEnabled ) // нулевые не нужны
        return false;
    if( !brct2.isEnabled )
        return true;

    ASSERT( !brct1.r.IsNull() && !brct2.r.IsNull() );

    Point a1 = brct1.r.A();
    Point a2 = brct2.r.A();
    // по левой точке
    if( a1 < a2 )
        return true;
    // среди одинаковых выбираем с наибольшей правой
    if( a1 == a2 && brct2.r.B() < brct1.r.B() )
        return true;
    return false;
}

int FindFirst(int cur, std::vector<BRect>& brct_arr)
{   
    bitr_typ beg = brct_arr.begin(), end = brct_arr.end();
    bitr_typ first = beg + cur;

    first = std::min_element(first, end, BRectLess);
    if( first == end || !first->isEnabled )
        return end-beg;
    return first-beg;
}

// проверочная функция - по выходу из DivideRects 
// не должно быть пустых прямоугольников
UNUSED_FUNCTION 
static bool HastEmptyRects(RectsChangeType res, Rect& rct1, Rect& rct2, Rect& tmp_rct)
{
    if( rct1.IsNull() )
        return true;
    if( res == rctNOTHING && rct2.IsNull() )
        return true;
    if( res == rctINS_NEW && (rct2.IsNull() || tmp_rct.IsNull()) )
        return true;
    return false;
}

} // namespace DividePriv


void ReDivideRects(std::vector<Rect>& rct_arr)
{
    using namespace DividePriv;
    // Используем последовательное вычитание прямоугольников с "верхней левой точкой"
    // из всех остальных, = O(n^2)
    // Замечание: вместо vector правильнее использовать list!

    std::vector<BRect> brct_arr;
    BRectCopy(brct_arr, rct_arr);
    rct_arr.clear();

    Rect tmp_rct;
    // так как в процессе можем добавить новые элементы, то итераторами не пользуемся
    for( int cur = 0, end = brct_arr.size() ; cur != end; ++cur )
    {
        // находим прямоугольник с самой левой точкой
        int first = FindFirst(cur, brct_arr);
        if( first == end )
            break;
        BRect* bcur = &brct_arr[cur];
        if( cur != first )
            std::swap(*bcur, brct_arr[first]);

        for( int cur2 = cur+1; cur2 != end; ++cur2 )
        {
            BRect& bcur2 = brct_arr[cur2];
            if( !bcur2.isEnabled )
                continue;

            RectsChangeType res = DivideRects(bcur->r, bcur2.r, tmp_rct);
            ASSERT( !HastEmptyRects(res, bcur->r, bcur2.r, tmp_rct) );
            switch( res )
            {
            case rctREMOVE:
                bcur2.isEnabled = false;
                break;
            case rctNOTHING:
                break;
            case rctINS_NEW:
                brct_arr.push_back(BRect(tmp_rct));
                bcur = &brct_arr[cur];
                end++;
                break;
            }
        }

        // сразу добавляем очередной результат
        rct_arr.push_back(bcur->r);
    }
}


