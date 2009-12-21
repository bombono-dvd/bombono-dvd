//
// mgui/author/indicator.cpp
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

#include <mgui/_pc_.h>

#include "execute.h"
#include "script.h"  // CheckAuthorMode

#include <mgui/img_utils.h> // Round()
#include <mlib/string.h>

static std::string PercentString(double p)
{
    str::stream strm;
    // если хотим PercentString(1) => 01.00%
    //strm.width(5);
    //strm.fill('0');
    //strm << std::fixed;
    //strm.precision(2);

    return (strm << Round(p) << "%").str();
}

void SetPercent(Gtk::ProgressBar& bar, double percent)
{
    if( (0 <= percent) && (percent <= 100.) )
    {
        bar.set_fraction(percent/100.);
        bar.set_text(PercentString(percent));
    }
}

namespace Author
{

struct StageMapT
{
  std::string  stgName;  

         bool  isNeeded; // нужен ли этот этап
         bool  isPassed; // пройден ли

          int  weight;   // веса экспериментально подбирать
       double  dWeight;
} 
StageMap[stLAST] = 
{ 
    { "Rendering Menus",      true, true, 1, 0.0 }, 
    { "Generating DVD-Video", true, true, 3, 0.0 }, 
    { "Creating ISO Image",   true, true, 2, 0.0 }, 
    { "Burning DVD",          true, true, 4, 0.0 }, 
};
// текущий этап
Stage CurStage = stNO_STAGE;

void InitStageMap(Mode mod)
{
    StageMap[stDVDAUTHOR].isNeeded = mod != modRENDERING;
    StageMap[stBURN].isNeeded      = mod == modBURN;
    StageMap[stMK_ISO].isNeeded    = mod == modDISK_IMAGE;

    double sum = 0;
    for( int i=0; i<(int)ARR_SIZE(StageMap); i++ )
    {
        StageMapT& s = StageMap[i];
        s.isPassed = false;
        if( s.isNeeded )
            sum += s.weight;
    }
    ASSERT_RTL( sum );

    for( int i=0; i<(int)ARR_SIZE(StageMap); i++ )
        StageMap[i].dWeight = StageMap[i].weight / sum;

    CurStage = stRENDER;
}

void ExecState::SetIndicator(double percent)
{
    SetPercent(prgBar, percent);
}

void SetStageProgress(double percent)
{
    if( Project::CheckAuthorMode::Flag )
        return;

    double sum = 0;
    for( int i=0; i<(int)ARR_SIZE(StageMap); i++ )
    {
        StageMapT& s = StageMap[i];
        if( s.isNeeded && s.isPassed )
            sum += s.dWeight;
    }
    
    ASSERT( CurStage != stNO_STAGE );
    StageMapT& cur_s = StageMap[CurStage];
    ASSERT( cur_s.isNeeded && !cur_s.isPassed );

    sum = sum * 100. + cur_s.dWeight * percent;
    GetES().SetIndicator(sum);
}

static void SetPassed(Stage st)
{
    if( st != stRENDER )
        SetPassed(Stage(st-1));
    StageMap[st].isPassed = true;
}

const std::string& StageToStr(Stage st)
{
    StageMapT& s = StageMap[st];
    return s.stgName;
}

void SetStage(Stage st)
{
    if( Project::CheckAuthorMode::Flag )
        return;

    GetES().SetStatus(StageToStr(st));
    StageMapT& s = StageMap[st];
    ASSERT( s.isNeeded );

    SetPassed(st);
    s.isPassed = false;

    CurStage = st;
    SetStageProgress(0.);
}

} // namespace Author

