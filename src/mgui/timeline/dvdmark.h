
#ifndef __MGUI_TIMELINE_DVDMARK_H__
#define __MGUI_TIMELINE_DVDMARK_H__

#include <mgui/mguiconst.h>
#include <mbase/project/media.h>

namespace Timeline
{

extern Project::VideoItem CurrVideo;

typedef Project::VideoMD::ListType DVDArrType;
typedef Project::ChapterItem       DVDMark;

inline DVDArrType& DVDMarks()
{
    ASSERT( CurrVideo );
    return CurrVideo->List();
}

struct DVDMarkData
{ 
                           int  pos; 
   CR::RefPtr<CR::ImageSurface> thumbPix;
}; 

// получение данных главы для монтажного окна
inline DVDMarkData& GetMarkData(const DVDMark& ci)
{ return ci->GetData<DVDMarkData>(); }
inline DVDMarkData& GetMarkData(int idx)
{ return GetMarkData(DVDMarks()[idx]); }

} // namespace Timeline

#endif // #ifndef __MGUI_TIMELINE_DVDMARK_H__

