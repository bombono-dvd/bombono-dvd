//
// mgui/author/execute.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009-2010 Ilya Murav'jov
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

#include "execute.h"
#include "output.h" // Project::FillAuthorLabel()
#include "script.h" // Project::ForeachVideo()

//#include <mgui/sdk/libgnome/gnome-exec.h>
#include <mgui/sdk/libgnome/gnome-util.h>
#include <mgui/win_utils.h>
#include <mgui/gettext.h>
#include <mgui/sdk/widget.h>
#include <mgui/sdk/textview.h>

#include <mlib/sigc.h>
#include <mlib/tech.h>
#include <mlib/sdk/logger.h>

#include <glib/gstdio.h>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <errno.h>
#include <sys/wait.h> // WIFEXITED

namespace Author
{

void ExecState::Init()
{
    Set(false);
    mode      = modFOLDER;
    detailsView.set_editable(false);
    Clean();

    // чтобы сбросить старые обработчики
    RenewPtr(execBtn);
}

void ExecState::Clean()
{
    eDat.Init(); 
    detailsView.get_buffer()->set_text("");

    SetStatus();
    SetIndicator(0);
}

Gtk::Label& ExecState::SetStatus(const std::string& name)
{
    return Project::FillAuthorLabel(prgLabel, _("Status: ") + name);
}

static void InitFoundStageTag(RefPtr<Gtk::TextTag> tag)
{
    tag->property_foreground() = "darkgreen";
}

boost::regex DVDAuthorRE(RG_CMD_BEG"dvdauthor"RG_EW ".*-x"RG_EW RG_SPS RG_BW"DVDAuthor\\.xml"RG_EW);
boost::regex MkIsoFsRE(RG_CMD_BEG"mkisofs"RG_EW ".*-dvd-video"RG_EW ".*>" RG_SPS RG_BW"dvd.iso"RG_EW);
boost::regex GrowIsoFsRE(RG_CMD_BEG"growisofs"RG_EW ".*-dvd-compat"RG_EW ".*-dvd-video"RG_EW); 

//static void PrintMatchResults(const boost::smatch& what)
//{
//    for( int i=1; i<(int)what.size(); i++ )
//        io::cout << "what[" << i << "] = \"" <<  what.str(i) << "\""<< io::endl;
//}

static bool ApplyStage(const std::string& line, const boost::regex& re, 
                       Stage stg, const TextIterRange& tir, OutputFilter& of)
{
    bool res = boost::regex_search(line.begin(), line.end(), re);
    if( res )
    {
        ApplyTag(of.GetTV(), tir, "Stage", InitFoundStageTag);
        of.SetStage(stg);
    }
    return res;
}

class MkIsoFsPP: public ProgressParser
{
    typedef ProgressParser MyParent;
    public:
                  MkIsoFsPP(OutputFilter& of_): MyParent(of_) {}
    virtual void  Filter(const std::string& line);
};

boost::regex MkIsoFsPercent_RE( RG_NUM"([\\.,]"RG_NUM")?% done"); 

void MkIsoFsPP::Filter(const std::string& line)
{
    boost::smatch what;
    if( boost::regex_search(line.begin(), line.end(), what, MkIsoFsPercent_RE) )
    {
        ASSERT( what[1].matched );
        std::string p_str = what.str(1) + "." + (what[3].matched ? what.str(3) : std::string("0"));
        double p = boost::lexical_cast<double>(p_str);
        of.SetProgress(p);
    }
}

const double DVDAuthorRel = 0.9; // 90% времени на VOBU-этап
// расчет индикатора строится на предположениях:
// - 1 titleset, потому что по каждому проходит пара этапов "VOBU"/"fixing"
// - размер dvd_sz равен сумме всех исходников, влкючая меню
class DVDAuthorPP: public ProgressParser
{
    typedef ProgressParser MyParent;
    public:
                  // размер в Mb
                  DVDAuthorPP(OutputFilter& of_, int dvd_sz)
                    : MyParent(of_), dvdSz(dvd_sz), fixStage(false) {}

