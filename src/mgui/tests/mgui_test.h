//
// mgui/tests/mgui_test.h
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

#ifndef __MGUI_MGUI_TEST_H__
#define __MGUI_MGUI_TEST_H__

#include <mlib/tests/test_common.h>
#include <mgui/win_utils.h>

//
// Заготовка для быстрой проверки рисования (на примере Gtk::DrawingArea). 
// Варианты использования:
//  - проверка API (Gdk, Cairo, ...);
//  - воспроизведение и отлов ошибок отрисовки;
//  - тестирование кода (?)
// 
typedef bool (*DAExposeFunc)(Gtk::DrawingArea& da, GdkEventExpose* event);
void TestExampleDA(DAExposeFunc ef);

// Служебный класс для считывания агрументов из файла
class FileCmdLine
{
    public:
                    FileCmdLine(const char* fpath);
                   ~FileCmdLine();

               int  Count() { return arr.size(); }
             char** Words() { return words; }

    protected:
                  char** words;
      std::vector<char*> arr;
};

// получить аргументы, передаваемые Boost.Test при выполнении тестов
typedef std::pair<int, char**> ArgumentsPair;

inline ArgumentsPair GetBTArguments()
{
    boost::unit_test::auto_unit_test_suite_t* master_test_suite =
        boost::unit_test::auto_unit_test_suite();
    return ArgumentsPair(master_test_suite->argc, master_test_suite->argv);
}

// Тест-функции для обнаружения виджетов:
// путь вложения виджета
void PrintWidgetPath(Gtk::Widget& searched_wdg);
// найти виджет, владеющий фокусом
Gtk::Widget* GetFocusWidget(Gtk::Widget& some_wdg);

namespace DND
{

void PrintSelectionData(const Gtk::SelectionData& selection_data, const std::string& location_str);
void SetData(Gtk::SelectionData& selection_data, void* dat, int dat_sz);

} // namespace DND

#endif // __MGUI_MGUI_TEST_H__

