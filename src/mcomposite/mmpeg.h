//
// mmpeg.h
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

#ifndef __MMPEG_H__
#define __MMPEG_H__

#include <mlib/patterns.h>  // class Singleton<>
#include <mlib/stream.h>
#include <mdemux/decoder.h>

#include "mdemux.h"

//
// MpegPlayer
//

class MpegPlayer;

//// Cостояния проигрывателя
//
//       <--------         <--------
// Demux --------> Decodec --------> Pict(ure)
//   |               |^    
//   |               ||
//   |               ||
//   V               V|
//  End             Seq(ence)

// переключение автомата (в новые состояния) проиходит по 
// принципу "уже" (готово), т.е. картинка уже загружена
// в состоянии "картинка", работа уже закончена в состоянии
// "eof", буфер для декодера (уже) пуст
// (в случае перехода в состояние "демиксирования")
class PlayerState
{
    public:

     virtual      ~PlayerState() {}

                   /* Управление состояниями */
                   // переход в следующее состояние
     virtual void  NextState(MpegPlayer& player) = 0;

                   /* Проверка состояний */
                   // заключительное состояние
     virtual bool  IsEnd() { return false; }
                   // готова ли очередная картинка
     virtual bool  IsPict() { return false; }
                   // загружен видео-заголовок
     virtual bool  IsSeq() { return false; }

     protected:

         void ChangeState(MpegPlayer& plyr, PlayerState& stt);
};

// конечное состояние - кончились данные или ошибка какая
class PlayerEnd: public PlayerState, public Singleton<PlayerEnd>
{
    public:

    virtual  bool  IsEnd() { return true; }
    virtual  void  NextState(MpegPlayer& player);
};

// нужно заполнить буфер демиксированием
class PlayerDemux: public PlayerState, public Singleton<PlayerDemux>
{
    public:

                   class DemuxData
                   {
                       public:
                                 DemuxData(): isEnd(false) { }

                       private:
                                       // если да, то больше из потока не читаем
                                 bool  isEnd;
            
                       friend class PlayerDemux;
                   };

    virtual  void  NextState(MpegPlayer& player);
};

// готов буфер для декодирования
class PlayerDecode: public PlayerState, public Singleton<PlayerDecode>
{
    public:

    virtual  void  NextState(MpegPlayer& player);
};

// загружен очередной кадр
class PlayerPict: public PlayerState, public Singleton<PlayerPict>
{
    public:

    virtual  void  NextState(MpegPlayer& player);
    virtual  bool  IsPict() { return true; }
};

// загружен заголовок видео (т.е. его характеристики)
class PlayerSeq: public PlayerState, public Singleton<PlayerSeq>
{
    public:
                   class SeqData
                   {
                       public:
                                SeqData(): isSet(false) { }

                       private:
                                      // позволяем только раз проходить
                                      // это состояние пока
                                bool  isSet;

                       friend class PlayerSeq;
                   };

    virtual  void  NextState(MpegPlayer& player);
    virtual  bool  IsSeq() { return true; }

};

// сам проигрыватель
struct MpegPlayer: public PlayerSeq::SeqData,
                   public PlayerDemux::DemuxData
{
        TrackBuf  trkBuf;
     MpegDecodec  decodec;
      SeqDemuxer* demuxer;
      io::stream* inpStrm;

        
             MpegPlayer(io::stream* strm, SeqDemuxer* dmx);
            ~MpegPlayer();

             // для проверок типа while( player ) ... ,
             // т.е. по аналогии с iostream
             operator void*() const 
             { 
                 return state->IsEnd() ? 0 : const_cast<MpegPlayer*>(this) ; 
             }
       bool  operator !() const { return state->IsEnd(); }
             // к следующему состоянию
       void  NextState() { state->NextState(*this); }

       bool  NextImage();
       bool  NextSeq();

    private:

     PlayerState* state; // текущее состояние проигрывателя


        void ChangeState(PlayerState* stt);
        friend class PlayerState;
};

// Функция для установки MovieInfo из Mpeg2dec
bool GetMovieInfo(MovieInfo& mi, MpegDecodec& dec);

#endif // #ifndef __MMPEG_H__

