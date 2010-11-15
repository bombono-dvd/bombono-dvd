//
// megg.cpp
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

//
// :COMMENT: 
//  "Пробный" исходник, для обкатки нового кода, алгоритмов и т.д.
// Полезной нагрузки нести не предусмотрен.
//

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <Magick++.h>

#include "myuvcontext.h"
#include "mmedia.h"
#include "megg.h"
#include "mmpeg.h"
#include "mstream.h"

/// Sample 1

static void ShowMpeg2Image(int width, int height,
                      int chroma_width, int chroma_height,
                      uint8_t * const * buf, int /*num*/)
{
    Magick::Image tmpImg;
    InitImage(tmpImg, width, height);

    // 1 так как записываем в YCbCr
    tmpImg.image()->colorspace = MagickLib::YCbCrColorspace; // YUVColorspace;

    // 2 перевод в Image
    int wdh = width;
    int hgt = height;
    MagickLib::PixelPacket* p = tmpImg.setPixels(0, 0, wdh, hgt);
    //if( !p )
    //    throw "Internal error: Can't setPixels()";
    ASSERT(p);

    Plane y_p(wdh, hgt, buf[0]);
    Plane u_p(chroma_width, chroma_height, buf[1]);
    Plane v_p(chroma_width, chroma_height, buf[2]);

    YuvContextIter iter(y_p, u_p, v_p);
    for( int y=0; y<hgt; y++, iter.YAdd() )
    {
        iter.XSet(0);
        for( int x=0; x<wdh; x++, iter.XAdd() )
        {
            //
            Magick::Color color( iter.yIter.constRes(), iter.uIter.constRes(), iter.vIter.constRes() );
            *p = color;
            p++;
        }
    }
    // 3
    tmpImg.syncPixels();
    // 4 перевод в RGB
    // :TODO: попробовать все делать в YCbCr, так быстрее получится
    tmpImg.colorSpace(MagickLib::RGBColorspace);

    //return tmpImg;
    tmpImg.display();
}


void decode_mpeg2 (char* current, char* end, MpegDecodec& dec)
{
    mpeg2dec_t * decoder = dec.Mpeg2Dec();
    const mpeg2_info_t * info;
    const mpeg2_sequence_t * sequence;
    mpeg2_state_t state;
    info = mpeg2_info (decoder);
    mpeg2_buffer (decoder, (uint8_t*)current, (uint8_t*)end);

    while( 1 )
    {
        state = mpeg2_parse (decoder);
        sequence = info->sequence;
        switch(state)
        {
        case STATE_BUFFER:
            return;
        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
            ASSERT(info->display_fbuf);
            if(info->display_fbuf)
                //save_pgm (sequence->width, sequence->height,
                ShowMpeg2Image (sequence->width, sequence->height,
                          sequence->chroma_width, sequence->chroma_height,
                          info->display_fbuf->buf, 0);
                ;
            break;
        default:
            break;
        }
    }
}

static void sample1 (FILE * mpgfile)
{
//     void TestStreams();
//     TestStreams();
//     return;
    char buffer[STRM_BUF_SZ];

//     TrackBuf buf;
//     MpegDecodec dec;
//     MpegDemuxer dmx;
//     while( 1 )
//     {
//
//     }
//
//     while( !dec.NextImage(buf) )
//     {
//         int size=0;
//         for( ; size=fread(buffer, 1, STRM_BUF_SZ, mpgfile), size ; )
//         {
//             //decode_mpeg2(buffer, buffer+size, dec);
//             dmx.Demux( (uint8_t*)buffer, (uint8_t*)buffer+size, dec );
//         }
//         if( !size )
//             break; // данные закончились
//
//
//
//     }


    {

        MpegDecodec dec;
        MpegDemuxer dmx;
        for( int size=fread(buffer, 1, STRM_BUF_SZ, mpgfile); size ; size=fread(buffer, 1, STRM_BUF_SZ, mpgfile) )
        {
            //decode_mpeg2(buffer, buffer+size, dec);
            //dmx.Demux( (uint8_t*)buffer, (uint8_t*)buffer+size, dec );
        } 
    }
}


#if 0
/// Sample 2

static void save_ppm (int width, int height, uint8_t * buf, int num)
{
    char filename[100];
    FILE * ppmfile;

    sprintf (filename, "%d.ppm", num);
    ppmfile = fopen (filename, "wb");
    if (!ppmfile) {
    fprintf (stderr, "Could not open file \"%s\".\n", filename);
    exit (1);
    }
    fprintf (ppmfile, "P6\n%d %d\n255\n", width, height);
    fwrite (buf, 3 * width, height, ppmfile);
    fclose (ppmfile);
}

