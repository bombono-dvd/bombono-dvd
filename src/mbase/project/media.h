//
// mbase/project/media.h
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

#ifndef __MBASE_PROJECT_MEDIA_H__
#define __MBASE_PROJECT_MEDIA_H__

#include "const.h"
#include "object.h"
#include "archieve-fwd.h"

#include <mlib/ptr.h>

#include <boost/operators.hpp> // equality_comparable

namespace Project
{

template<class SimpleObj, class ParentObj = Object>
class PSO: public SimpleVisitorObject<SimpleObj, ParentObj, ObjVisitor>
{};


class Media: public Object
{
    public:
        std::string mdName; // название

                     void  Serialize(Archieve& ar);
                           // :TODO: добавить "папки"
                     bool  IsFolder() { return false; }
                           // тип медиа
    virtual   std::string  TypeString() = 0; 

    protected:

          virtual    void  SerializeImpl(Archieve& ar) = 0;
};

// объект, представляющий один (медиа)файл
class StorageMD: public Media
{
    public:

                     void  MakeByPath(const std::string& path, bool cnv_to_utf8 = true,
                                      const std::string& cur_dir = std::string());
    
        const std::string& GetPath() { return mdPath; }

          protected:

              std::string mdPath; // абсолютный адрес медиа

                           // в отличие от GetPath(), требуется только для 
                           // MakeByPath()
                     void  SetPath(const std::string& abs_path);
        virtual      void  SerializeImpl(Archieve& ar);
};

// typedef ptr::shared<Media> MediaItem;
// typedef ptr::shared<StorageMD> StorageItem;
typedef boost::intrusive_ptr<Media> MediaItem;
typedef boost::intrusive_ptr<StorageMD> StorageItem;

// изображение
class StillImageMD: public PSO<StillImageMD, StorageMD> // от StorageMD
{
    typedef StorageMD MyParent;
    public:
            
    virtual   std::string  TypeString() { return "Still Picture"; }
        virtual      void  SerializeImpl(Archieve& ar);
};

class VideoMD;
class VideoChapterMD;
// typedef ptr::shared<VideoChapterMD> ChapterItem;
// typedef ptr::shared<VideoMD> VideoItem;
typedef boost::intrusive_ptr<VideoChapterMD> ChapterItem;
typedef boost::intrusive_ptr<VideoMD> VideoItem;

enum PostActionType
{
    patAUTO = 0,      // (по умолчанию) наиболее ожидаемое действие
                      // при "существующей топологии проекта":
                      // - для меню это Loop
                      // - для видео - Previous Menu или Next Video
    patNEXT_TITLE,    // (для видео) следующий по списку
    patEXP_LINK,      // явная ссылка
    patPLAY_ALL,      // смотреть все
};

struct PostAction
{
    PostActionType  paTyp;
         MediaItem  paLink; // цель для patEXP_LINK

         PostAction(): paTyp(patAUTO) {}
};

//
// разрешенные разрешения для DVD
//
enum DVDDims
{
    dvdAUTO = 0, // определить автоматом из исходника
    dvd352s,
    dvd352,
    dvd704,
    dvd720
};

struct DVDTransData
{
    DVDDims  dd;
        int  vRate;

    DVDTransData(): dd(dvdAUTO), vRate(0) {}
};

struct SubtitleData
{
    std::string  pth;
           bool  defShow;
    std::string  encoding;

    SubtitleData(): defShow(false) {}
};

// видео
class VideoMD: public PSO<VideoMD, StorageMD> // от StorageMD
{
    typedef StorageMD MyParent;
    public:
        typedef std::vector<ChapterItem> ListType;
        typedef      ListType::iterator  Itr;

                void  AddChapter(ChapterItem chp);
                void  OrderByTime();

 virtual std::string  TypeString() { return "Video"; }
            ListType& List() { return chpLst; }
          PostAction& PAction() { return pAct; }

    virtual     void  SerializeImpl(Archieve& ar);
                
        DVDTransData  transDat;
        SubtitleData  subDat;

    protected:

        ListType  chpLst; // список глав
      PostAction  pAct;
};


// глава в видео
class VideoChapterMD: public PSO<VideoChapterMD, Media> // от Media
{
    public:
      VideoMD * const  owner;
               double  chpTime; // начало 

       static ChapterItem  CreateChapter(VideoMD* owner, double time)
                           {
                               ChapterItem chp(new VideoChapterMD(owner, time));
                               owner->AddChapter(chp);
                               return chp;
                           }

      virtual std::string  TypeString() { return "Chapter"; }
       virtual       void  SerializeImpl(Archieve& ar);
 
    protected:

                    VideoChapterMD(): owner(0), chpTime(0) {}

                    VideoChapterMD(VideoMD* own, double time)
                        : owner(own), chpTime(time) 
                    {}

                    friend VideoChapterMD* MakeEmptyChapter();
};

// для служебных целей (вроде поиска)
inline VideoChapterMD* MakeEmptyChapter() { return new VideoChapterMD; }

// :TRICKY: используются dynamic/static-приведения
// более универсальный механизм - посетитель ObjVisitor
ChapterItem IsChapter(MediaItem mi);
VideoItem   IsVideo(MediaItem mi);
StorageItem IsStorage(MediaItem mi);
bool IsStillImage(MediaItem mi);

inline StorageItem GetAsStorage(MediaItem mi)
{
    return ptr::static_pointer_cast<StorageMD>(mi);
}


inline VideoMD::ListType& GetList(ChapterItem chp) { return chp->owner->List(); }

// найти позицию главы в видео (owner)
VideoMD::Itr ChapterPos(ChapterItem chp);
inline int ChapterPosInt(ChapterItem chp) { return ChapterPos(chp) - GetList(chp).begin(); }

// получить имя файла, по которому открывать медиа
std::string GetFilename(StorageMD& smd);
std::string MakeAutoName(const std::string& str, int old_sz);

// удаление объекта
void DeleteMedia(MediaItem mi);
void DeleteChapter(VideoMD::ListType& chp_lst, VideoMD::Itr itr);

} // namespace Project

struct FrameTheme: public boost::equality_comparable<FrameTheme>
{
	   bool  isIcon;
    std::string  themeName;
    
    FrameTheme(const std::string& str = std::string(), bool is_icon = false)
	: themeName(str), isIcon(is_icon) {}
    
    bool operator==(const FrameTheme& rhs) const
    {
	return (isIcon == rhs.isIcon) && (themeName == rhs.themeName);
    }
};

#endif // #ifndef __MBASE_PROJECT_MEDIA_H__


