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

