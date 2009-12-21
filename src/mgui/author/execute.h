//
// mgui/author/execute.h
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

#ifndef __MGUI_AUTHOR_EXECUTE_H__
#define __MGUI_AUTHOR_EXECUTE_H__

#include "indicator.h"

#include <mlib/patterns.h> // Singleton<>
#include <mlib/const.h>    // NO_HNDL
#include <mlib/ptr.h>
#include <mlib/string.h>

#include <mgui/mguiconst.h>

namespace Author
{

struct ExecState: public Singleton<ExecState>
{
              bool  isExec;

RefPtr<Gtk::Button> execBtn;
  Gtk::ProgressBar  prgBar;
        Gtk::Label  prgLabel;
     Gtk::TextView  detailsView;
              GPid  pid;

              Mode  mode;

       std::string  operationName;
       std::string  exitDesc;  // описание причины неудачного авторинга
              bool  userAbort; // пользователь сам отменил

       str::stream  settings; // = ASettings.py

              ExecState() { Init(); }
	
        void  Init();
 Gtk::Button& ExecButton() { return *UnRefPtr(execBtn); }

        void  Clean();
        void  Set(bool is_exec)
        {
            isExec = is_exec;
            if( !is_exec )
                pid = NO_HNDL; 
        }


        void  SetIndicator(double percent);
  Gtk::Label& SetStatus(const std::string& name = std::string());
};

inline ExecState& GetES() { return ExecState::Instance(); }
inline ExecState& GetInitedES() 
{
    ExecState& es = GetES();
    es.Init();
    return es; 
}

class OutputFilter;
class ProgressParser
{
    public:
                   ProgressParser(OutputFilter& of_): of(of_) {}
    virtual       ~ProgressParser() {}

    virtual  void  Filter(const std::string& line) = 0;

    protected:
            OutputFilter& of;
};

class OutputFilter
{
    public:
             virtual ~OutputFilter() {}
                      // сигнатура равна ReadReadyFnr
                void  OnGetLine(const char* buf, int sz, bool is_out);

       Gtk::TextView& GetTV() { return GetES().detailsView; }

        virtual void  SetStage(Stage stg) = 0;
                      // в мегабайтах
        virtual  int  GetDVDSize() = 0;
        virtual void  SetProgress(double percent) = 0;

    protected:
      ptr::one<ProgressParser> curParser;

                void  SetParser(ProgressParser* pp);
        virtual void  OnSetParser() {}
};

class BuildDvdOF: public OutputFilter
{
    public:

    virtual void  SetStage(Stage stg);
    virtual  int  GetDVDSize();
    virtual void  SetProgress(double percent);
};

Gtk::Widget& PackDetails(Gtk::TextView& txt_view);

typedef std::pair<Gtk::TextIter, Gtk::TextIter> TextIterRange;
typedef boost::function<void(RefPtr<Gtk::TextTag>)> InitTagFunctor;

TextIterRange AppendText(Gtk::TextView& txt_view, const std::string& text);
void ApplyTag(Gtk::TextView& txt_view, const TextIterRange& tir, 
              const std::string& tag_name, InitTagFunctor fnr);

template<typename T, template<typename R> class RefPtrT>
void ReRefPtr(RefPtrT<T>& p)
{
    RefPtr<T> new_p(new T);
    p.swap(new_p);
}

} // namespace Author

#define RG_BW "\\<"           // начало слова
#define RG_EW "\\>"           // конец  слова
#define RG_SPS "[[:space:]]*" // пробелы
#define RG_NUM "([0-9]+)"     // число
#define RG_CMD_BEG RG_BW // "^"RG_SPS  // начало команды

typedef boost::function<void(const char*, int, bool)> ReadReadyFnr;

struct ExitData
{
    bool  normExit; // нет в случае ненормального выхода (по сигналу, например)
     int  retCode;  // если !normExit, то это номер сигнала

     // вроде как больше 127 не может быть
     static const int impossibleRetCode = 128;

           ExitData(): normExit(true), retCode(impossibleRetCode) {}

      bool IsGood() const { return normExit && (retCode == 0); }
};

// line_up - вывод по строкам, а не по мере поступления данных
ExitData ExecuteAsync(const char* dir, const char* cmd, ReadReadyFnr& fnr, 
                      GPid* pid = 0, bool line_up = true);
ExitData ExecuteAsync(const char* dir, const char* cmd, Author::OutputFilter& of, GPid* pid);

void StopExecution(GPid& pid);
std::string ExitDescription(const ExitData& ed);

// COPY_N_PASTE_ETALON - симбиоз из:
// 1) g_spawn_command_line_async() - собственно выполнение
// 2) gnome_execute_shell()        - удобное преобразование в argv
//
// Изначально использовал свой gnome_execute_shell_redirect() 
// (подправленный gnome_execute_shell()), но из-за устаревания gnome_executeXXX
// перешел на g_spawnXXX 
// 
// need_watch - из-за специфики Unix (waitpid()) требуется явно указывать,
// будем ли наблюдать за запущенным процессом
GPid Spawn(const char* dir, const char *commandline, 
           int out_err[2] = 0, bool need_watch = false);

#endif // #ifndef __MGUI_AUTHOR_EXECUTE_H__

