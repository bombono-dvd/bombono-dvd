//
// mgui/execution.cpp
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

#include <mgui/_pc_.h>

#include "execution.h"
#include "dialog.h"
#include "gettext.h"

#include <mlib/tech.h>
#include <mlib/sigc.h>

//#include <mgui/sdk/libgnome/gnome-exec.h>
#include <mgui/sdk/libgnome/gnome-util.h>

#include <boost/lexical_cast.hpp>
#include <signal.h> // SIGTERM
#include <errno.h>
#include <sys/wait.h> // WIFEXITED

namespace Execution
{

void Stop(GPid& pid)
{
    ASSERT( pid != NO_HNDL );
    kill(pid, SIGTERM);
}

void Data::StopExecution(const std::string& what)
{
    // COPY_N_PASTE - тупо сделал содержимое сообщений как у "TSNAMI-MPEG DVD Author"
    // А что делать - нафига свои придумывать, если смысл один и тот же
    if( Gtk::RESPONSE_YES == MessageBox(BF_("You are about to cancel %1%. Are you sure?") % what 
                                            % bf::stop, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO) )
    {
        userAbort = true;
        if( pid != NO_HNDL ) // во время выполнения внешней команды
            Stop(pid);
    }
}

static bool PulseProgress(Gtk::ProgressBar& prg_bar)
{
    prg_bar.pulse();
    return true;
}

Pulse::Pulse(Gtk::ProgressBar& prg_bar) 
{ 
    tm.Connect(bl::bind(&PulseProgress, boost::ref(prg_bar)), 500); 
}

Pulse::~Pulse() 
{
    tm.Disconnect(); 
}

void SimpleSpawn(const char *commandline, const char* dir)
{
    GPid p = Spawn(dir, commandline);
    ASSERT_RTL( p > 0 );
    g_spawn_close_pid(p);
}

} // namespace Execution

struct ExecOutput
{
          sigc::connection  outConn;
    RefPtr<Glib::IOChannel> outChnl;

