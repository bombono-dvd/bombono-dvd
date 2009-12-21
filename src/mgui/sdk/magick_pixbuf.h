//
// mgui/sdk/magick_pixbuf.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#include <Magick++.h>
#include <gdkmm/pixbuf.h>

#include <mgui/mguiconst.h>

// Вспомогательные функции для работы с Pixbuf и Magick::Image

// привести к формату RGBA (на самом деле у Cairo и GM форматы представления
// цвета совпадают при прозрачности = 1)
void AlignToPixbuf(Magick::Image& img);

// привести к Pixbuf, не изменяя содержимого
RefPtr<Gdk::Pixbuf> GetAsPixbuf(Magick::Image& img);

// преобразовать к Pixbuf в общем виде:
// - del_img         - результат возьмет во владение изображение
// - align_to_pixbuf - преобразовать в формат Pixbuf как в AlignToPixbuf()
RefPtr<Gdk::Pixbuf> CreatePixbufFromImage(Magick::Image* img, bool del_img = true, 
                                          bool align_to_pixbuf = true);


