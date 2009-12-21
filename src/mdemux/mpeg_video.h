//
// mdemux/mpeg_video.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#ifndef __MDEMUX_MPEG_VIDEO_H__
#define __MDEMUX_MPEG_VIDEO_H__

#include <arpa/inet.h> // htonX()
#include <mlib/geom2d.h>
#include <mlib/patterns.h>

#include "trackbuf.h"

namespace Mpeg
{

const uint32_t vidPIC       = 0x100; // picture start code
const uint32_t vidSLICE_BEG = 0x101; // slice start code range
const uint32_t vidSLICE_END = 0x1B0; //
const uint32_t vidUSR_DAT   = 0x1B2; // user data start code
const uint32_t vidSEQ_HDR   = 0x1B3; // sequence header start code
const uint32_t vidSEQ_ERR   = 0x1B4; // sequence error start code
const uint32_t vidEXT       = 0x1B5; // extension start code
const uint32_t vidSEQ_END   = 0x1B7; // sequence end code
const uint32_t vidGOP       = 0x1B8; // GOP start code

const uint32_t vidFF     = 0x0ff;
const uint32_t vidPREFIX = 0x100;

inline uint8_t VSignByte(uint32_t prefix)
{ return uint8_t(vidFF & prefix); }

class Decoder;

enum DecodeType
{
    dtOPAQUE,
    dtEND,      // видео закончилось
    dtBUFFER,   // нужны данные
    dtINVALID   // ошибка
};

class DecodeState
{
    public:
        virtual      ~DecodeState() {}

                      /* Управление состояниями */
                      // переход в следующее состояние
        virtual void  NextState(Decoder& dcr) = 0;
//                       // заключительное состояние
//         virtual bool  IsEnd() { return false; }
                      // нужны данные
        virtual bool  IsBuffer() { return false; }
                      //
  virtual DecodeType  Type() { return dtOPAQUE; }
                      //
  virtual   uint32_t  Code() { return vidSEQ_ERR; }

    protected:
                void  ChangeState(DecodeState& stt, Decoder& dcr);


bool  HaveBytes(int cnt, Decoder& dcr);
// перейти в состояние stt после нахождения его начала
void  ChangeWithNextState(DecodeState& stt, Decoder& dcr);
// пропустить текущ. секцию
void  SkipState(DecodeState& after_stt, Decoder& dcr, int skip_bytes = 0);
// 
void  InvalidState(Decoder& dcr, int skip_bytes = 4);

};

// End
class EndDecode: public DecodeState, public Singleton<EndDecode>
{
    public:
        virtual void  NextState(Decoder&);
  virtual DecodeType  Type() { return dtEND; }
};

// Buffer
class BufferDecode: public DecodeState, public Singleton<BufferDecode>
{
    public:
                struct Data
                {
                    DecodeState* nxtStt;  // для какого состояния
                            int  bytNeed; // сколько нужно данных

                            Data(): nxtStt(0), bytNeed(0) {}
                };


        virtual void  NextState(Decoder& dcr);
        virtual bool  IsBuffer() { return true; }
  virtual DecodeType  Type() { return dtBUFFER; }

                void  NeedBytes(int cnt, DecodeState* for_stt, Decoder& dcr);
};

class CommonSkipDecode: public DecodeState
{
    public:
        struct Data
        {
            DecodeState* nxtStt;
                    Data(): nxtStt(0) {}
        };

                void  SetState(DecodeState& stt, Decoder& dcr);

    protected:
                void  Complete(Decoder& dcr);
};

// NextDecode (поиск следующей структуры, next start code)
class NextDecode: public CommonSkipDecode, public Singleton<NextDecode>
{
    public:
        virtual void  NextState(Decoder& dcr);
};

// пропустить все данные до след. структуры
class SkipDecode: public CommonSkipDecode, public Singleton<SkipDecode>
{
    public:
        virtual void  NextState(Decoder& dcr);
};

// в случае ошибок ищем, с чего бы продолжить работу
class SeekDecode: public DecodeState, public Singleton<SeekDecode>
{
    public:
        virtual void  NextState(Decoder& dcr);
  virtual DecodeType  Type() { return dtINVALID; }
};

enum ChromaType
{
    ctRESERVED = 0,
    ct420      = 1,
    ct422      = 2,
    ct444      = 3
};

struct SequenceData
{
         // размеры, то же самое, что и sequence->picture_width, sequence->picture_height в libmpeg2
         // если нет расширения extSEQ_DISP, то picture_* = display_*
    int  wdh; 
    int  hgt;