static void sample2 (FILE * mpgfile)
{
#define BUFFER_SIZE 4096
    uint8_t buffer[BUFFER_SIZE];
    mpeg2dec_t * decoder;
    const mpeg2_info_t * info;
    mpeg2_state_t state;
    size_t size;
    int framenum = 0;

    decoder = mpeg2_init ();
    if (decoder == NULL) {
    fprintf (stderr, "Could not allocate a decoder object.\n");
    exit (1);
    }
    info = mpeg2_info (decoder);

    size = (size_t)-1;
    do {
    state = mpeg2_parse (decoder);
    switch (state) {
    case STATE_BUFFER:
        size = fread (buffer, 1, BUFFER_SIZE, mpgfile);
        mpeg2_buffer (decoder, buffer, buffer + size);
        break;
    case STATE_SEQUENCE:
        mpeg2_convert (decoder, mpeg2convert_rgb24, NULL);
        break;
    case STATE_SLICE:
    case STATE_END:
    case STATE_INVALID_END:
        if (info->display_fbuf)
        save_ppm (info->sequence->width, info->sequence->height,
              info->display_fbuf->buf[0], framenum++);
        break;
    default:
        break;
    }
    } while (size);

    mpeg2_close (decoder);
}
#endif //0

 
int mpeg2_sample_main (int argc, char ** argv)
{
    FILE * mpgfile;

    if(argc > 1)
    {
        mpgfile = fopen (argv[1], "rb");
        if(!mpgfile)
        {
            fprintf (stderr, "Could not open file \"%s\".\n", argv[1]);
            exit (1);
        }
    }
    else
        mpgfile = stdin;

    sample1 (mpgfile);

    return 0;
}

///////////////////////////////////////////////////////////////////////

// class StrStrm
// {
//     public:
//
//
//      const       std::string  Str() const { return strm.str(); }
//                  std::string  Str() { return strm.str(); }
//
//
//      const std::stringstream& Strm() const { return strm; }
//            std::stringstream& Strm() { return strm; }
//
//     protected:
//
//         std::stringstream strm;
// };
//
//
// template<class T>
// StrStrm& operator << (StrStrm& strm, const T t)
// {
//     strm.Strm() << t;
//     return strm;
// }
//
// #include <iostream>
//
// class tst
// {
// public:
//
//     int t;
//     int v;
//
// };
//
//
// void Tst1( tst& r )
// {
//     std::cout << r.t;
// }
//
// void FuncTST()
// {
// //     StrStrm s;
// //     (s << "A") << std::endl;
// //
// //
// //    test t;
//     Tst1(tst());
// }
// 
// 
// 
// 
// 

// int old_main(int argc, char *argv[])
// {
//     int in_fd  = 0; // stdin
//     int out_fd = 1; // out
//
//     if( argc<3+1 )
//     {
//         usage(argv[0]);
//         return 1;
//     }
//
//     // 1
//     if( strcmp(argv[1], "-") != 0 )
//     {
//         in_fd = open(argv[1], O_RDONLY);
//         if( in_fd == NO_HNDL )
//         {
//             usage(argv[0]);
//             return 1;
//         }
//     }
//
//     ListObj grp;
//
//     MovieMedia* mm = new MovieMedia(in_fd);
//     OutYuvContext out_strm(out_fd);
//     //int cnt = 0;
//     if( mm->Begin()  )
//     {
//         // 1 создаем структуру
//         ImgComposVis ivis(mm->Width(), mm->Height());
//
//         SimpleOverObj* soo = new SimpleOverObj( Rect(0, 0, mm->Width(), mm->Height()) );
//         grp.Ins(*soo);
//
//         StillPictMedia* spm = new StillPictMedia(argv[2]);
//         FrameThemeObj* fto = new FrameThemeObj(argv[3], Rect(100, 50, 450, 300));
//         grp.Ins(*fto);
//
//         //soo->SetMedia(mm);
//         soo->SetMedia(spm); //mm);
//         fto->SetMedia(mm); //spm);
//
//         out_strm.InitByIn(*mm);
//
//         // 2 сама работа
//         while( mm->GetFrame() )
//         {
//             grp.Accept(ivis);
//
//             ivis.CanvImg().display();
//             //StrmWriteImage(out_strm, ivis.CanvImg());
//
//             //CopyPlanesImage(out_strm, *mm);
//             //out_strm.PutFrame();
//
//             //Magick::Image& img = mm->GetImage();
//             //img.colorSpace(MagickLib::RGBColorspace);
//             //img.colorSpace(Magick::RGBColorspace);
//             //img.colorSpace(Magick::YCbCrColorspace);
//
//             //spm->GetImage().display();
//             //StrmWriteImage(out_strm, spm->GetImage());
//         }
//     }
//
//     return 0;
// }

// --frame frames/oval --size 300x200 --offset  400+10 --content ../Autumn.mpg --offset 200+200 --content ../Autumn.mpg  -r /dev/null -n 10 ../samples/AcerTX.jpg
