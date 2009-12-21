//
// mgui/editor/region.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008 Ilya Murav'jov
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

#include "region.h"
#include "kit.h"
#include "actions.h"

bool IsInArray(int pos, const int_array& arr)
{
    return arr.end() != std::find(arr.begin(), arr.end(), pos);
}

namespace Editor
{

// void MenuRegion::SetByMovieInfo(MovieInfo& m_inf)
// {
//     Point pasp = m_inf.PixelAspect();
//     bool res = TryConvertParams(mInf, FullDP(pasp, m_inf.Size()));
//     ASSERT(res); // если будут проблемы, заменить на FullDP
// }

Project::Menu Region::CurMenu() 
{ 
    if( curMRgn )
        return GetOwnerMenu(curMRgn);
    return Project::Menu(); // пусто
}

bool Region::IsSelObj(int pos)
{
    return IsInArray(pos, selArr);
}

void Region::SelObj(int pos)
{
    if( !IsSelObj(pos) )
        selArr.push_back(pos);
}

void Region::UnSelObj(int pos)
{
    std::vector<int>::iterator pos_it = std::find(selArr.begin(), selArr.end(), pos);
    if( pos_it != selArr.end() )
        selArr.erase(pos_it);
}

void Region::ClearSel()
{
    selArr.clear();
}

Point Region::GetAspectRadio() 
{ 
    return DisplayAspectOrDef(curMRgn); 
}

void Region::ClearPixbuf()
{
    if( curMRgn )
    {
        Editor::ClearInitTextVis vis(RefPtr<Gdk::Pixbuf>(0));
        curMRgn->Accept(vis);
    }
}

void Region::InitPixbuf()
{
    if( curMRgn )
    {
        Editor::ClearInitTextVis vis(Canvas());
        curMRgn->Accept(vis);
    }
}

} // namespace Editor
