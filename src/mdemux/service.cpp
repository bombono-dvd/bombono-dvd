//
// mdemux/service.cpp
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

#include "service.h"

namespace Mpeg 
{ 

SystemServiceDecor::SystemServiceDecor(Demuxer& dmx)
    : decorSvc(dmx.GetService()), ServiceSaver(dmx)
{
    dmx.SetService(this);
}
    
void SystemServiceDecor::GetData(Demuxer& dmx, int len)
{
    if( decorSvc )
        decorSvc->GetData(dmx, len);
}

void SystemServiceDecor::OnPack(Demuxer& dmx)
{
    if( decorSvc )
        decorSvc->OnPack(dmx);
}

VideoServiceDecor::VideoServiceDecor(Decoder& dcr)
    : decorSvc(dcr.GetService()), VServiceSaver(dcr)
{
    dcr.SetService(this);
}

void VideoServiceDecor::TagData(Decoder& dcr, VideoTag tag)
{
    if( decorSvc )
        decorSvc->TagData(dcr, tag);
}

} // namespace Mpeg


