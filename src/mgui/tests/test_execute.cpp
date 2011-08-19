//
// mgui/tests/test_execute.cpp
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

#include <mgui/tests/_pc_.h>

#include "mgui_test.h"

#include <mgui/author/output.h>
#include <mgui/sdk/window.h>
#include <mgui/sdk/packing.h>
#include <mgui/sdk/textview.h>

#include <boost/test/floating_point_comparison.hpp>
#include <mlib/regex.h>

// максимальное кол-во работ, которое можно выполнять одновременно
// = кол-ву процессоров/ядер
int MaxCPUWorkload()
{
#ifdef _WIN32
#error "TODO"
#else
    // кол-во работающих процессоров, а не всего, _SC_NPROCESSORS_CONF 
    // (система может использовать не все)
    int res = sysconf(_SC_NPROCESSORS_ONLN);
    if( res <= 0 ) // может возвращать -1, если не осилит
        res = 1;
    return res;
#endif
}

static void CheckOutput(int fd, const std::string& etalon_str)
{
    int cnt = etalon_str.size();
    const int max_cnt = 10;
    BOOST_CHECK(max_cnt >= cnt+1);
    char buf[max_cnt];

    buf[0] = 0;
    BOOST_CHECK( read(fd, buf, cnt) == cnt );
    buf[cnt] = 0;
    //io::cout << "buf: " << buf << io::endl;
    BOOST_CHECK( etalon_str == buf );
    // признак конца потока - считали 0 байт!
    BOOST_CHECK( read(fd, buf, 1) == 0 );

    BOOST_CHECK( close(fd) == 0 );
}

static std::string SetupHelloCommand(std::string& ho_str, std::string& he_str)
{
  ho_str = "HelloOut!";
  he_str = "HelloErr!";
  return "echo -n " + ho_str + "; echo -n " + he_str + " >&2";
}

BOOST_AUTO_TEST_CASE( TestShellExecuteRedirect )
{
    std::string ho_str, he_str;
    std::string cmd_str = SetupHelloCommand(ho_str, he_str);

    int out_err[2] = {NO_HNDL, NO_HNDL};
    //int pid = gnome_execute_shell_redirect(0, cmd_str.c_str(), out_err);
    int pid = Spawn(0, cmd_str.c_str(), out_err);
    BOOST_CHECK( (pid > 0) && (out_err[0] != NO_HNDL) && (out_err[1] != NO_HNDL) );

    CheckOutput(out_err[0], ho_str);
    CheckOutput(out_err[1], he_str);
}

////////////////////////////////////////////////////////////////////////////////

static void AppendToStrings(const char* buf, int sz, bool is_out, 
                            std::string& out_str, std::string& err_str)
{
    (is_out ? out_str : err_str) += std::string(buf, sz);
}

static void TestAR(bool line_up)
{
    std::string ho_str, he_str;
    std::string cmd_str = SetupHelloCommand(ho_str, he_str);

    std::string out_str, err_str;
    ReadReadyFnr fnr = bb::bind(&AppendToStrings, _1, _2, _3, 
                                    boost::ref(out_str), boost::ref(err_str));
    ExitData ed = ExecuteAsync(0, cmd_str.c_str(), fnr, 0, line_up);

    BOOST_CHECK( ed.IsGood() );
    BOOST_CHECK_EQUAL( ho_str, out_str );
    BOOST_CHECK_EQUAL( he_str, err_str );
}

// функционал выделен в 2 теста ради тестирования конфликта Boost.Test и GTK:
// при запуске тестов без --catch_system_errors=no B.T. перед выполнением каждого
// теста (!) химичит с UNIX-сигналами, и из-за этого ExecuteAsyncImpl() не прекращает работу,-
// не приходит событие завершения процесса в WaitExitCode()
//
// Долбаные сигналы UNIX! Вы - проклятье всех POSIX-систем из-за того, что вами никто
// нормально пользоваться не умеет.
BOOST_AUTO_TEST_CASE( TestAsyncReadingFalse )
{
    InitGtkmm();
    TestAR(false);
}

