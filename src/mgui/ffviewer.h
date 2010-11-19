
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

struct FFViewer
{
    AVFormatContext* iCtx;
                int  videoIdx;
                     // время текущего кадра
             double  curPTS;

            AVFrame  srcFrame;
            AVFrame  rgbFrame;
            uint8_t* rgbBuf;
         SwsContext* rgbCnvCtx;
              Point  vidSz; // лучше бы с rgbCnvCtx брать, но
                            // не дают


                    FFViewer();
                   ~FFViewer();

              bool  Open(const char* fname, std::string& err);
              bool  Open(const char* fname);
              bool  IsOpened();
              void  Close();
};


#endif // #ifndef __MGUI_FFVIEWER_H__

