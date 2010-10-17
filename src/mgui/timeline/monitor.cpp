//
// mgui/timeline/monitor.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#include "monitor.h"
#include "service.h"

#include <mgui/sdk/player_utils.h>
#include <mgui/render/common.h> // FillEmpty()

//namespace Editor
//{
//void FillAsEmptyMonitor(RefPtr<Gdk::Pixbuf> canv_pix, Gtk::Widget& wdg);
//}

namespace Timeline
{

Monitor::Monitor(): curPos(-1)
{}

DAMonitor::DAMonitor(): VideoArea(false)
{
    //SetOutputFormat(fofYCBCR);
    //SetOutputFormat(fofRGBA);
    //SetOutputFormat(plyr, fofRGB);
    RGBOpen(plyr);
}

Point DAMonitor::GetAspectRadio()
{
    return Mpeg::GetAspectRatio(plyr);
}

void DAMonitor::GetFrame(RefPtr<Gdk::Pixbuf> pix, int frame_pos)
{
    ::GetFrame(pix, GetFrameTime(plyr, frame_pos), plyr);
    // :TODO: все равно дизайн для монитора нужен!
    //if( !TryGetFrame(pix, GetFrameTime(plyr, frame_pos), plyr) )
    //    if( pix )
    //    {
    //        if( plyr.IsOpened() )
    //            FillEmpty(pix);
    //        else
    //            Editor::FillAsEmptyMonitor(pix, *this);
    //    }
}

void DAMonitor::UpdateCanvas()
{
    GetFrame(FramePixbuf(), curPos);
}

void DAMonitor::OnSetPos()
{
    UpdateCanvas();
    queue_draw_area( framPlc.lft, framPlc.top, framPlc.Width(), framPlc.Height() );
}

void DAMonitor::DoOnConfigure(bool is_update)
{
    if( is_update )
        UpdateCanvas();
}

} // namespace Timeline

