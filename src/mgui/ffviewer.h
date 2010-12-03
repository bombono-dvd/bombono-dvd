
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

struct FFViewer;

//
// VideoViewer - интерфейс для просмотра видео
//
typedef FFViewer VideoViewer;

void CheckOpen(VideoViewer& vwr, const std::string& fname);
void RGBOpen(VideoViewer& viewer, const std::string& fname = std::string());
Point DAspectRatio(VideoViewer& ffv);
double FrameFPS(FFViewer& ffv);
double FrameTime(VideoViewer& ffv, int fram_pos);
RefPtr<Gdk::Pixbuf> GetFrame(RefPtr<Gdk::Pixbuf>& pix, double time, FFViewer& ffv);
double FramesLength(FFViewer& ffv);
double Duration(FFViewer& ffv);

RefPtr<Gdk::Pixbuf> GetRawFrame(double time, FFViewer& ffv);

// в отличие от FFViewer открывать/закрывать самостоятельно
struct FFInfo: public boost::noncopyable
{
    AVFormatContext* iCtx;
                int  videoIdx;
              Point  vidSz; // первоначальный размер

          FFInfo();
    bool  IsOpened();
};

bool CanOpenAsFFmpegVideo(const char* fname, std::string& err_str);

struct FFViewer: public FFInfo
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


#endif // #ifndef __MGUI_FFVIEWER_H__

