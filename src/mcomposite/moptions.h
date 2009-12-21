//
// moptions.h
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

#ifndef __MOPTIONS_H__
#define __MOPTIONS_H__

#include <mbase/composite/component.h>

#include "mvisitor.h"
#include "mmedia.h"

namespace CmdOptions {

struct TempObjs
{
              Rect  plc;      // где кладем очередной объект
        const char* frameName; // тема рамки для всех последующих медиа
  Comp::MovieMedia* mm;       // какой клип берем за основу
              bool  nxtBas;   // первый следующий клип считать базовым
        const char* fontDsc;  // параметры шрифта

            TempObjs(): frameName(NULL), mm(NULL), nxtBas(false), fontDsc(NULL) { }
            // взять данное медиа за основу (по кол-ву кадров)
      void  SetBaseMovie(Comp::MovieMedia* new_mm)
            {
                if( !mm || nxtBas )
                {
                    mm = new_mm;
                    nxtBas = false;
                }
            }
            //
      void  SetNextBase() { nxtBas = true; }
};

// открыть файл: только для чтения (is_read) или только для записи
int OpenFileAsArg(const char* fpath, bool is_read);
//
Comp::Media* CreateMedia(const char* fpath, bool& is_movie);

Comp::StillPictMedia* CreateBlackImage(const Point& sz);

} // CmdOptions


// bool ParseOptions(int argc, char *argv[], DoBeginVis& beg);
//
// int CreateCompositionFromCmd(int argc, char *argv[], DoBeginVis& beg, bool is_abort);

class BaseCmdParser
{
    public:
                    BaseCmdParser(int argc, char *argv[]): argCnt(argc), argVars(argv) {}
      virtual      ~BaseCmdParser() {}

                    // разобрать командную строку и заполнить по ней композицию
                    // beg.lstObj;
                    // возвращает 0, если все нормально, иначе - код выхода
                    // (если is_abort, то завершаем программу по abort())
               int  CreateComposition(DoBeginVis& beg, bool is_abort);

              bool  ParseOptions(DoBeginVis& beg);

    protected:

                       int  argCnt;
                     char** argVars;
      CmdOptions::TempObjs  tOpts;

                         // :TODO: перенести в mcomposite TextObj вместе с его рендерингом
   virtual Comp::Object* CreateTextObj(const char* /*text*/) { return 0; }
};

#endif // #ifndef __MOPTIONS_H__

