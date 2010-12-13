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

DVDTransData GetRealTransData(VideoItem vi);

} // namespace Project

#endif // #ifndef __MGUI_PROJECT_VIDEO_H__

