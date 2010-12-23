//
// mgui/author/indicator.h
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

#ifndef __MGUI_AUTHOR_INDICATOR_H__
#define __MGUI_AUTHOR_INDICATOR_H__

//#include <mlib/patterns.h> // Singleton<>
//#include <mlib/const.h>    // NO_HNDL
//#include <mgui/mguiconst.h>

namespace Author
{

// режим авторинга на вкладке Output
enum Mode
{
    modFOLDER,
    modDISK_IMAGE,
    modBURN,
    modRENDERING
};

enum Stage
{
    stNO_STAGE   = -1,
    stBEG_STAGE  = 0,

    stTRANSCODE  = stBEG_STAGE,
    stRENDER     = 1,
    stDVDAUTHOR  = 2,
    stMK_ISO     = 3,
    stBURN       = 4,

    stLAST
};

// trans_ratio - в долях 1 от размера проекта
void InitStageMap(Mode mod, double trans_ratio);
void SetStage(Stage st);
// is_percent = false : в долях 1, иначе в %
void SetStageProgress(double val, bool is_percent = false);

std::string StageToStr(Stage st);

} // namespace Author

#endif // #ifndef __MGUI_AUTHOR_INDICATOR_H__

