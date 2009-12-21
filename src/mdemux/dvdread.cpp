//
// mdemux/dvdread.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009 Ilya Murav'jov
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

#include "dvdread.h"
#include "util.h"

#include <mlib/lambda.h>
#include <mlib/string.h>
#include <mlib/filesystem.h>

#include <map>
#include <set>
#include <limits>

namespace DVD {

std::string VobFName(VobPos& pos, const std::string& suffix)
{
    using Mpeg::set_hms;
    return (str::stream("Video") << set_hms() << int(pos.Vts()) 
            << "-" << set_hms() << pos.VobId() << suffix << ".vob").str();
}

typedef boost::function<void(int, double)> VobTimeFnr;
static void PushVobTime(const VobTimeFnr& fnr, int vob_id, double tm)
{
    if( fnr && (vob_id != -1) )
        fnr(vob_id, tm);
}

bool operator < (const VobPtr& v1, const VobPtr& v2)
{
    return v1->pos < v2->pos;
}

typedef std::set<VobPtr> VobSet;

static void CalcVobTime(VobSet& vset, VobPtr tmp_vob, int vob_id, double tm)
{
    tmp_vob->pos.VobId() = vob_id;
    VobSet::iterator itr = vset.find(tmp_vob);
    if( itr != vset.end() )
        (**itr).tmLen = tm;
}

static int BCD2Dec(uint8_t bcd)
{
	return (bcd >> 4)*10 + (bcd & 0x0f);
}

// COPY_N_PASTE_REALIZE, ifoPrint_time()

static double DTime2Sec(dvd_time_t *dtime) 
{
#define CHECK_VALUE(arg)     \
    if( !(arg) )             \
    {                        \
        ASSERT(0);           \
        return 0.;           \
    }

    CHECK_VALUE((dtime->hour>>4) < 0xa && (dtime->hour&0xf) < 0xa);
    CHECK_VALUE((dtime->minute>>4) < 0x7 && (dtime->minute&0xf) < 0xa);
    CHECK_VALUE((dtime->second>>4) < 0x7 && (dtime->second&0xf) < 0xa);
    CHECK_VALUE((dtime->frame_u&0xf) < 0xa);
#undef CHECK_VALUE

    double rate = 0.;
    //printf("%02x:%02x:%02x.%02x",
    //       dtime->hour,
    //       dtime->minute,
    //       dtime->second,
    //       dtime->frame_u & 0x3f);
    switch( (dtime->frame_u & 0xc0) >> 6 )
    {
    case 1:
        rate = 25.0;
        break;
    case 3:
        rate = 29.97;
        break;
    default:
        if (dtime->hour == 0 && dtime->minute == 0 
            && dtime->second == 0 && dtime->frame_u == 0)
            ; //rate = "no";
        else
            //rate = "(please send a bug report)";
            ASSERT(0);
        break;
    } 
    //printf(" @ %s fps", rate);

    double res = 3600*BCD2Dec(dtime->hour) + 60*BCD2Dec(dtime->minute) + BCD2Dec(dtime->second);
    if( rate )
        res += BCD2Dec(dtime->frame_u & 0x3f) / rate;
    return res;
}

void CalcVobTimes(ifo_handle_t* ifo, const VobTimeFnr& fnr)
{
    // для подсчета длительности VOB
    typedef std::pair<uint16_t, uint16_t> vc_t;
    typedef std::map<vc_t, double> TimeMap;
    TimeMap tmap;

    pgcit_t* pgcs = ifo->vts_pgcit;
    vc_t tmp;
    for( int i=0; i<pgcs->nr_of_pgci_srp; i++ )
    {
        pgc_t* pgc = pgcs->pgci_srp[i].pgc;
        for( int j=0; j<pgc->nr_of_cells; j++ )
        {
            double this_tm = DTime2Sec(&pgc->cell_playback[j].playback_time);
            cell_position_t& ct = pgc->cell_position[j];
            tmp.first  = ct.vob_id_nr;
            tmp.second = ct.cell_nr;

            double& tm = tmap[tmp];
            tm = std::max(tm, this_tm);
        }
    }

    int vob_id = -1;
    double tm  = 0.;
    for( TimeMap::iterator itr = tmap.begin(), end = tmap.end(); itr != end; ++itr )
    {
        TimeMap::value_type& val = *itr;
        uint16_t this_vobid = val.first.first;
        if( vob_id != this_vobid )
        {
            PushVobTime(fnr, vob_id, tm);
            tm = val.second;
            vob_id = this_vobid;
        }
        else
            tm += val.second;
    }
    PushVobTime(fnr, vob_id, tm);
}

VobPtr MakeVob(uint8_t vts, uint16_t vob_id)
{
    VobPtr vob(new Vob);
    VobPos& pos = vob->pos;
    pos.Vts()   = vts;
    pos.VobId() = vob_id;
    return vob;
}

static bool IsPAL(video_attr_t& v_attr)
{
    return v_attr.video_format == 1;
}

static Point VideoSize(video_attr_t& v_attr)
{
    int h = !IsPAL(v_attr) ? 480 : 576 ;
    int w = 0;
    switch( v_attr.picture_size ) 
    {
    case 0: w = 720; break;
    case 1: w = 704; break;
    case 2: w = 352; break;
    case 3: w = 352; h /= 2; break;
    default:
        ;// printf("(please send a bug report) ");
    }
    return Point(w, h);
}

// see ifoPrint_video_attributes()
bool IsPAL(ifo_handle_t* vmg_ifo)
{
    //io::cout << "video_format " << vmg_ifo->vmgi_mat->vmgm_video_attr.video_format << io::endl;
    return IsPAL(vmg_ifo->vmgi_mat->vmgm_video_attr);
}

class IfoCloser
{
    public:
        IfoCloser(ifo_handle_t* ifo_): ifo(ifo_) {}
       ~IfoCloser() { ifoClose(ifo); }
    private:
        ifo_handle_t* ifo;
};

void FillVobArr(VobArr& dvd_vobs, dvd_reader_t* dvd)
{
    ifo_handle_t* vmg_ifo = ifoOpen(dvd, 0); // VMG
    ASSERT(vmg_ifo);
    IfoCloser ifo_wrp(vmg_ifo);

    // see ifoPrint_video_attributes()
    dvd_vobs.isPAL = IsPAL(vmg_ifo);

    VobPtr tmp_vob(new Vob);
    VobPos& tmp_pos = tmp_vob->pos;
    for( int i=1, vts_cnt=vmg_ifo->vmgi_mat->vmg_nr_of_title_sets; i <= vts_cnt; i++ )
    {
        tmp_pos.Vts() = i;
        ifo_handle_t* ifo = ifoOpen(dvd, i);
        if( !ifo ) // не судьба
            continue;
        IfoCloser ifo_wrp(ifo);

        video_attr_t& v_attr = ifo->vtsi_mat->vts_video_attr;
        Point sz(VideoSize(v_attr));
        AspectFormat aspect = (v_attr.display_aspect_ratio == 3) ? af16_9 : af4_3 ;

        //PrintSortedCAdt(ifo);
        //CalcVobTimes(ifo, &PrintVobTime);

        VobSet vset; // временный массив, потом переливаем в dvd_vobs
        VobSet::iterator end = vset.end();
        c_adt_t* cptr = ifo->vts_c_adt;
        for( int j=0, cnt=CAdtSize(cptr); j<cnt; j++ )
        {
            cell_adr_t& cell = cptr->cell_adr_table[j];
            if( !(cell.start_sector <= cell.last_sector) ) // проверка на всякий случай
                continue;

            tmp_pos.VobId() = cell.vob_id;

            // обнаружено, что в таблице Cell Address Table данные vob'ов идут
            // подряд; поэтому можно ускориться - сохраняя последний редактируемый
            // Vob избавимся от поиска
            VobPtr vob;
            VobSet::iterator itr = vset.find(tmp_vob);
            if( itr != end )
                vob = *itr;
            else
            {
                vob = MakeVob(tmp_pos.Vts(), tmp_pos.VobId());
                vob->sz     = sz;
                vob->aspect = aspect;
                vset.insert(vob);
            }

            Vob::LocationArr& arr = vob->locations;
            uint32_t off = arr.size() ? arr.back().End() : 0;
            arr.push_back(Vob::Part(cell.start_sector, cell.last_sector+1, off));
        }

        // просчет длительности vob'ов
        CalcVobTimes(ifo, bl::bind(&CalcVobTime, boost::ref(vset), tmp_vob, bl::_1, bl::_2));

        // добавляем в общий массив
        dvd_vobs.insert(dvd_vobs.end(), vset.begin(), vset.end());
    }
}

VobPtr FindVob(VobArr& dvd_vobs, uint8_t vts, uint16_t vob_id)
{
    VobPtr tmp_vob = MakeVob(vts, vob_id);
    VobPtr vob = *std::lower_bound(dvd_vobs.begin(), dvd_vobs.end(), tmp_vob);
    ASSERT( tmp_vob->pos == vob->pos );
    
    return vob;    
}

#define ABLOCK_SECT 128
#define ABLOCK_LEN (DVD_VIDEO_LB_LEN * ABLOCK_SECT)

static void TryDVDReadBlocks(dvd_file_t* file, int off, size_t cnt, char* buf)
{
    int real_cnt = DVDReadBlocks(file, off, cnt, (unsigned char*)buf);
    if( (int)cnt != real_cnt )
        throw std::runtime_error( (str::stream() << real_cnt << 
                                   " != DVDReadBlocks(" << cnt << ")").str() );
}

// размер буфера должен соответствовать читаемому диапазону
void ReadDVD(dvd_file_t* file, char* buf, 
             uint32_t beg_sec, uint32_t end_sec, const ReadFunctor& fnr)
{
    for( uint32_t sec = beg_sec; sec < end_sec; sec += ABLOCK_SECT )
    {
        int cnt = std::min(ABLOCK_SECT, int(end_sec - sec));
        TryDVDReadBlocks(file, sec, cnt, buf);

        buf = fnr(buf, cnt << 11); // *DVD_VIDEO_LB_LEN
        if( !buf )
            break;
    }
}

void ExtractVob(ReadFunctor& fnr, VobPtr vob, dvd_reader_t* dvd)
{
    VobPos& pos = vob->pos;
    DVDFile dfile(dvd, pos.Vts());
    dvd_file_t* vobs = dfile.file;

    char buf[ABLOCK_LEN]; // переносим блоками по 256kb
    Vob::LocationArr& arr = vob->locations;
    for( int i=0; i<(int)arr.size(); i++ )
    {
        Vob::Part& be = arr[i];
        ReadDVD(vobs, buf, be.beg, be.end, fnr);
    }
}

void ExtractVob(VobPtr vob, const std::string& dir_path, dvd_reader_t* dvd)
{
    std::string fname = VobFName(vob->pos);
    io::stream out_strm(AppendPath(dir_path, fname).c_str(), iof::out);
    ReadFunctor fnr = MakeWriter(out_strm);

    ExtractVob(fnr, vob, dvd);
}

bool LessPart(const Vob::Part& p1, const Vob::Part& p2)
{
    return p1.off < p2.off;
}

const std::streamoff ReadStreambuf::streamsize_limit = std::numeric_limits<streamsize>().max();

void VobStreambuf::xsgetnImpl(char* s, streamsize n)
{
    ASSERT( n >= 0 );
    if( n )
    {
        uint32_t beg_sec = pos >> 11;
        int      beg_off = pos & 0x7ff;
        pos_type end_pos = pos + pos_type(n);
        uint32_t end_sec = end_pos >> 11;
        int      end_off = end_pos & 0x7ff;
        if( end_off )
            end_sec += 1;
    
        Vob::LocationArr& arr = vob->locations;
        Vob::Part tmp(0, 0, beg_sec);
        Vob::LocationArr::iterator end = arr.end();
        Vob::LocationArr::iterator itr = std::lower_bound(arr.begin(), end, tmp, LessPart);
        if( (itr == end) || (itr->off != beg_sec) )
        {
            --itr;
            ASSERT( itr->off < beg_sec );
        }
        
        char* s_end = s + n;
        ReadFunctor shift_fnr = MakeBufShifter();
        for( ; (itr != end) && (itr->off < end_sec); ++itr )
        {
            Vob::Part& part = *itr;
            bool is_beg_part = part.off <= beg_sec;
            uint32_t b_sec = part.beg + (is_beg_part ? beg_sec - part.off  : 0);
            bool is_end_part = end_sec  <= part.End();
            uint32_t e_sec = part.end + (is_end_part ? end_sec - part.End(): 0);

            char buf[DVD_VIDEO_LB_LEN]; // 2048
            if( is_beg_part && beg_off )
            {
                // читаем начальный сектор в буфер, а затем нужную часть копируем в целевой
                TryDVDReadBlocks(content.file, b_sec, 1, buf);

                int cnt = (int)std::min(streamsize(DVD_VIDEO_LB_LEN-beg_off), n);
                memcpy(s, buf+beg_off, cnt);

                s += cnt;
                b_sec += 1;
            }

            if( (b_sec < e_sec) && is_end_part && end_off )
            {
                // сначала загоним "хвост"
                e_sec -= 1;
                TryDVDReadBlocks(content.file, e_sec, 1, buf);

                memcpy(s_end - end_off, buf, end_off);
            }

            ReadDVD(content.file, s, b_sec, e_sec, shift_fnr);
        }
    }
}

} // namespace DVD

