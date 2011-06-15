//
// mdemux/videoline.h
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

#ifndef __MDEMUX_VIDEOLINE_H__
#define __MDEMUX_VIDEOLINE_H__

#include <deque>  // FrameList
#include <vector> // FrameData::ChunkList
#include <list>   // FrameList::IterList

#include "mpeg.h"
#include "mpeg_video.h"
#include "decoder.h"


namespace Mpeg {

struct ChunkOpt
{
    io::pos  intPos; // внутренняя позиция блока 
     double  pts;

        ChunkOpt(io::pos pos, double p): intPos(pos), pts(p) {}
};

// флаги FrameData
const uint8_t fdHEADER      = 0x0001; // кадр начинается с послед.
const uint8_t fdGOP         = 0x0002; // присутствует GOP
const uint8_t fdGOP_CLOSED  = 0x0004; // присутствует закрытый GOP
const uint8_t fdDEF_PTS     = 0x0008; // кадр имеет явный PTS (полученный от демиксера)

struct FrameData
{
    typedef std::vector<Chunk> ChunkList;

     PicType  typ;
        char  len; // длина в пол-тактах
     uint8_t  opt; // fdXXX
      double  pts;
   ChunkList  dat;

             FrameData()
             {
                 Init();
             }

       bool  IsInit() const { return typ != ptNONE; }
       void  Init()   
             { 
                 typ = ptNONE;
                 len = 2;
                 opt = 0;
                 dat.clear();
                 pts = INV_TS;
             }
       bool  IsFull() { return !dat.empty(); }
};

// продолжительность кадра
// len - длина в пол-тактах
inline double FrameDuration(double tick_len, int len) 
{ 
    return tick_len * len / 2.0; 
}

struct ParseContext
{
      Demuxer  dmx;

      Decoder  dcr;
  MpegDecodec  m2d;

            ParseContext(io::stream& strm): dmx(&strm) {}

      void  Init();
};

// 
// FrameList
//
class FrameList;

struct FLIterator
{
    typedef std::random_access_iterator_tag  iterator_category;

    typedef FrameData* Pointer;
    typedef FrameData& Ref;
    typedef ptrdiff_t  DiffType;
    typedef FLIterator Itr;

    typedef std::list<Itr*> IterList;


                FLIterator();
                FLIterator(const FLIterator& it);
               ~FLIterator();

          bool  IsNone() const;
          void  SetNone();

           Itr& operator =(const Itr& it);

           Ref  operator*() const;
       Pointer  operator->() const;

           Itr& operator++()    { pos++; return *this; }
           Itr  operator++(int) { Itr res(*this); ++*this; return res; }
           Itr& operator--()    { pos--; return *this; }
           Itr  operator--(int) { Itr res(*this); --*this; return res; }
           Itr& operator+=(DiffType n)       { pos += n; return *this; }
           Itr  operator +(DiffType n) const { Itr res(*this); res += n; return res; }
           Itr& operator-=(DiffType n)       { pos -= n; return *this; }
           Itr  operator -(DiffType n) const { Itr res(*this); res -= n; return res; }
           Ref  operator[](DiffType n) const;

           int  Pos() const { return pos; }

     static const FLIterator  None; // экземпляр нулевого итератора
    protected:

                   FrameList* owner;
                         int  pos;
          IterList::iterator  itrPos; // позиция данного итератора в списке его владельца

                FLIterator(int p, FrameList* own);

void  SetOwner(FrameList* own);
          friend class FrameList;
};

inline bool operator ==(const FLIterator& x, const FLIterator& y)
{ return x.Pos() == y.Pos(); }
inline bool operator !=(const FLIterator& x, const FLIterator& y)
{ return !(x == y); }
inline bool operator < (const FLIterator& x, const FLIterator& y)
{ return x.Pos() < y.Pos(); }
inline bool operator > (const FLIterator& x, const FLIterator& y)
{ return x.Pos() > y.Pos(); }
inline bool operator <=(const FLIterator& x, const FLIterator& y)
{ return !(x > y); }
inline bool operator >=(const FLIterator& x, const FLIterator& y)
{ return !(x < y); }

inline FLIterator::DiffType operator -(const FLIterator& x, const FLIterator& y)
{ return x.Pos() - y.Pos(); }
inline FLIterator           operator +(ptrdiff_t n, const FLIterator& x)
{ FLIterator res(x); res += n; return res; }

// 
// FLState
// 
// RawFL(нач. состояние) -> IRoundedFL(начинаемся с I-кадра) -> 
// PlayableFL (можно проигрывать) ->(FrameList::Init()) RawFL
// 
class FLState
{
    public:
        virtual      ~FLState() {}

        virtual void  Clear(FrameList&) {}
        virtual void  UpdateState(FrameList& flist) = 0;
        virtual bool  IsPlayable() { return false; }

        virtual void  ShiftIterators(int /*shift*/, FrameList& /*flist*/) {}
        virtual void  MoveIterator(int /*old_pos*/, int /*new_pos*/, FrameList&) {}

    protected:
                void  ChangeUpdateState(FLState& stt, FrameList& flist);
};

class RawFL: public FLState, public Singleton<RawFL>
{
    public:
        virtual void  UpdateState(FrameList& flist);
};

class IRoundedFL: public FLState, public Singleton<IRoundedFL>
{
    public:
        virtual void  UpdateState(FrameList& flist);
};

class PlayableFL: public FLState, public Singleton<PlayableFL>
{
    public:
                struct Data
                {
                        bool  firstCutTry;
                              Data(): firstCutTry(true) {}
                };

        virtual void  Clear(FrameList&);

