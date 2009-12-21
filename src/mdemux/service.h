//
// mdemux/service.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2008 Ilya Murav'jov
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

#ifndef __MDEMUX_SERVICE_H__
#define __MDEMUX_SERVICE_H__

#include "mpeg.h"
#include "mpeg_video.h"

namespace Mpeg 
{ 

class ServiceSaver
{
    public:
            ServiceSaver(Demuxer& dmx): demuxer(dmx), service(dmx.GetService()),
                isStrict(dmx.IsStrict()) 
            {}
           ~ServiceSaver()
            {
                demuxer.SetStrict(isStrict);
                demuxer.SetService(service);
            }
    protected:
            Demuxer& demuxer;
    private:
               bool  isStrict;
            Service* service;
};

class VServiceSaver
{
    public:
            VServiceSaver(Decoder& dcr): decoder(dcr), service(dcr.GetService()) {}
           ~VServiceSaver()
            { decoder.SetService(service); }
    protected:
            Decoder& decoder;
       VideoService* service;
};

//////////////////////////////////////////////////////////

// декоратор System-сервиса
class SystemServiceDecor: public FirstVideoSvc, ServiceSaver
{
    typedef FirstVideoSvc MyParent;
    public:
                      SystemServiceDecor(Demuxer& dmx);

        virtual void  GetData(Demuxer& dmx, int len);
        virtual void  OnPack(Demuxer& dmx);

    private:
        Service* decorSvc;
};

//////////////////////////////////////////////////////////

// декоратор Video-сервиса
class VideoServiceDecor: public VideoService, public VServiceSaver
{
    public:
                   VideoServiceDecor(Decoder& dcr);
                 
     virtual void  TagData(Decoder& dcr, VideoTag tag);
    protected:
        VideoService* decorSvc;
};

} // namespace Mpeg

#endif // __MDEMUX_SERVICE_H__

