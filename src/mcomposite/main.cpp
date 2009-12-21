//
// main.cpp
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
#include <Magick++.h>

#include <mbase/composite/component.h>

#include "mutils.h"
#include "mstring.h"
#include "mtheme.h"
#include "mvisitor.h"
#include "moptions.h"

//#include "megg.h"

int main (int argc, char *argv[])
{
    Comp::ListObj grp;
    DoBeginVis beg(grp);

    //int res = CreateCompositionFromCmd(argc, argv, beg, false);
    BaseCmdParser bcp(argc, argv);
    int res = bcp.CreateComposition(beg, false);
    if( res )
        return res;

    MovieInfo& info = beg.PatInfo(); 
    Point sz = info.Size();

    Planed::OutYuvContext out_strm(beg.outFd); //OUT_HNDL);
    out_strm.SetInfo(info);

    ImgComposVis ivis(sz.x, sz.y);
    (FrameCounter&)ivis = beg.framCnt;

    if( ivis.IsDone() )
    {
        io::cerr << "Checking only mode (0 frames): good." << io::endl;
        return 0;
    }

    // сама работа
    for( ivis.First(grp) ; !ivis.IsDone(); ++ivis )
    {
        ivis.CanvImg().display();
        StrmWriteImage(out_strm, ivis.CanvImg());
    }

    return 0;
}

