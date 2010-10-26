//
// mgui/author/script.h
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

#ifndef __MGUI_AUTHOR_SCRIPT_H__
#define __MGUI_AUTHOR_SCRIPT_H__

#include "execute.h"

#include <mbase/project/media.h>
#include <mbase/project/menu.h>
#include <mbase/composite/component.h>

#include <mlib/filesystem.h> // fs::path

#include <boost/function.hpp>

namespace Project
{

typedef boost::function<bool(VideoItem, int)> VideoFnr;
int ForeachVideo(VideoFnr fnr);

typedef boost::function<bool(Menu, int)> MenuFnr;
void ForeachMenu(MenuFnr fnr);

void AuthorMenus(const std::string& out_dir);
// run_all - включая запуск внешшней команды сборки
std::string AuthorDVD(const std::string& out_dir);

#define AUTHOR_TAG "Authoring"

std::string MenuAuthorDir(Menu mn, int i, bool cnv_from_utf8 = true);
fs::path SConsAuxDir();

bool HasButtonLink(Comp::MediaObj& m_obj, std::string& targ_str);

bool IsMotion(Menu mn);
guint64 MenuSize(Menu mn);
double MenuDuration(Menu mn);

void ClearTaggedData(Menu mn, const char* tag);

} // namespace Project

namespace Author {

void Warning(const std::string& str);
void Info(const std::string& str, bool add_info_sign = true);

void FillSconsOptions(str::stream& scons_options, bool fill_def);
void ExecuteSconsCmd(const std::string& out_dir, OutputFilter& of, 
                     Mode mod, const str::stream& scons_options);
// вызов внешней команды в процессе авторинга
// устанавливает GetES().eDat.pid на время работы, потому нельзя выполнять 
// одновременно более одной команды (а если потребуется, то придется держать
// список идентификаторов процессов)
ExitData AsyncCall(const char* dir, const char* cmd, const ReadReadyFnr& fnr);

io::pos ProjectSizeSum();

// функтор сообщает об ошибках через исключение, которое преобразуется
// в строку => признак ошибки - непустой результат
std::string SafeCall(const ActionFunctor& fnr);
inline bool IsGood(const std::string& res) { return res.empty(); }

void Error(const std::string& str);
void Error(const std::string& msg, const std::string& reason);
void ErrorByED(const std::string& msg, const ExitData& ed);
void CheckAbortByUser();

} // namespace Author

#endif // #ifndef __MGUI_AUTHOR_SCRIPT_H__


