//
// mbase/project/menu.h
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

#ifndef __MBASE_PROJECT_MENU_H__
#define __MBASE_PROJECT_MENU_H__

#include <mlib/sdk/misc.h> // MenuParams
#include "media.h"

namespace Project
{

class MenuMD;
class MenuItemMD;

typedef boost::intrusive_ptr<MenuMD> Menu;
typedef boost::intrusive_ptr<MenuItemMD> MenuItem;

const double DEF_MOTION_DURATION = 15; // секунд

struct MotionData
{
      bool  isMotion; // меню будет анимационным
    double  duration; // длительность в секундах
 MediaItem  audioRef; // отсюда берем аудио (видео или глава!)
      bool  isStillPicture; // "неподвижное видео"

    MotionData(): isMotion(false), duration(DEF_MOTION_DURATION), isStillPicture(false) {}
};

// меню
class MenuMD: public PSO<MenuMD, Media> // от Media
{
    typedef Media MyParent;
    public:
        typedef std::vector<MenuItem> ListType;
        typedef   ListType::iterator  Itr;

                           MenuMD();

               MenuParams& Params() { return mPrms; }
                 ListType& List()   { return itmLst; }
                MediaItem& BgRef()  { return bgRef; }
              std::string& Color()  { return color; }
               MotionData& MtnData(){ return mtnData; }


    virtual   std::string  TypeString() { return "Menu"; }
    virtual          void  SerializeImpl(Archieve& /*ar*/);

    protected:
            MenuParams  mPrms;
              ListType  itmLst; // список глав
             MediaItem  bgRef;  // фон меню
           std::string  color;
            MotionData  mtnData;
};

Menu IsMenu(MediaItem mi);

// пункт меню
class MenuItemMD: public Media
{
    public:
                           MenuItemMD(): owner(0) {}

                           // положение на холсте
                     Rect& Placement() { return itmPlc; }
                MediaItem& Ref() { return itmRef; }

       virtual       void  SerializeImpl(Archieve& ar);

    protected:
                     void  SetOwner(MenuMD* own)
                           {
                               ASSERT( !owner );
                               owner = own;
                               owner->List().push_back(this);
                           }  

                   MenuMD* owner;
                     Rect  itmPlc;
                MediaItem  itmRef; // ссылка по этому пункту
};

class FrameItemMD: public PSO<FrameItemMD, MenuItemMD>// от MenuItemMD
{
    typedef MenuItemMD MyParent;
    public:

                           FrameItemMD(MenuMD* own)
                           { SetOwner(own); }

      virtual std::string  TypeString() { return "Frame Item"; }
       virtual       void  SerializeImpl(Archieve& ar);

              std::string& Theme() { return themeStr; }
                MediaItem& Poster() { return posterRef; }
       
    protected:
              std::string  themeStr;
                MediaItem  posterRef;
};

class TextItemMD: public PSO<TextItemMD, MenuItemMD>// от MenuItemMD
{
    typedef MenuItemMD MyParent;
    public:

                           TextItemMD(MenuMD* own): isUnderlined(false)
                           { SetOwner(own); }

      virtual std::string  TypeString() { return "Text Item"; }

              std::string& FontDesc()     { return fontDsc; }
                     bool& IsUnderlined() { return isUnderlined; }
              std::string& Color()        { return color; }

       virtual       void  SerializeImpl(Archieve& ar);

    protected:

            std::string  fontDsc;
                   bool  isUnderlined;
            std::string  color;
};

std::string Media2Ref(MediaItem mi);
MediaItem Ref2Media(const std::string& ref);

// при загрузке устанавливаем ссылки в конце, когда все объекты уже загружены
typedef std::vector<std::pair<MediaItem*, std::string> > MediaRefArr;

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_MENU_H__

