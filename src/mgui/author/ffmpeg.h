#ifndef __MGUI_AUTHOR_FFMPEG_H__
#define __MGUI_AUTHOR_FFMPEG_H__

#include <mgui/sdk/ioblock.h>

#include <mdemux/trackbuf.h>

#include <string>

namespace Project {

std::string FFmpegPostArgs(const std::string& out_fname, bool is_4_3, bool is_pal, 
                           const std::string& a_fname = std::string(), double a_shift = 0.);

struct FFmpegCloser
{
                GPid  pid;
                 int  inFd;
            ExitData& ed;
ptr::one<OutErrBlock> oeb;

     FFmpegCloser(ExitData& ed_): pid(NO_HNDL), inFd(NO_HNDL), ed(ed_) {}
    ~FFmpegCloser();
};

struct PPMWriter
{
         int  inFd;
    TrackBuf  pipeBuf;

    PPMWriter(int in_fd);
    void Write(RefPtr<Gdk::Pixbuf> img);
};

} // namespace Project

#endif // #ifndef __MGUI_AUTHOR_FFMPEG_H__

