//
// mgui/sdk/player_utils.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#include <mgui/_pc_.h>

#include <mgui/project/thumbnail.h>
#include <mgui/render/common.h>

#include "player_utils.h"
//#include "magick_pixbuf.h"

void SetOutputFormat(Mpeg::FwdPlayer& plyr, FrameOutputFrmt fof)
{
    plyr.VLine().GetParseContext().m2d.SetOutputFormat(fof);
}

void RGBOpen(Mpeg::FwdPlayer& plyr, const std::string& fname)
{
    SetOutputFormat(plyr, fofRGB);
    if( !fname.empty() )
        CheckOpen(plyr, fname);
}

static RefPtr<Gdk::Pixbuf> GetRawFrame(Mpeg::Player& plyr)
{
    RefPtr<Gdk::Pixbuf> res_pix;
    switch( plyr.VLine().GetParseContext().m2d.GetOutputFormat() )
    {
    case fofYCBCR:
        {
#if 0
            Magick::Image* img = new Magick::Image;
            //Mpeg::Log::FillImage(*img, plyr);
            FillImage(*img, plyr);
            //img.display();

            res_pix = CreatePixbufFromImage(img);
#endif // #if 0
            ASSERT(0); // не используем
        }
        break;
    case fofRGB:
        {
            Mpeg::SequenceData& seq = plyr.MInfo().vidSeq;
            res_pix = Gdk::Pixbuf::create_from_data(plyr.Data()[0], Gdk::COLORSPACE_RGB, false, 8, seq.wdh, seq.hgt, seq.wdh*3);
        }
        break;
    case fofRGBA:
        {
            Mpeg::SequenceData& seq = plyr.MInfo().vidSeq;
            res_pix = Gdk::Pixbuf::create_from_data(plyr.Data()[0], Gdk::COLORSPACE_RGB, true, 8,  seq.wdh, seq.hgt, seq.wdh*4);

            // приходится править альфу тут, потому что на каждую смену кадра
            // mpeg2dec переписывает ее
            for( unsigned char* beg = plyr.Data()[0], * end = plyr.Data()[0] + seq.hgt*seq.wdh*4; beg<end ; beg += 4 )
                beg[3] = 255;
        }
        break;
    default:
        ASSERT(0);
        break;
    }
    return res_pix;
}

// если не смог перейти, то вернет ноль
RefPtr<Gdk::Pixbuf> GetRawFrame(double time, Mpeg::FwdPlayer& plyr)
{
    RefPtr<Gdk::Pixbuf> img_pix;
    if( plyr.IsOpened() && plyr.SetTime(time + plyr.MInfo().begTime) )
        img_pix = GetRawFrame(plyr);
    return img_pix;
}

bool TryGetFrame(RefPtr<Gdk::Pixbuf>& pix, double time, Mpeg::FwdPlayer& plyr)
{
    bool res = false;
    RefPtr<Gdk::Pixbuf> img_pix = GetRawFrame(time, plyr);
    if( img_pix )
    {
        res = true;
        // заполняем кадр
        if( pix )
            RGBA::Scale(pix, img_pix);
        else
            pix = img_pix->copy();
    }
    return res;
}

RefPtr<Gdk::Pixbuf> GetFrame(RefPtr<Gdk::Pixbuf>& pix, double time, Mpeg::FwdPlayer& plyr)
{
    if( !TryGetFrame(pix, time, plyr) )
        FillEmpty(pix);
    return pix;
}


