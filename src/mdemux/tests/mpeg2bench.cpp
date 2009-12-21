//
// mdemux/mpeg2bench.cpp
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

#include <mdemux/tests/_pc_.h>
#include <mdemux/player.h>

int main(int argc, char *argv[])
{
    if( argc<2 || argc>3 )
    {
        io::cerr << "Usage: \t mpeg2bench [-f|-r] file.mpg" << io::endl;
        return 1;
    }
    const char* fname = argv[1];

    char opt = 'f'; // default
    if( argc == 3 )
    {
        const char* opt_str = fname;
        fname = argv[2];

        if( opt_str[0] != '-' )
        {
            io::cerr << "Not an option: " << opt_str << io::endl;
            return 1;
        }
        opt = opt_str[1];
    }

    Mpeg::FwdPlayer plyr(fname);
    ASSERT( plyr.IsOpened() );
    switch( opt )
    {
    case 'f':
        // full pass
        {
            io::cerr << "Do full pass: " << fname << io::endl;
            for( plyr.First(); !plyr.IsDone(); plyr.Next() )
                ;
        }
        break;
    case 'r':
        // rewind, 1 per 5 sec
        {
            io::cerr << "Do rewind (1 by 5 frames): " << fname << io::endl;
            Mpeg::MediaInfo& inf = plyr.MInfo();
            double len = inf.FrameLength()*5;
            io::cerr << "Interval: " << len << " sec" << io::endl;

            for( double time = inf.begTime; time<inf.endTime; time += len )
            {
                bool res = plyr.SetTime(time);
                ASSERT( res );
            }
        }
        break;
    default:
        io::cerr << "Unknown option: " << opt << io::endl;
        return 1;
    }
    return 0;
}
