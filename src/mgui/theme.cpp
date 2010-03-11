//
// mgui/theme.cpp
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

#include <mgui/_pc_.h>

#include "theme.h"

#include <mbase/project/theme.h>
#include <mbase/composite/component.h>
#include <mgui/project/thumbnail.h>

namespace Editor
{

ThemeCache::iterator ThemeCache::MakeFirst(ThemeCacheData& dat)
{
    push_front(dat);
    //io::cout << "ThemeCache: size = " << size() << ", name " << dat.thmName << io::endl;
    return begin();
}

const ThemeData& ThemeCache::GetTheme(const std::string& theme_name)
{
    // * поиск
    ThemeCache& lst = Instance();
    iterator beg_ = lst.begin(), end_ = lst.end();
    for( iterator itr = beg_; itr != end_; ++itr )
        if( itr->thmName == theme_name )
        {
            // переносим в начало
            //std::rotate(beg_, itr, itr+1);
            if( beg_ != itr )
            {
                ThemeCacheData dat(*itr);
                lst.erase(itr);

                itr = lst.MakeFirst(dat);
            }
            return itr->td;
        }

    // * уменьшаем кэш
    const int max_num = 3;
    if( (int)lst.size() >= max_num )
        lst.pop_back();

    // * вставляем
    fs::path pth = Project::FindThemePath(theme_name);
    
    ThemeCacheData dat;
    dat.thmName   = theme_name;
    dat.td.frameImg  = Gdk::Pixbuf::create_from_file((pth / "frame.png").string());
    dat.td.vFrameImg = Gdk::Pixbuf::create_from_file((pth / "vframe.png").string());

    return lst.MakeFirst(dat)->td;
}

const ThemeData& GetThumbTheme(const std::string& theme_name)
{
    typedef std::map<std::string, ThemeData> ThemeMapType;
    static ThemeMapType ThemeMap;
    ThemeMapType::iterator it = ThemeMap.find(theme_name);
    if( it != ThemeMap.end() )
        return it->second;

    const ThemeData& orig_theme = ThemeCache::GetTheme(theme_name);
    ThemeData res;
    Point sz = Project::Calc4_3Size(SMALL_THUMB_WDH);

    res.frameImg  = Project::MakeCopyWithSz(orig_theme.frameImg,  sz);
    res.vFrameImg = Project::MakeCopyWithSz(orig_theme.vFrameImg, sz);

    return ThemeMap[theme_name] = res;
}

FTOData::FTOData(DataWare& dw): owner(0)
{
    owner = dynamic_cast<FrameThemeObj*>(&dw);
    ASSERT( owner );
}

RefPtr<Gdk::Pixbuf> FTOData::GetPix()
{ 
    if( !pix )
        CompositeFTO(pix, *owner);
    return pix; 
}

RefPtr<Gdk::Pixbuf> CompositeWithFrame(RefPtr<Gdk::Pixbuf> obj_pix, const Editor::ThemeData& td)
{
    RefPtr<Gdk::Pixbuf> vfram_pix = td.vFrameImg;
    // 1 frame
    RefPtr<Gdk::Pixbuf> canv_pix = td.frameImg->copy();

    // 2 временное изображение с конечным медиа + с альфа от vframe
    RGBA::AddAlpha(obj_pix);
    RGBA::CopyAlphaComposite(obj_pix, vfram_pix, true);

    RGBA::AlphaComposite(canv_pix, obj_pix, PixbufBounds(canv_pix));
    return canv_pix;
}

void FTOData::CompositeFTO(RefPtr<Gdk::Pixbuf>& pix, FrameThemeObj& fto)
{
    const Editor::ThemeData& td = GetTheme(fto.Theme());

    RefPtr<Gdk::Pixbuf> obj_pix = CalcSource(Project::MIToDraw(fto), PixbufSize(td.vFrameImg));
    pix = CompositeWithFrame(obj_pix, td);
}

} // namespace Editor


namespace Project {

MediaItem MIToDraw(FrameThemeObj& fto)
{
    MediaItem mi = fto.PosterItem();
    if( !mi )
        mi = fto.MediaItem();

    return mi;
}

} // namespace Project