    int  rawWdh; // те же размеры, но кратны 16
    int  rawHgt; 

    int  sarCode;  // соотношение сторон пиксела (sample aspect ratio)
  Point  framRat;  // частота кадров
    int  bytRat;   // пропускная способность 
    int  vbvBufSz; // мин. требуемый размер буфера для декодировки (в байтах)

         // MPEG2
    int  plId;        // profile and level indication
   bool  isProgr;     // прогрессивная (или черезстрочная)
    int  chromaFrmt;  // 4:2:0, 4:2:2, 4:4:4
   bool  lowDelay;    // "большие картинки"

              SequenceData() { Init(); }
    
        bool  IsInit() const {  return wdh != -1; }
        void  Init() { wdh = -1; }
        bool  operator==(const SequenceData& sd);

       Point  PixelAspect() const;
       Point  SizeAspect() const;
};

// sequence header
class HeaderDecode: public DecodeState, public Singleton<HeaderDecode>
{
    public:
                struct Data: public SequenceData
                {
                    int  framRatCode;
                    int  framRatN;
                    int  framRatD;
                };

        virtual void  NextState(Decoder& dcr);
    virtual uint32_t  Code() { return vidSEQ_HDR; }
};

class CommonExtDecode: public DecodeState
{
    public:
    virtual uint32_t  Code() { return vidEXT; }
};

// sequence extension
class ExtHeaderDecode: public CommonExtDecode, public Singleton<ExtHeaderDecode>
{
    public:
        virtual void  NextState(Decoder& dcr);
    virtual uint32_t  Code() { return vidSEQ_ERR; }

                bool  SetSeqData(Decoder& dcr);
};

// picture coding extension
class PicCodingExtDecode: public CommonExtDecode, public Singleton<PicCodingExtDecode>
{
    public:
        virtual void  NextState(Decoder& dcr);
                bool  CompletePicture(Decoder& dcr, int pic_struct, 
                                      bool tff, bool rff, bool progr_frame);
};

enum
{
    extSEQ        = 1,  // seq ext id
    extSEQ_DISP   = 2,  // seq display ext id
    extQUANT_MATR = 3,  // quant matrix ext id
    extCOPYRIGHT  = 4,  // copyright ext id
    extSCAL_EXT   = 5,  // seq scalable ext id
    extPIC_DISP   = 7,  // picture display ext id
    extPIC_CODING = 8,  // picture coding  ext id
    extPIC_SPT    = 9,  // picture spatial scalable ext id
    extPIC_TMP    = 10  // picture temporal scalable ext id
};

class ExtUserDecode: public DecodeState, public Singleton<ExtUserDecode>
{
    public:
        struct Data
        {
          unsigned char  type;     // 0, 1, 2 
                    int  allowExt; // список допустимых расширений
                    Data(): type(3) {}
        };

        virtual void  NextState(Decoder& dcr);
        virtual void  SetEUType(unsigned char type, Decoder& dcr);
};

class GOPDecode: public DecodeState, public Singleton<GOPDecode>
{
    public:
        virtual void  NextState(Decoder& dcr);
};

// типы кадров, Doc: 6.3.9 Picture Header
enum PicType
{
    ptFORBIDDEN = 0,
    ptNONE      = ptFORBIDDEN,

    ptI_FRAME   = 1,
    ptP_FRAME   = 2,
    ptB_FRAME   = 3,
    ptD_FRAME   = 4
};

// picture_structure, Doc: 6.3.10
enum PicStructType
{
    pstRESERVED     = 0,
    pstTOP_FIELD    = 1,
    pstBOTTOM_FIELD = 2,
    pstFRAM_PICTURE = 3
};

struct Picture
{
            PicType  type;
                int  tmpRef; // temporal_reference
      PicStructType  structTyp;
               bool  tff;    // top_field_first
               bool  rff;    // repeat_first_field
               bool  progrFrame; // progressive_frame

