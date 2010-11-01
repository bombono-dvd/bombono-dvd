//
// mgui/img-factory.h
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

#ifndef __MGUI_IMG_FACTORY_H__
#define __MGUI_IMG_FACTORY_H__

#include "img_utils.h"

// получить изображение из ресурсов
// данный функционал преследует те же цели, что и GtkIconFactory + gtk_image_new_from_stock(),
// но удобнее в плане получения именно в формате GdkPixbuf
RefPtr<Gdk::Pixbuf> GetFactoryImage(const std::string& img_str);
// наложить изображение emblem_str на pix, в левом нижнем углу
void StampEmblem(RefPtr<Gdk::Pixbuf> pix, const std::string& emblem_str);

//void CheckEmblem(RefPtr<Gdk::Pixbuf> pix, RefPtr<Gdk::Pixbuf> emblem);
RefPtr<Gdk::Pixbuf> GetCheckEmblem(RefPtr<Gdk::Pixbuf> pix, const std::string& emblem_str);

#endif // #ifndef __MGUI_IMG_FACTORY_H__