        ~ExecOutput() { outConn.disconnect(); }
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

static ExecOutput& GetEO(ProgramOutput& po, bool is_out)
{
    return is_out ? po.outEO : po.errEO ;
}

static bool ReadPendingData(ProgramOutput& po, bool is_out)
{
    LOG_INF << "ReadPendingData(" << is_out << ")" << io::endl;

    bool res = true;
    RefPtr<Glib::IOChannel> chnnl = GetEO(po, is_out).outChnl;
    char buf[256];
    gsize nbytes;

    Glib::ustring dat;
    ReadDest& rd = is_out ? po.outRd : po.errRd;
    for( Glib::IOStatus st; st = chnnl->read(buf, ARR_SIZE(buf), nbytes), true; )
    {
        LOG_INF << "IOStatus = " << st << io::endl;
        if( st == Glib::IO_STATUS_NORMAL )
        {
            ASSERT_RTL( nbytes>0 ); // иначе должно быть IO_STATUS_EOF
            rd.PutData(buf, nbytes);
        }
        else
        {
            if( st != Glib::IO_STATUS_AGAIN )
                res = false;
            break;
        }
    }
    if( !res )
        rd.OnEnd();

    LOG_INF << "ReadPendingData(): end" << io::endl;
    return res;
}

Glib::IOCondition StreamConditionMask = Glib::IO_IN | Glib::IO_HUP;

static bool OnStreamReadReady(ProgramOutput& po, bool is_out, Glib::IOCondition cond)
{
    ASSERT_OR_UNUSED_VAR( cond & StreamConditionMask, cond );
    return ReadPendingData(po, is_out);
}

static void SetupEO(ProgramOutput& po, int fd, bool is_out)
{
    ExecOutput& eo = GetEO(po, is_out);
    using namespace boost;
    function<bool(Glib::IOCondition)> fnr = lambda::bind(&OnStreamReadReady, boost::ref(po), is_out, lambda::_1);
    eo.outConn = Glib::signal_io().connect(wrap_return<bool>(fnr), fd, StreamConditionMask);

    eo.outChnl = Glib::IOChannel::create_from_fd(fd);
    eo.outChnl->set_close_on_unref(true);
    eo.outChnl->set_flags(Glib::IO_FLAG_NONBLOCK);
}

static void WaitExitCode(ExitData& ed, GPid pid, int status)
{
    ed.normExit = true;
    if( WIFEXITED(status) )
        ed.retCode = WEXITSTATUS(status);
    else if( WIFSIGNALED(status) )
    {
        ed.normExit = false;
        ed.retCode  = WTERMSIG(status);
    }
    else
        ASSERT_RTL(0);
    g_spawn_close_pid(pid);

    Gtk::Main::quit();
}

static void TestFdState(int fd, bool be_open)
{
    // ожидаемое значение
    int need_errno = be_open ? 0 : EBADF; // нет такого открытого fd

    errno = 0; // чистим
    io::tell(fd);
    ASSERT_OR_UNUSED_VAR( errno == need_errno, need_errno );
}

static void LogExecuteAsync(const std::string& str)
{
    LOG_INF << io::endl;
    LOG_INF << "ExecuteAsync(): " << str << io::endl;
    LOG_INF << io::endl;
}

static ExitData ExecuteAsyncImpl(const char* dir, const char* cmd, ReadDest& out_rd, ReadDest& err_rd,
                                 GPid* pid)
{
    LogExecuteAsync("Begin");
    ExitData ed;
    int out_err[2];
    {
        GPid p = Spawn(dir, cmd, out_err, true);
        ASSERT_RTL( p > 0 );
        if( pid )
            *pid = p;

        using namespace boost;
        Glib::signal_child_watch().connect(lambda::bind(&WaitExitCode, boost::ref(ed), lambda::_1, lambda::_2), p);
        ProgramOutput po(out_rd, err_rd);
        SetupEO(po, out_err[0], true);
        SetupEO(po, out_err[1], false);
    
        Gtk::Main::run();

        ReadPendingData(po, true);
        ReadPendingData(po, false);
    }
    // должны быть закрыты ExecOutput::outStrm
    TestFdState(out_err[0], false);
    TestFdState(out_err[1], false);

    LogExecuteAsync("End");
    return ed;
}

class RawRD: public ReadDest
{
    public:
                    RawRD(bool is_out, const ReadReadyFnr& f)
                        : isOut(is_out), fnr(f) {}

      virtual void  PutData(const char* dat, int sz) { Put(dat, sz); }
     virtual  void  OnEnd() {}

    protected:
        ReadReadyFnr  fnr;
                bool  isOut;

              void  Put(const char* dat, int sz) { fnr(dat, sz, isOut); }
};

class LinedRD: public RawRD
{
    typedef RawRD MyParent;
    public:
                    LinedRD(bool is_out, const ReadReadyFnr& f): MyParent(is_out, f) {}
                    
      virtual void  PutData(const char* dat, int sz);
      virtual void  OnEnd()
      {
          if( int sz = (int)line.size() )
          {    
              Put(line.c_str(), sz);
              line.clear();
          }
      }

