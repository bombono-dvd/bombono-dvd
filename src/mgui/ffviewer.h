//
// mgui/ffviewer.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#ifndef __MGUI_FFVIEWER_H__
#define __MGUI_FFVIEWER_H__

#include "mguiconst.h"

#include <mdemux/demuxconst.h>

#include <mlib/geom2d.h>
#include <mlib/tech.h>

C_LINKAGE_BEGIN
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
C_LINKAGE_END

#include <boost/noncopyable.hpp>

// иначе импорт только DVD-совместимых файлов 
#define FFMPEG_IMPORT_POLICY 1

struct FFViewer;

//
// VideoViewer - интерфейс для просмотра видео
//
typedef FFViewer VideoViewer;

struct FFData;
double FrameFPS(FFData& ffv);
double Duration(FFData& ffv);
Point DAspectRatio(FFData& ffv);

void CheckOpen(VideoViewer& vwr, const std::string& fname);
void RGBOpen(VideoViewer& viewer, const std::string& fname = std::string());
double FrameTime(VideoViewer& ffv, int fram_pos);
RefPtr<Gdk::Pixbuf> GetFrame(RefPtr<Gdk::Pixbuf>& pix, double time, FFViewer& ffv);
double FramesLength(FFViewer& ffv);

RefPtr<Gdk::Pixbuf> GetRawFrame(double time, FFViewer& ffv);

// в отличие от FFViewer открывать/закрывать самостоятельно
struct FFData: public boost::noncopyable
{
    AVFormatContext* iCtx;
                int  videoIdx;
              Point  vidSz; // первоначальный размер

          FFData();
    bool  IsOpened();
};

// обертка для удобства пользования
struct FFInfo: public FFData
{
     FFInfo();
     FFInfo(const std::string& fname);
    ~FFInfo();
};

bool CanOpenAsFFmpegVideo(const char* fname, std::string& err_str);

struct FFViewer: public FFData
{
                     // время текущего кадра
             double  curPTS;
             double  prevPTS;

            AVFrame  srcFrame;
            AVFrame  rgbFrame;
            uint8_t* rgbBuf;
         SwsContext* rgbCnvCtx;


                    FFViewer();
                   ~FFViewer();

              bool  Open(const char* fname, std::string& err);
              bool  Open(const char* fname);
              void  Close();
};

AVCodecContext* GetVideoCtx(FFData& ffv);

// однострочная настройка
std::string PrefContents(const char* fname);

#endif // #ifndef __MGUI_FFVIEWER_H__

