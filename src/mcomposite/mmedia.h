//
// mmedia.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007 Ilya Murav'jov
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

#ifndef __MMEDIA_H__
#define __MMEDIA_H__

#include <Magick++.h>

#include <mbase/composite/comp_vis.h>
#include "myuvcontext.h"
#include "mmpeg.h"

namespace Composition {

class Media: public Object //SO<Media>
{
    public:

            virtual      ~Media() { }

                          // получить очередной объект
   virtual Magick::Image& GetImage() = 0; 
};

class StillPictMedia: public SO<StillPictMedia, Media>
{
    public:

                       StillPictMedia(const char* img_path);
                       StillPictMedia(Magick::Image& img) : stillImg(img) { }

   virtual Magick::Image& GetImage() { return stillImg; }

    protected:


            Magick::Image  stillImg;
};

class MovieMedia: public SO<MovieMedia, Media>
{
    public:

   virtual Magick::Image& GetImage() { return tmpImg; }

                          // характеристики
               MovieInfo& Info() { return mInfo; }
                   Point  Size() { return mInfo.Size(); }

                          // прочитать начальные характеристики медиа
   virtual          bool  Begin() = 0;
                          // загрузить следующий кадр
   virtual          bool  NextFrame() = 0;
                          // сделать из кадра изображение
   virtual          void  MakeImage() = 0;

    protected:
            
                 Magick::Image  tmpImg; // промежуточное изображение
                     MovieInfo  mInfo;  // загружено после Begin()
};

class YuvMedia: public MovieMedia, public Planed::InYuvContext
{
    typedef InYuvContext MyYuvContext;
    public:

                          YuvMedia(int in_fd) : MyYuvContext(in_fd) { }
                          YuvMedia(const char* fpath);

                          // MovieMedia-
   virtual          bool  Begin();
   virtual          bool  NextFrame() { return MyYuvContext::GetFrame(); }
   virtual          void  MakeImage();

};

class MpegMedia: public MovieMedia
{
    public:

                          MpegMedia(io::stream* strm, SeqDemuxer* dmx);
                          
                          // MovieMedia-
   virtual          bool  Begin();
   virtual          bool  NextFrame();
   virtual          void  MakeImage();

   private:

       MpegPlayer mPlyr;
};

class MediaStrategy
{
    public:

                         MediaStrategy(): media(0) { }
        virtual         ~MediaStrategy() { Clear(); }

                   void  Clear()
                         {
                             if( media ) 
                                 delete media;
                             media = 0;
                         }

                  Media* GetMedia() { return media; }
                   void  SetMedia(Media* md) 
                         {
                             Clear();
                             media = md;
                         }
 
    protected:

        Media* media;

};

class MediaObj;

Media* GetMedia(MediaObj& mobj);
void SetMedia(MediaObj& mobj, Media* md);

} // namespace Composition

// инициализовать картинку перед работой 
// если создали Image конструктором по умолчанию, то перед использованием
// это обязательное действие
void InitImage(Magick::Image& img, int wdh, int hgt);


void TransferToImg(Magick::Image& tmp_img, int wdh, int hgt,
                           Planed::YuvContextIter& iter, bool convert_to_rgb = true);
void CopyImageToPlanes(Planed::OutYuvContext& out_cont, const Magick::Image& img);


// записать картинку в yuv-поток
void StrmWriteImage(Planed::OutYuvContext& out_strm, const Magick::Image& img);

#endif // #ifndef __MMEDIA_H__


