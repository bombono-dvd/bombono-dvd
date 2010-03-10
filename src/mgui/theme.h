//
// mgui/theme.h
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

#ifndef __MGUI_THEME_H__
#define __MGUI_THEME_H__

#include <list>

#include <mlib/patterns.h>
#include <mbase/project/media.h>
#include <mbase/composite/comp_vis.h>
#include "img_utils.h"

namespace Editor
{

struct ThemeData
{
    RefPtr<Gdk::Pixbuf> frameImg;
    RefPtr<Gdk::Pixbuf> vFrameImg;
};

struct ThemeCacheData
{
    std::string  thmName;
      ThemeData  td;
};

class ThemeCache: protected std::list<ThemeCacheData>, public Singleton<ThemeCache>
{
    typedef std::vector<ThemeCacheData> MyParent;
    public:
                      // получить тему по названию
    static const ThemeData& GetTheme(const std::string& theme_name);

    protected:
            iterator  MakeFirst(ThemeCacheData& dat);
};

// вариант темы малого размера - для миниатюр
const ThemeData& GetThumbTheme(const std::string& theme_name);

//
// FTOData - "обрамление" картинки
// 
class FTOData: public DWConstructorTag
{
    public:
                       FTOData(DataWare& dw);
         virtual      ~FTOData() {}
 
                 void  CompositeFTO(RefPtr<Gdk::Pixbuf>& canv_pix, FrameThemeObj& fto);

   RefPtr<Gdk::Pixbuf> GetPix();
                 void  ClearPix() { ClearRefPtr(pix); }

    protected:

        RefPtr<Gdk::Pixbuf> pix;
             FrameThemeObj* owner;

    virtual const Editor::ThemeData& GetTheme(std::string& thm_nm) = 0; 
    virtual      RefPtr<Gdk::Pixbuf> CalcSource(Project::MediaItem mi, const Point& sz) = 0; 
};

// Обрамление obj_pix
// размеры obj_pix должны быть равны td.vFrameImg
RefPtr<Gdk::Pixbuf> CompositeWithFrame(RefPtr<Gdk::Pixbuf> obj_pix, const ThemeData& td);

// выбор: что отрисовывать в рамке
Project::MediaItem MIToDraw(FrameThemeObj& fto);

} // namespace Editor

#endif // __MGUI_THEME_H__

