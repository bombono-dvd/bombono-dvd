//
// moptions.cpp
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

#include <getopt.h>

#include <Magick++.h>

#include <mlib/ptr.h>
#include <mbase/composite/component.h>

#include "mconst.h"
#include "mstring.h"
#include "moptions.h"
#include "mmedia.h"
#include "mmpeg_demux.h"

#include <string.h> // strcmp()

static void usage(const char* app_nm)
{
    const char* p = strrchr(app_nm, '/');
    if( !p ) 
        p = app_nm;
    else 
        p++; // без /

    io::cerr << "usage: " << p << " [-g, --geometry geom] [-f, --frame frame_dir] [-s, --size size]"
                    " [-c, --content media_file] [-o, --offset offset] [-n, --count frame_number]" 
                    " [-b, --base] [ -r, --result res_video_in_yuv] background";

}

namespace CmdOptions
{

static Comp::Media* TryMpegMedia(const char* fpath, bool& is_movie)
{
    ptr::one<SeqDemuxer> dmx;
    if( const char* ext = Str::GetFileExt(fpath) )
    {
        if( strcmp(ext, "m2v") == 0 )
            dmx = new IDemuxer;
        else if( strcmp(ext, "mpeg") == 0 || strcmp(ext, "mpg") == 0 )
            //dmx = new Mpeg_legacy::MpegDemuxer;
            dmx = new MpegSeqDemuxer;
        else if( strcmp(ext, "dva") == 0 )
            dmx = new Mpeg_legacy::TSMpegDemuxer;
    }

    if( dmx )
    {
        ptr::one<io::stream> strm = new io::stream(fpath);
        if( *strm ) // файл открылся
        {
            Comp::MpegMedia* mm = new Comp::MpegMedia(strm.release(), dmx.release());
            is_movie = true;
            return mm;
        }
    }
    return 0;
}

Comp::Media* CreateMedia(const char* fpath, bool& is_movie)
{
    Comp::Media* md = NULL;
    is_movie = false;
    // определим, что перед нами
    if( Str::IsYuvMedia(fpath) )
    {
//         YuvMedia* mm = NULL;
//         if( strcmp( fpath, "-" ) == 0 )
//             mm = new YuvMedia(IN_HNDL);
//         else
//             mm = new YuvMedia(fpath);
        Comp::YuvMedia* mm = new Comp::YuvMedia( OpenFileAsArg(fpath, true) );

        if( mm->inFd == NO_HNDL ) // не смогли открыть
        {
            delete mm;
            return NULL;
        }
        is_movie = true;
        md = mm;
    }
    else if( Str::IsPictureMedia(fpath) )
        md = new Comp::StillPictMedia(fpath);
    else 
        md = TryMpegMedia(fpath, is_movie);

    return md;
}

static Comp::Media* CreateMedia(const char* fpath, TempObjs& t_opts)
{
    bool is_movie;
    Comp::Media* md = CreateMedia(fpath, is_movie);
    if( is_movie )
        t_opts.SetBaseMovie(static_cast<Comp::MovieMedia*>(md));

    if( !md )
        Error("Cant open one of source files!");
    return md;
}

Comp::StillPictMedia* CreateBlackImage(const Point& sz)
{
    Magick::Image black_img(Magick::Geometry(sz.x, sz.y), Magick::Color("#000000"));
    return new Comp::StillPictMedia(black_img);
}

} // CmdOptions