    virtual void  Filter(const std::string& line);
    protected:
             int  dvdSz;
            bool  fixStage;
};

boost::regex DVDAuthorVOB_RE( "^STAT: VOBU "RG_NUM" at "RG_NUM"MB"); 
boost::regex DVDAuthorFix_RE( "^STAT: fixing VOBU at "RG_NUM"MB \\("RG_NUM"/"RG_NUM", "RG_NUM"%\\)"); 

void DVDAuthorPP::Filter(const std::string& line)
{
    double p = 0.;

    boost::smatch what;
    bool is_fix = boost::regex_search(line.begin(), line.end(), what, DVDAuthorFix_RE);
    if( !(fixStage || is_fix) )
    {
        if( boost::regex_search(line.begin(), line.end(), what, DVDAuthorVOB_RE) )
        {
            int sz = boost::lexical_cast<int>(what.str(2));
            p = sz/(double)dvdSz * DVDAuthorRel;
        }
    }
    else
    {
        fixStage = true;
        if( is_fix )
        {
            p = boost::lexical_cast<int>(what.str(4))/100.;
            p = DVDAuthorRel + p*(1-DVDAuthorRel);
        }
    }

    p *= 100.;
    if( p )
        of.SetProgress(p);
}

void OutputFilter::SetParser(ProgressParser* pp)
{
    curParser = pp;
    OnSetParser();
}

void OutputFilter::OnGetLine(const char* dat, int sz, bool is_out)
{
    Gtk::TextView& txt_view = GetTV();
    std::string line(dat, sz);
    TextIterRange tir = AppendNewText(txt_view, line, is_out);

    using namespace Author;
    if( ApplyStage(line, DVDAuthorRE, stDVDAUTHOR, tir, *this) )
        SetParser(new DVDAuthorPP(*this, GetDVDSize()));
    if( ApplyStage(line, MkIsoFsRE,   stMK_ISO, tir, *this) ||
        ApplyStage(line, GrowIsoFsRE, stBURN,   tir, *this) )
        SetParser(new MkIsoFsPP(*this));

    if( curParser )
        curParser->Filter(line);
}

void BuildDvdOF::SetStage(Stage stg)
{
    ::Author::SetStage(stg);
}

static bool GetSize(Project::VideoItem vi, io::pos& sz)
{
    struct stat buf;
    if( g_stat(Project::GetFilename(*vi).c_str(), &buf) == 0 )
        sz += buf.st_size;
    return true;
}

io::pos VideoSizeSum()
{
    io::pos sz = 0;
    Project::ForeachVideo(boost::lambda::bind(&GetSize, boost::lambda::_1, boost::ref(sz)));
    return sz;
}

int BuildDvdOF::GetDVDSize()
{
    return Round(VideoSizeSum() / (double)(1024*1024));
}

void BuildDvdOF::SetProgress(double percent)
{
    SetStageProgress(percent);
}

//static void PrintRect(const Gdk::Rectangle& rct)
//{
//    io::cout.precision(8);
//    io::cout << rct.get_x() << " " << rct.get_y() << " " << rct.get_width() << " " << rct.get_height() << io::endl;
//}

} // namespace Author

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
    ASSERT( cond & StreamConditionMask );
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
    ASSERT( errno == need_errno );
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

ExitData ExecuteAsync(const char* dir, const char* cmd, Author::OutputFilter& of, GPid* pid)
{
    using namespace boost;
    ReadReadyFnr fnr = lambda::bind(&Author::OutputFilter::OnGetLine, &of,
                                    lambda::_1, lambda::_2, lambda::_3);
    return ExecuteAsync(dir, cmd, fnr, pid);
}

static void ThrowGError(const std::string& dsc, GError* err)
{
    std::string str(err->message);
    str += " (" + dsc + ")";
    throw std::runtime_error(str);
}

GPid Spawn(const char* dir, const char *commandline, int out_err[2], bool need_watch)
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
    argv[1] = "-c";
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
                                        0, out_err, out_err ? out_err+1 : 0, &error);
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