                int  len;      // продолжительность кадра в "пол-тактах", 1 такт = 1/frame_rate
            io::pos  datPos;   // смещение в потоке

               Picture() { Init(); }

         void  Init()   { type = ptFORBIDDEN; }
         bool  IsInit() { return type != ptFORBIDDEN; }
};

class PicHeaderDecode: public DecodeState, public Singleton<PicHeaderDecode>
{
    public:
        struct Data: public Picture
        {
            Picture  firstPic; // вторая нужна для парной картинки
            Picture  secndPic;
        };

    virtual uint32_t  Code() { return vidPIC; }
        virtual void  NextState(Decoder& dcr);
                      //
                void  ClearState(Decoder& dcr, bool tag_error = true);
    protected:

                bool  StartPicture(Decoder& dcr, int tmp_ref, int type);
};

class SliceDecode: public DecodeState, public Singleton<SliceDecode>
{
    public:
        virtual void  NextState(Decoder& dcr);
};

//////////////////////////////////////////////////////////////

//  Метки начала данных в потоке
// для всех (кроме vtHEADER_COMPLETE, vtFRAME_FOUND) установка метки означает, что
// декодер находится в начале соответ. блока
enum VideoTag
{
    vtNONE,    // ничего особенного
    vtHEADER,  // найдена последовательность
    vtHEADER_COMPLETE, // последовательность инициализирована (seqInf)
    vtGOP,     // GOP
    vtFRAME_FOUND,     // найден кадр, начинается с dcr.pic.datPos
    vtEND,     // 
    vtERROR    //
};

class VideoService;

class Decoder: public HeaderDecode::Data,
               public BufferDecode::Data,
               public CommonSkipDecode::Data,
               public ExtUserDecode::Data,
               public PicHeaderDecode::Data
{
    public:

    SequenceData  seqInf;    // базовые данные о потоке (размеры, частота кадров, ...)
            bool  isGOPClosed;
         Picture  pic;

                //
                Decoder();

                // 
                // Cтандартный рабочий цикл:
                // 
                //  Mpeg::Decoder dcr;
                //  dcr.Begin();
                //  while( 1 )
                //      dcr.NextState();
                //
                // (при перемещении по потоку тоже нужен вызов Begin(),- сразу после)
          void  Begin(io::pos dat_pos = -1);
    DecodeType  NextState() 
                { 
                    dcrStt->NextState(*this); 
                    return dcrStt->Type();
                }

                // возвратиться в начальное состояние (= после конструктора),
                // т.е. последовательность не инициализована
          void  Init();
                // даем данные в ответ на dtBUFFER
          void  Feed(const char* beg, const char* end) {  inpBuf.Append(beg, end); }
          void  FeedFromStream(io::stream& strm, int len) { inpBuf.AppendFromStream(strm, len); };

                // работа с данными
       uint8_t* Data(int idx) { return (uint8_t*)inpBuf.Beg()+idx; }
           int  DataSize()    { return inpBuf.Size(); }
                // сбросить данные с начала, размером skip_cnt
          void  DataSkip(int skip_cnt);
                // --||-- с указанием принадлежности
          void  DataSkipTag(VideoTag tag, int skip_cnt);
                //
          void  TagError();
          void  SetFrame(Picture& pic);

                // получить позицию начала блока по типу
       io::pos  DatPosForTag(VideoTag tag);
                // текущая позиция
       io::pos  DatPos() { return datPos; }

                // 
  VideoService* GetService() { return vdSvc; }
          void  SetService(VideoService* svc) { vdSvc = svc; }


    protected:

           TrackBuf  inpBuf;
            io::pos  datPos; // смещение данных в видео-дорожке
       VideoService* vdSvc;

        DecodeState* dcrStt;


          void  ChangeState(DecodeState* stt);
          friend class DecodeState;
};

class VideoService
{
    public:
            virtual      ~VideoService() {}
            virtual void  TagData(Decoder& dcr, VideoTag tag) = 0;
};

} // namespace Mpeg


#endif // __MDEMUX_MPEG_VIDEO_H__

