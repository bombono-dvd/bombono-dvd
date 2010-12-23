//
// mgui/execution.h
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

#ifndef __MGUI_EXECUTION_H__
#define __MGUI_EXECUTION_H__

#include <mlib/const.h>    // NO_HNDL
#include <mgui/timer.h>

namespace Execution {

struct Data 
{
    GPid  pid;
    bool  userAbort; // пользователь сам отменил

            Data();

            // в процессе выполнения внешней команды
            // установивший pid в конце должен его обнулить снова
      bool  IsAsyncCall() { return pid != NO_HNDL; }

      void  Init();
      void  StopExecution(const std::string& what);
};

class Pulse
{
    public:
    Pulse(Gtk::ProgressBar& prg_bar);
   ~Pulse();

    protected:
    Timer tm;
};

void SimpleSpawn(const char *commandline, const char* dir = 0);
void Stop(GPid& pid);

class ConsoleMode
{
    public:
        static bool Flag; // спец. режим для тестов (доп. проверки)

        ConsoleMode(bool turn_on = true);
       ~ConsoleMode();
    protected:
        bool origVal;
};

} // namespace Exection

// :TODO: перейти на сигнатуру void(const std::string&, bool) - 
// лишняя оптимизация (все равно RawRD нигде сейчас не используется)
typedef boost::function<void(const char*, int, bool)> ReadReadyFnr;
ReadReadyFnr TextViewAppender(Gtk::TextView& txt_view, 
                              const ReadReadyFnr& add_fnr = ReadReadyFnr());

struct ExitData
{
    bool  normExit; // нет в случае ненормального выхода (по сигналу, например)
     int  code;  // если !normExit, то это номер сигнала

     // вроде как больше 127 не может быть
     static const int impossibleRetCode = 128;

           ExitData(): normExit(true), code(impossibleRetCode) {}

      bool IsGood() const { return IsCode(0); }
      bool IsCode(int c) const;
};
// результат system() интерпретировать так
ExitData StatusToExitData(int status);
ExitData WaitForExit(GPid pid);
ExitData System(const std::string& cmd);

// line_up - вывод по строкам, а не по мере поступления данных
ExitData ExecuteAsync(const char* dir, const char* cmd, const ReadReadyFnr& fnr, 
                      GPid* pid = 0, bool line_up = true);

// записать в output весь (и out, и err!) вывод команды cmd
ExitData PipeOutput(const std::string& cmd, std::string& output);

std::string ExitDescription(const ExitData& ed);
// название в кавычках из-за пробелов (например)
std::string FilenameForCmd(const std::string& fname);

// COPY_N_PASTE_ETALON - симбиоз из:
// 1) g_spawn_command_line_async() - собственно выполнение
// 2) gnome_execute_shell()        - удобное преобразование в argv
//
// Изначально использовал свой gnome_execute_shell_redirect() 
// (подправленный gnome_execute_shell()), но из-за устаревания gnome_executeXXX
// перешел на g_spawnXXX 
// 
// need_wait - хотим ли знать/ждать статус окончания работы процесса; если да, то в той
// или иной форме родителю нужно вызывать waitpid(),- в том числе и для удаления этой информации из таблицы процессов
// (неосвобожденный таким образом закончивший работу процесс называют зомби); если нет, то инфо
// о статусе не сохранится по завершению процесса, и waitpid() вызывать не надо
//
// Если происходит ошибка создания процесса, то выкинется исключение std::runtime_error
GPid Spawn(const char* dir, const char *commandline, 
           int out_err[2] = 0, bool need_wait = false, int* in_fd = 0);

#endif // #ifndef __MGUI_EXECUTION_H__

