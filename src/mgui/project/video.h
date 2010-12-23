#ifndef __MGUI_PROJECT_VIDEO_H__
#define __MGUI_PROJECT_VIDEO_H__

#include <mbase/project/media.h>
#include <mlib/range/any_range.h>

namespace Project
{

fe::range<VideoItem> AllVideos();
// список видео "IsTransVideo() == true"
fe::range<VideoItem> AllTransVideos();

// vi && RequireTranscoding(vi)
bool IsTransVideo(VideoItem vi);

// расчет параметров транскодирования автоматом
DVDTransData GetRealTransData(VideoItem vi);
DVDTransData DVDDims2TDAuto(DVDDims dd);

Point DVDDimension(DVDDims dd);
DVDDims CalcDimsAuto(VideoItem vi);
//
int OutAudioNum(int i_anum);

// кэш для расчетов по транскодированию и т.д.
struct RTCache
{
      bool  isCalced;

      bool  reqTrans;
    double  duration;
     Point  vidSz;
     Point  dar;
       int  audioNum; // число аудиоканалов

    RTCache(): isCalced(false) {}
};

RTCache& GetRTC(VideoItem vi);
io::pos CalcTransSize(RTCache& rtc, int vrate);

} // namespace Project

// kbit/s 
// 448 - умолчание в ffmpeg для -target *-dvd
// выбор возможен из ff_ac3_bitrate_tab
const int TRANS_AUDIO_BITRATE = 320;

#endif // #ifndef __MGUI_PROJECT_VIDEO_H__

