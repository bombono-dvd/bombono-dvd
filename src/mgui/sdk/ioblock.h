//
// mgui/sdk/ioblock.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#ifndef __MGUI_SDK_IOBLOCK_H__
#define __MGUI_SDK_IOBLOCK_H__

#include <mgui/execution.h> // ReadReadyFnr

#include <mlib/function.h>

struct ExecOutput
{
    typedef boost::function<bool(Glib::IOCondition)> Functor;

          sigc::connection  outConn;
    RefPtr<Glib::IOChannel> outChnl;

        ~ExecOutput();

   void  Watch(int fd, const Functor& fnr, Glib::IOCondition cond);
};

class ReadDest
{
    public:
    virtual       ~ReadDest() {}

    virtual  void  PutData(const char* dat, int sz) = 0;
    virtual  void  OnEnd() = 0;
};

struct ProgramOutput
{
    ExecOutput  outEO;
    ExecOutput  errEO;

      ReadDest& outRd; 
      ReadDest& errRd;

      ProgramOutput(ReadDest& o_rd, ReadDest& e_rd): outRd(o_rd), errRd(e_rd) {}
};

// в пределах жизни этого блока в fnr будут поступать данные с выходов out_err,
// а после последние будут закрыты
class OutErrBlock
{
    public:

        OutErrBlock(int out_err[2], const ReadReadyFnr& fnr, bool line_up = true);
       ~OutErrBlock();

    protected:
        ptr::one<ReadDest>  outRd;
        ptr::one<ReadDest>  errRd;
             ProgramOutput  po;
};

#endif // #ifndef __MGUI_SDK_IOBLOCK_H__

