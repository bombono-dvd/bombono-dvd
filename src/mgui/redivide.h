//
// mgui/redivide.h
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

#ifndef __MGUI_REDIVIDE_H__
#define __MGUI_REDIVIDE_H__

#include <vector>

#include <mlib/geom2d.h>

namespace DividePriv
{

enum RectsChangeType 
{
    rctREMOVE,          // удалить (rct2)
    rctNOTHING,         // просто продолжить дальше
    rctINS_NEW,         // добавить новый элемент (tmp_rct)
};

// разобраться с взаимным расположением rct1 и rct2, переразбить их
// в rct1, rct2, tmp_rct и вернуть результат - сколько прямоугольников
// в итоге (1, 2 или 3) стало
RectsChangeType DivideRects(Rect& rct1, Rect& rct2, Rect& tmp_rct);

// вспомогательные вещи
struct BRect
{
    Rect r;
    bool isEnabled;

            BRect(): isEnabled(false) {}
            BRect(const Rect& rct, bool is_enabled = true): 
                r(rct), isEnabled(is_enabled) {}
};
bool BRectLess(const BRect& brct1, const BRect& brct2);

}

// переразбить набор прямоугольников так, чтобы 
// они не перекрывали друг друга, покрывая ровно ту же площадь
void ReDivideRects(std::vector<Rect>& rct_arr);

#endif // __MGUI_REDIVIDE_H__

