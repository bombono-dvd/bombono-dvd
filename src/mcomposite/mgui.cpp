//
// mcomposite/mgui.cpp
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

// 
// Устаревший код
// 

//
// mgui/meditor.[h,cpp]
// 

void LoadByCmdLine(EditorRegion& mea, int argc, char** argv);

// получить и подготовить первые изображения для показа в редакторе
class PrepareVis: public Comp::MovieVisitor
{
    typedef Comp::MovieVisitor MyParent;
    public:
                   PrepareVis(bool need_open): needOpen(need_open) {}

    virtual  void  Visit(Comp::MovieMedia& mm);
    virtual  void  Visit(FrameThemeObj& fto);
    virtual  void  Visit(SimpleOverObj& soo);

    protected:
            bool needOpen;
};


// typedef Comp::Media OldMedia;
//
// static OldMedia* CreateEditorMedia(MediaItem mi)
// {
//     OldMedia* md = 0;
//     if( StorageItem si = IsStorage(mi) )
//     {
//         bool is_movie;
//         md = CmdOptions::CreateMedia(si->GetPath().c_str(), is_movie);
//     }
//
//     if( !md )
//         md = CmdOptions::CreateBlackImage(Point(10, 10));
//
//     return md;
// }

void PrepareVis::Visit(Comp::MovieMedia& mm)
{
    bool is_good = !needOpen || mm.Begin();
    ASSERT( is_good );

    if( !mm.NextFrame() )
        Error("Bad movie! It has no frames.");
    mm.MakeImage();
}

void PrepareVis::Visit(FrameThemeObj& fto)
{
    MyParent::Visit(fto);

    // подготовить для отображения в редакторе
    AlignToPixbuf( FrameImg(fto) );
    AlignToPixbuf( VFrameImg(fto) );
    AlignToPixbuf( Comp::GetMedia(fto)->GetImage() );
}

void PrepareVis::Visit(SimpleOverObj& soo)
{
    MyParent::Visit(soo);

    AlignToPixbuf( Comp::GetMedia(soo)->GetImage() );
}

namespace {

class CmdParser: public BaseCmdParser
{
    typedef BaseCmdParser MyParent;
    public:
                    CmdParser(int argc, char *argv[]): MyParent(argc, argv) {}

    protected:

   virtual  Comp::Object* CreateTextObj(const char* text);
};

Comp::Object* CmdParser::CreateTextObj(const char* text)
{
    TextObj* obj = 0;
    if( text )
    {
        obj = new TextObj;
        if( tOpts.fontDsc )
            obj->SetFontDesc(tOpts.fontDsc);

        MEdt::DefInit(obj, tOpts.plc.A(), text);
    }
    return obj;
}

} // namespace

void LoadByCmdLine(EditorRegion& mea, int argc, char** argv)
{
    DoBeginVis beg(mea);

    CmdParser bcp(argc, argv);
    bcp.CreateComposition(beg, true);

    mea.SetByMovieInfo(beg.PatInfo());

    PrepareVis fpv(false);
    mea.Accept(fpv);
}