        virtual void  UpdateState(FrameList& flist);
        virtual bool  IsPlayable() { return true; }

        virtual void  ShiftIterators(int shift, FrameList& flist);
        virtual void  MoveIterator(int old_pos, int new_pos, FrameList&);

    protected:
                void  CutOverLimit(FrameList& flist);
};

// :TODO: выделить в отдельный класс FLList (контейнер в пару итератору
//  FLIterator), и вынести туда следующие методы:
//  - Bind()/UnBind()
//  - ShiftIterators(), MoveIterator()
//  - Clear() из PlayableFL::Clear()

// список кадров
typedef std::deque<FrameData> FrameListCont;
class FrameList: protected FrameListCont,
                 public PlayableFL::Data
{
    public:

    typedef   FrameListCont::reference  Ref;
    // кадры в физическом порядке
    typedef    FrameListCont::iterator  PhisItr;
    typedef            std::deque<int>  IndexList;
    // iterList
    typedef                 FLIterator  Itr;
    typedef       FLIterator::IterList  IterList;

                             FrameList(): isEOF(false), flStt(0) { Init(); }
                            ~FrameList() { Init(); } // чтоб iterLst обнулить
                            
              FrameListCont& GetContainer() { return (FrameListCont&)*this; }

                       void  Init();
                       bool  IsInit() { return timeShift != 0.; }
                       void  Setup( SequenceData& seq );

                             // после добавления кадров следует обновить индекс и время новых кадров
                             // time_shift - время между двумя кадрами (из частоты кадров)
                             // is_eof - установить признак конечного кадра (end of frames)
                       void  UpdateTimeIndex(bool is_eof);

                             // Перебор кадров - логические позиции
                             // можем проиграть промежуток кадров [Beg(), End())
                             // Если true, то:
                             //  - можно вызывать Beg(), End();
                             //  -  Beg() < End()
                       bool  IsPlayable();
                             // возвращает логические позиции первого и за последним кадра
                 const  Itr& Beg();
                 const  Itr& End();
                             // всегда End() <= DataEnd(); использовать кадры (обычно это один 
                             // не B-кадр, все левые B-кадры которого еще не получены) сверх
                             // End() можно, но проигрывать нельзя
                        Itr  DataEnd();
                             // проигрываемый промежуток [TimeBeg(), TimeEnd())
                     double  TimeBeg();
                     double  TimeEnd();
                             // получить кадр по текущей позиции
                        Ref  At(int i) { return PhisAt(idxLst.at(i)); }
                             // 
                        Itr  PosByTime(double time);

                             // Перебор кадров - физические позиции
                    PhisItr  PhisBeg() { return begin(); }
                    PhisItr  PhisEnd();
                        Ref  PhisAt(int i) { return FrameListCont::at(i); }
                        int  PhisSize()  { return PhisEnd()-PhisBeg(); }

    protected:

               IndexList  idxLst; // кадры в логическом (по времени) порядке
                    bool  isEOF;
                  double  timeShift; // продолжительность одного кадра, вычисляемая по частоте кадров

                 FLState* flStt;

                 IterList  iterLst;
                      Itr  playBeg;
                      Itr  playEnd;

// возвращает позицию, с которой обновили
int  UpdateIndex();
//bool IsIRounded();
// найти начиная с логич. позиции I
int  FindIFrame(int log_pos, bool is_fwd);
void  CutByIFrame(int log_pos);

void  Bind(Itr* it);
void  UnBind(Itr* it);

    friend struct FLIterator;

    friend class FLState;
    friend class RawFL;
    friend class IRoundedFL;
    friend class PlayableFL;
};


class VideoLine: public FirstVideoSvc, public VideoService
{
    typedef std::deque<Chunk> ChunkList;
    typedef std::deque<ChunkOpt> ChunkOptList;

    public:

                      VideoLine(ParseContext& cont);
                     ~VideoLine();

                      // заново начать создание FrameList на промежутке (beg, beg+len)
                void  MakeForPeriod(io::pos beg, io::pos len);
                      // обработать следующие len байт
                void  MovePeriod(io::pos len);
                      // --||--, не обрезая при достижении максимума кадров
                void  ContinuePeriod(io::pos len);
                      // обнулить FrameList
                void  Clear();

        ParseContext& GetParseContext() { return prsCont; }
                      // System
        virtual void  GetData(Mpeg::Demuxer& dmx, int len);
                      // Video
        virtual void  TagData(Decoder& dcr, VideoTag tag);
                      //
           FrameList& GetFrames() { return framLst; }

    protected:

          ParseContext& prsCont;

                double  prevPts;
             ChunkList  chkLst;
          ChunkOptList  chkOptLst;

             FrameList  framLst;
                  bool  errStt;  // декодеру не нравятся данные

void  AddChunk(io::pos ext_pos, int len, double pts);
// удалить с начала блоки
void  RemoveChunks(int cnt);
// вернуть текущий кадр, если нужно - создать
FrameData&  MakeGetCurFrame(VideoTag tag);
void  FinishFrame();
void  FillFrame(FrameData& fd, io::pos int_pos);
// если были в ошибочном состоянии, то
// кадр пропадает,- начинаем с нового
void  ClearErrorState(FrameData& fd, io::pos int_pos);
void  FillFrameData(FrameData::ChunkList* chk_lst, io::pos int_pos);
//
void  ProcessPeriod(io::pos to_pos);
};

void CleanEof(Demuxer& dmx);
bool IsGood(Demuxer& dmx);

} // namespace Mpeg

#endif // __MDEMUX_VIDEOLINE_H__

