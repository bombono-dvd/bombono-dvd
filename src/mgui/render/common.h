//
// mgui/render/common.h
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

#ifndef __MGUI_RENDER_COMMON_H__
#define __MGUI_RENDER_COMMON_H__

#include <mgui/theme.h>
#include <mgui/design.h>


//
// LimitedRecursiveCall() - ограничение на рекурсию
// 

class Incrementer
{
    public:
            Incrementer(int& v): val(v) { ++val; }
           ~Incrementer() { --val; }
    private:
        int& val;
};

const int RenderRecurseLimit = 3;

template<typename Res>
Res LimitedRecursiveCall(boost::function<Res(bool)> fnr, int& RecurseDepth, 
                         int max_depth = RenderRecurseLimit)
{
    //static int RecurseDepth = 0;
    Incrementer inc(RecurseDepth);
    // ограничение на рекурсивный вызов (для меню)
    return fnr(RecurseDepth > max_depth);
}

// вызов рекурсивного функтора-получателя снимка с ограничением
// (при превышении выдаст прозрачное изображение)
typedef boost::function<RefPtr<Gdk::Pixbuf>()> ShotFunctor;
RefPtr<Gdk::Pixbuf> GetLimitedRecursedShot(ShotFunctor s_fnr, int& rd);
// для отрисовки "вживую"
RefPtr<Gdk::Pixbuf> GetInteractiveLRS(ShotFunctor s_fnr);

inline RefPtr<Gdk::Pixbuf> Make11Image()
{
    return CreatePixbuf(1, 1, true);
}
// однопиксельное изображение цвета clr
RefPtr<Gdk::Pixbuf> MakeColor11Image(int clr);

// сделать картинку "пустой"
inline void FillEmpty(RefPtr<Gdk::Pixbuf>& pix)
{
    if( !pix )
        pix = Make11Image();
    pix->fill(BLACK_CLR);
}

#endif // __MGUI_RENDER_COMMON_H__

