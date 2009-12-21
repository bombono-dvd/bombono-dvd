//
// myuvcontext.cpp
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

#include "mtech.h"
#include "myuvcontext.h"
#include "mconst.h"

YuvInfo::~YuvInfo()
{
    Clear();
}

void YuvInfo::Clear()
{
    if( isInit )
    {
        y4m_fini_frame_info(&frameInfo);
        y4m_fini_stream_info(&streamInfo);
    }
    y4m_init_stream_info(&streamInfo);
    y4m_init_frame_info(&frameInfo);

    isInit = false;
}

void YuvInfo::Copy(const YuvInfo& info)
{
    Clear();
    if( info.IsInit() )
    {
        y4m_copy_stream_info(&streamInfo, &info.streamInfo);
        y4m_copy_frame_info(&frameInfo, &info.frameInfo);

        isInit = info.isInit;
    }
}

namespace Planed { 

YuvContext::~YuvContext()
{
    Clear();
}

void YuvContext::Clear()
{
    if( isInit )
    {
        delete [] yuv[0];
        delete [] yuv[1];
        delete [] yuv[2];
    }
    MyParent::Clear();
}

static void InitPlane(Plane& plane, int wdh, int hgt, unsigned char** raw_yuv = 0 )
{
    plane.x = wdh;
    plane.y = hgt;
    plane.data = new unsigned char[plane.Size()];

    if( raw_yuv )
        *raw_yuv = plane.data;
}

bool InYuvContext::Begin()
{
    Clear();

    if( y4m_read_stream_header(inFd, &streamInfo) != Y4M_OK )
        return false;

    if( y4m_si_get_plane_count(&streamInfo) != 3 )
        return false;

//     yPlane.x = Width();
//     yPlane.y = Height();
//     yPlane.data = yuv[0] = new unsigned char[yPlane.Size()];
    InitPlane(yPlane, Width(), Height(), &(yuv[0]));

//     uPlane.x = y4m_si_get_plane_width(&streamInfo, 1);
//     uPlane.y = y4m_si_get_plane_height(&streamInfo, 1);
//     uPlane.data = yuv[1] = new unsigned char[uPlane.Size()];
    InitPlane(uPlane, 
              y4m_si_get_plane_width(&streamInfo, 1), 
              y4m_si_get_plane_height(&streamInfo, 1), 
              &(yuv[1]));

//     vPlane.x  = y4m_si_get_plane_width(&streamInfo, 2);
//     vPlane.y = y4m_si_get_plane_height(&streamInfo, 2);
//     vPlane.data = yuv[2] = new unsigned char[vPlane.Size()];
    InitPlane(vPlane, 
              y4m_si_get_plane_width(&streamInfo, 2), 
              y4m_si_get_plane_height(&streamInfo, 2), 
              &(yuv[2]));

    isInit = true;
    return isInit;
}

bool YuvContext::CheckRes(int yuv_res)
{
    if( yuv_res == Y4M_ERR_EOF )
        return false;
    else if( yuv_res == Y4M_OK )
        return true;

    Error("GetFrame: Bad yuv-stream");
    return false;
}

bool InYuvContext::GetFrame()
{
    int res = y4m_read_frame(inFd, &streamInfo, &frameInfo, yuv);
//     if( res == Y4M_ERR_EOF )
//         return false;
//     else if( res == Y4M_OK )
//         return true;
//
//     throw "GetFrame: Bad yuv-stream";
    return CheckRes(res);
}


// Держим как рабочий (и работающий) пример - какие характеристики есть у видео-потока
#if 0
static void InitYuvHeaders(YuvContext& out_ycont, const YuvContext& in_ycont,
                           bool is_frame_info_copy = false)
{
    const y4m_stream_info_t& src_strm = in_ycont.streamInfo;
    y4m_stream_info_t& dst_strm = out_ycont.streamInfo;

    const y4m_frame_info_t& src_frame = in_ycont.frameInfo;
    y4m_frame_info_t& dst_frame = out_ycont.frameInfo;

    // 0 инициализация
    y4m_init_stream_info(&dst_strm);
    y4m_init_frame_info(&dst_frame);

    // 1 размеры
    y4m_si_set_width(&dst_strm,  y4m_si_get_width(&src_strm));
    y4m_si_set_height(&dst_strm, y4m_si_get_height(&src_strm));

    // 2 частота кадров (framerate)
    y4m_si_set_framerate(&dst_strm, y4m_si_get_framerate(&src_strm));

    // 3 хроматический режим (хроморежим, chroma)
    y4m_si_set_chroma(&dst_strm, y4m_si_get_chroma(&src_strm));

    // 4 развертка (interlace)
    y4m_si_set_interlace(&dst_strm,  y4m_si_get_interlace(&src_strm));

    // 5 аспект точек (aspect)
    y4m_si_set_sampleaspect(&dst_strm, y4m_si_get_sampleaspect(&src_strm));

    // 6 дополнительные тэги yuv4mpeg (xtags )
    y4m_copy_xtag_list(y4m_si_xtags(&dst_strm), 
                       y4m_si_xtags(&const_cast<y4m_stream_info_t&>(src_strm)));
    if( is_frame_info_copy )
    {
        // :TODO: разобраться с тэгами кадров
        y4m_copy_xtag_list(y4m_fi_xtags(&dst_frame), 
                           y4m_fi_xtags(&const_cast<y4m_frame_info_t&>(src_frame)));
    }
}
#endif 

bool GetMovieInfo(MovieInfo& mi, const YuvContext& y_c)
{
    if( !y_c.IsInit() )
        return false;
//     mi.Clear();
//
//     const y4m_stream_info_t& src_strm = y_c.streamInfo;
//     const y4m_frame_info_t& src_frame = y_c.frameInfo;
//     YuvContext& y_c_ = const_cast<YuvContext&>(y_c); // :KLUDGE: лень XPlane() к const приводить
//
//     // 1 размеры
//     mi.ySz = (Point&)(y_c_.YPlane());
//     mi.uSz = (Point&)(y_c_.UPlane());
//     mi.vSz = (Point&)(y_c_.VPlane());
//
//     y4m_copy_stream_info(&mi.streamInfo, &src_strm);
//     y4m_copy_frame_info(&mi.frameInfo, &src_frame);
//
//     mi.Init();
    mi.Copy(y_c);
    // размеры
    YuvContext& y_c_ = const_cast<YuvContext&>(y_c); // :KLUDGE: лень XPlane() к const приводить
    mi.ySz = (Point&)(y_c_.YPlane());
    mi.uSz = (Point&)(y_c_.UPlane());
    mi.vSz = (Point&)(y_c_.VPlane());

    return true;
}

static bool SetMovieInfo(YuvContext& y_c, const MovieInfo& mi)
{
    if( !mi.IsInit() )
        return false;
    y_c.Clear();

    y4m_copy_stream_info(&y_c.streamInfo, &mi.streamInfo);
    y4m_copy_frame_info(&y_c.frameInfo, &mi.frameInfo);

    return true;
}

// инициализировать по входному потоку
void OutYuvContext::SetInfo(MovieInfo& mi)
{
    // 1 инициализация структур yuv4mpeg
//     Clear();
//     InitYuvHeaders(*this, in_ycont);
    if( !SetMovieInfo(*this, mi) )
        Error("OutYuvContext::SetInfo(): not valid input yuv!");

    // 2 инициализация плоскостей
    InitPlane(yPlane, mi.ySz.x, mi.ySz.y, &(yuv[0]));
    InitPlane(uPlane, mi.uSz.x, mi.uSz.y, &(yuv[1]));
    InitPlane(vPlane, mi.vSz.x, mi.vSz.y, &(yuv[2]));

    // 3
    isInit = true;
}

void OutYuvContext::Clear()
{
    MyParent::Clear();
    isHdrWrit = false;
}

bool OutYuvContext::PutFrame()
{
    if( !isHdrWrit )
    {
        isHdrWrit = true;
        CheckRes( y4m_write_stream_header(outFd, &streamInfo) );
    }

    int res = y4m_write_frame(outFd, &streamInfo, &frameInfo, yuv);
    return CheckRes(res);
}

} // namespace Planed