BOOST_AUTO_TEST_CASE( TestAsyncReadingTrue )
{
    InitGtkmm();
    TestAR(true);
}

////////////////////////////////////////////////////////////////////////////////

namespace Author {

static void SetConsoleState(ExecState& es, bool is_exec)
{
    es.Set(is_exec);
    Gtk::Button& btn = es.ExecButton();
    btn.set_label(is_exec ? "_Abort Command" : "_Execute Command");
    btn.set_use_underline(true);
}

class ConsoleSetter
{
    public:
        ConsoleSetter(ExecState& es_): es(es_) 
        { SetConsoleState(es, true); }
       ~ConsoleSetter()
        { SetConsoleState(es, false); }

    protected:
    ExecState& es;
};

ReadReadyFnr OF2RRF(OutputFilter& of);

static void OnExecuteCommand(Gtk::ComboBoxEntryText& cmd_ent, 
                             Gtk::FileChooserButton& ch_btn)
{
    ExecState& es = GetES();
    Execution::Data& edat = es.eDat;
    if( !es.isExec )
    {
        ConsoleSetter ess(es);

        Gtk::Entry& ent = *cmd_ent.get_entry();
        std::string cmd = ent.get_text().raw();
        if( cmd.size() )
        {
            Gtk::TextView& txt_view = es.detailsView;
            // переставим команду наверх по истории
            cmd_ent.remove_text(cmd);
            cmd_ent.prepend_text(cmd);
            ent.set_text(""); // очищаем
            AppendCommandText(txt_view, "Execute Command: '" + cmd + "'");

            ConsoleOF cof;
            ExitData ed = ExecuteAsync(ch_btn.get_filename().c_str(), cmd.c_str(), OF2RRF(cof), &edat.pid);


            std::string exit_str = ExitDescription(ed);
            AppendCommandText(txt_view, "Exit Status: " + exit_str + ".");
            es.SetStatus(ed.IsGood() ? std::string() : exit_str);
        }
    }
    else
        edat.StopExecution("\"command\"");
}

BOOST_AUTO_TEST_CASE( TestInteractiveExecute )
{
    return;
    InitGtkmm();

    Gtk::Window win;
    SetAppWindowSize(win, 640);

    ExecState& es = GetInitedES();
    Gtk::TextView& txt_view = es.detailsView; //NewManaged<Gtk::TextView>();

    Gtk::ComboBoxEntryText& cmd_ent = NewManaged<Gtk::ComboBoxEntryText>();
    Gtk::Entry& ent = *cmd_ent.get_entry();
    ent.set_activates_default(true);
    cmd_ent.signal_changed().connect(bb::bind(&GrabFocus, boost::ref(ent)));
    Gtk::FileChooserButton& ch_btn = NewManaged<Gtk::FileChooserButton>("Select folder for execution", 
                                                                        Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    //ch_btn.set_filename(Project::MakeAbsolutePath("../t").string());

    Gtk::VBox& vbox = Add(win, NewManaged<Gtk::VBox>(false, 2));
    {
        Gtk::HBox& hbox = PackStart(vbox, NewManaged<Gtk::HBox>(false, 2));

        PackStart(hbox, NewManaged<Gtk::Label>("Command:"));
        PackStart(hbox, cmd_ent, Gtk::PACK_EXPAND_WIDGET);
        PackStart(hbox, ch_btn);
    }
    PackStart(vbox, PackDetails(txt_view), Gtk::PACK_EXPAND_WIDGET);
    Project::PackProgressBar(vbox, es);

    Gtk::Button& btn = es.ExecButton();
    SetConsoleState(es, false);
    btn.signal_clicked().connect(bb::bind(&OnExecuteCommand, boost::ref(cmd_ent), boost::ref(ch_btn)));
    PackStart(vbox, btn);
    SetDefaultButton(btn);

    RunWindow(win);
}

BOOST_AUTO_TEST_CASE( TestIndicator )
{
    InitGtkmm();
    // веса 0+1+3+6=10
    InitStageMap(modBURN, 0);
    Gtk::ProgressBar& bar = GetES().prgBar;
    //bar.set_fraction(percent/100.);
    double stage_step = 1/10.0;
    double trans_dur  = 0.;

    SetStage(stTRANSCODE);
    BOOST_CHECK_EQUAL( bar.get_fraction(), 0 );
    SetStage(stRENDER);
    BOOST_CHECK_EQUAL( bar.get_fraction(), trans_dur );
    SetStageProgress(0.5);
    BOOST_CHECK_CLOSE( bar.get_fraction(), trans_dur + stage_step * 0.5, 0.001 );

    SetStage(stDVDAUTHOR);
    BOOST_CHECK_CLOSE( bar.get_fraction(), trans_dur + stage_step, 0.001 );
    SetStageProgress(0.8);
    BOOST_CHECK_CLOSE( bar.get_fraction(), trans_dur + stage_step * (1 + 3*0.8), 0.001 );

    SetStage(stBURN);
    BOOST_CHECK_CLOSE( bar.get_fraction(), trans_dur + stage_step * 4, 0.001 );
    SetStageProgress(1.0);
    BOOST_CHECK_EQUAL( bar.get_fraction(), 1 );
}

} // namespace Author

////////////////////////////////////////////////////////////////////////////////

// purpose:
// takes the contents of a file in the form of a string
// and searches for all the C++ class definitions, storing
// their locations in a map of strings/int's

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;

const char* re_text = 
    // possibly leading whitespace:   
    "^[[:space:]]*" 
    // possible template declaration:
    "(template[[:space:]]*<[^;:{]+>[[:space:]]*)?"
    // class or struct:
    "(class|struct)[[:space:]]*" 
    // leading declspec macros etc:
    "("
      "\\<\\w+\\>"
      "("
         "[[:blank:]]*\\([^)]*\\)"
      ")?"
      "[[:space:]]*"
    ")*" 
    // the class name
    "(\\<\\w*\\>)[[:space:]]*" 
    // template specialisation parameters
    "(<[^;:{]+>)?[[:space:]]*"
    // terminate in { or :
    "(\\{|:[^;\\{()]*\\{)";

re::pattern expression(re_text);

void IndexClasses(map_type& m, const std::string& file)
{
    std::string::const_iterator start, end;
    start = file.begin();
    end = file.end();   
    re::match_results what;
    re::constants::match_flag_type flags = re::constants::match_default;
    while( re::search(start, end, what, expression, flags) )
    {
        // what[0] contains the whole string
        // what[5] contains the class name.
        // what[6] contains the template specialisation if any.
        // add class name and position to map:
        m[std::string(what[5].first, what[5].second) + std::string(what[6].first, what[6].second)] = 
        what[5].first - file.begin();      
        // update search position:
        start = what[0].second;      
        // update flags:
        flags |= boost::match_prev_avail;
        flags |= boost::match_not_bob;

        io::cout << "\n\nFound declaration: " << io::endl;
        for( int i=0; i<(int)what.size(); i++ )
            io::cout << "what[" << i << "]: \"" <<  what.str(i) << "\""<< io::endl;
    }
}

void load_file(std::string& s, io::stream& is)
{
    s.erase();
    if (is.bad()) return;
    s.reserve(is.rdbuf()->in_avail());
    char c;
    while (is.get(c))
    {
        if (s.capacity() == s.size())
            s.reserve(s.capacity() * 3);
        s.append(1, c);
    }
}

//int main(int argc, const char** argv)
BOOST_AUTO_TEST_CASE( TestRegex )
{
    return;
    int argc = 2;
    const char* argv[] = { 0, "src/mgui/author/execute.h" }; 

    std::string text;
    for (int i = 1; i < argc; ++i)
    {
        io::cout << "Processing file " << argv[i] << io::endl;
        map_type m;
        io::stream fs(argv[i]);
        load_file(text, fs);
        fs.close();
        IndexClasses(m, text);
        io::cout << m.size() << " matches found" << io::endl;
        map_type::iterator c, d;
        c = m.begin();
        d = m.end();
        while (c != d)
        {
            io::cout << "class \"" << (*c).first << "\" found at index: " << (*c).second << io::endl;
            ++c;
        }
    }
    //return 0;
}


