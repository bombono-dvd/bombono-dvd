//
// mdemux/tests/cutmpeg.cpp
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

#include <algorithm>

#include <mlib/read_stream.h>

#include <mdemux/seek.h>

using namespace Mpeg;

struct SavePackets: public SystemServiceDecor
{
    typedef SystemServiceDecor MyParent;
    typedef std::vector<Chunk> ChunkList;
    ChunkList chunks;

                      SavePackets(Demuxer& dmx);

        virtual void  GetData(Demuxer& dmx, int len);
};

SavePackets::SavePackets(Demuxer& dmx)
    : MyParent(dmx)
{}
    
void SavePackets::GetData(Demuxer& dmx, int len)
{
    io::pos chk_pos = dmx.ObjStrm().tellg();
    if( !chunks.empty() && (chk_pos <= chunks.back().extPos) )
        chunks.clear();
    chunks.push_back( Chunk(chk_pos, len) );

    MyParent::GetData(dmx, len);
}

bool LessChunk(const Chunk& lft, const Chunk& rgt)
{
    return lft.extPos < rgt.extPos;
}

int main(int argc, char *argv[])
{
    if( argc<4 )
    {
        io::cerr << "Usage: \t cutmpeg <secs> src-file(.mpg, .vob) dst-file" << io::endl;
        io::cerr << "\t Cut mpeg by time <secs>, from src-file(.mpg, .vob) to dst-file" << io::endl;
        return 1;
    }

    double time = atof(argv[1]);
    PlayerData pd;
    io::stream& src_strm = pd.srcStrm;
    src_strm.open(argv[2], iof::in);
    if( !src_strm )
    {
        io::cout << "Cant open file " << argv[2] << "." << io::endl;
        return 2;
    }
    io::stream dst_strm(argv[3], iof::out);
    if( !dst_strm )
    {
        io::cout << "Cant write file " << argv[3] << "." << io::endl;
        return 2;
    }

    MediaInfo& inf     = pd.mInf;
    ParseContext& cont = pd.prsCont;
    if( !inf.GetInfo(cont) )
    {
        io::cerr << inf.ErrorReason() << io::endl;
        return 2;
    }
    time += inf.begTime;

    VideoLine vl(cont);
    SavePackets sp(cont.dmx);
    if( !Mpeg::MakeForTime(vl, time, inf) )
    {
        io::cerr << "Cant seek to time: " << time << " sec" << io::endl;
        return 3;
    }

    FrameList::Itr itr = vl.GetFrames().PosByTime(time);
    ASSERT( itr.Pos() != -1 );
    FrameData::ChunkList& chunks = itr->dat;
    ASSERT( !chunks.empty() );
    Chunk& last_chk = chunks.back();

    io::pos end_pos = last_chk.extPos + last_chk.len;
    //io::cout << "End of frame: " << end_pos << io::endl;

    typedef SavePackets::ChunkList::iterator c_iterator;
    c_iterator c_it = std::lower_bound(sp.chunks.begin(), sp.chunks.end(), Chunk(end_pos, 0), LessChunk);
    ASSERT( c_it != sp.chunks.begin() );
    Chunk& chk = *(c_it-1);
    io::pos cut_pos = chk.extPos + chk.len;
    ASSERT( (chk.extPos < end_pos) && (end_pos <= cut_pos) );
    //io::cout << "End of packet: " << cut_pos << io::endl;

    src_strm.seekg(inf.begPos);
    ReadStream(MakeWriter(dst_strm), src_strm, cut_pos);
    return 0;
}
