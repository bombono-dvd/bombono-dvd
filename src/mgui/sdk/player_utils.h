//
// mgui/sdk/player_utils.h
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

#ifndef __MGUI_SDK_PLAYER_H__
#define __MGUI_SDK_PLAYER_H__

#include <mdemux/player.h>
#include <mdemux/decoder.h>
#include <mgui/img_utils.h>

// установить до первого применения
void SetOutputFormat(Mpeg::FwdPlayer& plyr, FrameOutputFrmt fof);

// вернуть кадр только на чтение (если не смог перейти, то вернет ноль)
RefPtr<Gdk::Pixbuf> GetRawFrame(double time, Mpeg::FwdPlayer& plyr);
// если !pix, то возвратит изображение оригинального размера
RefPtr<Gdk::Pixbuf> GetFrame(RefPtr<Gdk::Pixbuf>& pix, double time, Mpeg::FwdPlayer& plyr);
bool TryGetFrame(RefPtr<Gdk::Pixbuf>& pix, double time, Mpeg::FwdPlayer& plyr);

inline double GetFrameTime(Mpeg::Player& plyr, int fram_pos)
{
    return plyr.MInfo().FrameTime(fram_pos, false);
}

#endif // #ifndef __MGUI_SDK_PLAYER_H__