// bool ParseOptions(int argc, char *argv[], DoBeginVis& beg)
bool BaseCmdParser::ParseOptions(DoBeginVis& beg)
{
    Comp::ListObj& l_obj = beg.lstObj;
    FrameCounter& counter = beg.framCnt;
    // вначале фон, а медиа к нему в конце найдем
    SimpleOverObj* soo = new SimpleOverObj;
    l_obj.Ins(*soo);

    // CmdOptions::TempObjs t_opts;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help",     no_argument,       0, 'h'},
            {"content",  required_argument, 0, 'c'},
            {"geometry", required_argument, 0, 'g'},
            {"frame",    required_argument, 0, 'f'},
            {"size",     required_argument, 0, 's'},
            {"offset",   required_argument, 0, 'o'},
            {"count",    required_argument, 0, 'n'},
            {"base",     no_argument,       0, 'b'},
            {"result",   required_argument, 0, 'r'},

            // текст (CmdParser)
            {"text",      required_argument, 0, 't'},
            {"font_desc", required_argument, 0, 'd'},

            {0, 0, 0, 0}
        };
        int option_index = 0;
        char c = (char)getopt_long(argCnt, argVars, "hc:g:f:s:o:n:br:t:d:",
                         long_options, &option_index);

        if(c == -1)
            break;

        switch(c)
        {
        case 0:   // we dont use such things
        case '?':
        case 'h':
        default:  // ха, getopt() глючит
            usage(argVars[0]);
            return false;
        case 'c': // content
            {
                if( !tOpts.frameName || !*tOpts.frameName )
                {
                    io::cerr << "No frame is set before content option: "
                                "--content " << optarg << io::endl;
                    usage(argVars[0]);
                    return false;
                }
                if( tOpts.plc.Size().x == 0 )
                {
                    io::cerr << "Null frame size for --content " << optarg << io::endl;
                    usage(argVars[0]);
                    return false;
                }

                Comp::Media* md = CmdOptions::CreateMedia(optarg, tOpts);
                FrameThemeObj* fto = new FrameThemeObj(tOpts.frameName, tOpts.plc);
                //fto->SetMedia(md);
                Comp::SetMedia(*fto, md);

                l_obj.Ins(*fto);
                break;
            }
        case 'g': // geometry
            if( !Str::GetGeometry(tOpts.plc, optarg) )
            {
                io::cerr << "Invalid --geometry option: " << optarg << io::endl;
                usage(argVars[0]);
                return false;
            }
            break;
        case 'f': // frame
            tOpts.frameName = optarg;
            break;
        case 's': // size
            {
                Point sz;
                if( !Str::GetSize(sz, optarg) )
                {
                    io::cerr << "Invalid --size option: " << optarg << io::endl;
                    usage(argVars[0]);
                    return false;
                }
                Rect& plc = tOpts.plc;

                plc.rgt = plc.lft+sz.x;
                plc.btm = plc.top+sz.y;
            }
            break;
        case 'o': // offset
            {
                Point off;
                if( !Str::GetOffset(off, optarg) )
                {
                    io::cerr << "Invalid -o(--offset) option: " << optarg << io::endl;
                    usage(argVars[0]);
                    return false;
                }
                Rect& plc = tOpts.plc;

                plc += off - Point(plc.lft, plc.top);
            }
            break;
        case 'n': // count, frame number
            {
                long n;
                if( !Str::GetLong(n, optarg) )
                {
                    io::cerr << "Invalid -n(--count) option: " << optarg << io::cerr;
                    usage(argVars[0]);
                    return false;
                }
                counter.Set(n);
            }
            break;
        case 'b': // base movie
            tOpts.SetNextBase();
            break;
        case 'r': // result, output movie
            beg.outFd = CmdOptions::OpenFileAsArg(optarg, false);
            break;
        case 't': // text
            {
                Comp::Object* obj = CreateTextObj(optarg);
                if( obj )
                    l_obj.Ins(*obj);
            }
            break;
        case 'd': // font desc
            tOpts.fontDsc = optarg;
            break;
        }
    }

    // остаток - один аргумент - фон
    Comp::Media* back_md = NULL;
    if( optind == argCnt )
    {
        io::cerr << "Warning: background is lacked!\n"
                 << "Using black-color as background.\n";

//         Magick::Image black_img(Magick::Geometry(0,0,10,10), Magick::Color("#000000"));
//         back_md = new StillPictMedia(black_img);
        back_md = CmdOptions::CreateBlackImage(Point(10, 10));
    }
    else if( optind+1 == argCnt )
    {
        back_md = CmdOptions::CreateMedia(argVars[optind], tOpts);
    }
    else
    {
        io::cerr << "Too many non-optional arguments\n";
        usage(argVars[0]);
        return false;
    }
    if( !back_md )
        Error("Error: ParseOptions()");
    //soo->SetMedia(back_md);
    Comp::SetMedia(*soo, back_md);

    // устанавливаем MovieMedia-основу
    if( !tOpts.mm )
    {
        io::cerr << "No movie found: one of sourses must be movie" << io::endl;
        usage(argVars[0]);
        return false;
    }

    beg.basMedia = tOpts.mm;
    beg.bckSoo = soo;
    return true;
}


// int CreateCompositionFromCmd(int argc, char *argv[], DoBeginVis& beg, bool is_abort)
int BaseCmdParser::CreateComposition(DoBeginVis& beg, bool is_abort)
{
    int res = 0;
    try
    {
        if( !ParseOptions(beg) )
        {
            io::cerr << "There was an error while parsing options." << io::endl;
            throw 1;
        }

        if( !beg.Begin() )
        {
            io::cerr << "There was an error while opening sources (different framerate?)." << io::endl;
            throw 2;
        }
    }
    catch(int new_res)
    {
        res = new_res;
    }

    if( res && is_abort )
        abort();
    return res;
}