    protected:
        std::string line;
};

static bool IsCR(char c) { return c == '\r'; }
static bool IsLF(char c) { return c == '\n'; }

// строкой является одно из '\r', '\n', '\r\n'
void LinedRD::PutData(const char* dat, int dat_sz)
{
    int off    = (int)line.size();
    bool is_cr = (off != 0) && IsCR(line[off-1]);

    line.append(dat, dat_sz);
    int beg = 0;
    for( int sz = (int)line.size(); off < sz; off++ )
    {
        char c = line[off];
        bool is_new_cr = IsCR(c);
        if( IsLF(c) ) // '\n'
        {
            int new_beg = off+1;
            Put(line.c_str()+beg, new_beg-beg);

            beg  = new_beg;
        }
        else if( is_cr )
        {
            Put(line.c_str()+beg, off-beg);
            beg = off;
        }

        is_cr = is_new_cr;
    }
    if( beg )
        // обрезаем прочитанные строки безопасным образом
        line = std::string(line.begin()+beg, line.end());
}

static ReadDest* CreateReadDest(const ReadReadyFnr& fnr, bool is_out, bool line_up)
{
    return line_up ? new LinedRD(is_out, fnr) : new RawRD(is_out, fnr);
}

ExitData ExecuteAsync(const char* dir, const char* cmd, ReadReadyFnr& fnr,
                      GPid* pid, bool line_up)
{
    ptr::one<ReadDest> o_rd(CreateReadDest(fnr, true,  line_up));
    ptr::one<ReadDest> e_rd(CreateReadDest(fnr, false, line_up));
    return ExecuteAsyncImpl(dir, cmd, *o_rd, *e_rd, pid);
}

static void ThrowGError(const std::string& dsc, GError* err)
{
    std::string str(err->message);
    str += " (" + dsc + ")";
    throw std::runtime_error(str);
}

GPid Spawn(const char* dir, const char *commandline, int out_err[2], bool need_watch,
           int* in_fd)
{
    LOG_INF << "Spawn(" << commandline << ")" << io::endl;
    //// старый вариант
    //return gnome_execute_shell_redirect(dir, commandline, out_err);

    GError* error = 0;
    //gchar **argv = NULL;
    //if( !g_shell_parse_argv(commandline, NULL, &argv, &error) )
    //  ThrowGError(std::string("while parsing command_line: ") + commandline, error);

    g_return_val_if_fail(commandline != NULL, -1);

    char* user_shell = gnome_util_user_shell();
    char * argv[4];
    argv[0] = user_shell;
    argv[1] = (char *)"-c";
    /* necessary cast, to avoid warning, but safe */
    argv[2] = (char *)commandline;
    argv[3] = NULL;

    GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
    // чтобы exec() проходил после первого fork() и можно было использовать waitpid(),
    // см. описание waitpid()
    if( need_watch ) 
      flags = GSpawnFlags(flags | G_SPAWN_DO_NOT_REAP_CHILD);

    GPid pid;
    bool res = g_spawn_async_with_pipes(dir, argv, 0, flags, 0, 0, &pid, 
                                        in_fd, out_err, out_err ? out_err+1 : 0, &error);
    //g_strfreev (argv);
    g_free (user_shell);

    if( !res )
        ThrowGError(std::string("while spawning: ") + argv[0], error);
    return pid;
}

std::string SignalToString(int sig)
{
    const char* sigtable[]= {
        "SIGHUP",
        "SIGINT",
        "SIGQUIT",
        "SIGILL",
        "SIGTRAP",
        "SIGABRT/SIGIOT",
        "SIGBUS",
        "SIGFPE",
        "SIGKILL",
        "SIGUSR1",
        "SIGSEGV",
        "SIGUSR2",
        "SIGPIPE",
        "SIGALRM",
        "SIGTERM",
        "SIGSTKFLT",
        "SIGCHLD",
        "SIGCONT",
        "SIGSTOP",
        "SIGTSTP",
        "SIGTTIN",
        "SIGTTOU",
        "SIGURG",
        "SIGXCPU",
        "SIGXFSZ",
        "SIGVTALRM",
        "SIGPROF",
        "SIGWINCH",
        "SIGIO"
    };
    if( (sig > 0) && (sig <= (int)ARR_SIZE(sigtable)) )
        return sigtable[sig-1];
    else
        return boost::lexical_cast<std::string>(sig);
}

std::string ExitDescription(const ExitData& ed)
{
    std::string end_str;
    if( ed.IsGood() )
        end_str = "normal completion";
    else if( ed.normExit )
        end_str = BF_("exit code = %1%") % ed.retCode % bf::stop;
    else
        end_str = BF_("broken by signal %1%") % SignalToString(ed.retCode) % bf::stop;

    return end_str;
}

