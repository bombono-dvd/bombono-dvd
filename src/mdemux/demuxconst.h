//
// mdemux/demuxconst.h
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

#ifndef __MDEMUX_DEMUXCONST_H__
#define __MDEMUX_DEMUXCONST_H__

#include <mlib/tech.h>
#include <mlib/const.h>

const double INV_TS = -1.0;

inline bool IsTSValid(double ts) 
{
    return ts >= 0.;
}

namespace Mpeg { 

// численные ограничения декодирования
const int DecodeBlockSize = 100000;  // 100kb, парсим столько за раз
const int BoundDecodeSize = 800000;  // размер данных, читаемых в начале и в конце
                                     // при открытии файла
const double MaxContinueTime = 10.0; // сек, макс. отступ от тек. времени к нужному
const int MaxFrameListLength = 400;  // держим не больше столько кадров, чтоб в памяти занимать
                                     // не больше 100kb на один плейер (не считая самих картинок,...)



} // namespace Mpeg

#endif // __MDEMUX_DEMUXCONST_H__

