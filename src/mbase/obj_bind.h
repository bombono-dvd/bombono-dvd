//
// mbase/obj_bind.h
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

#ifndef __MBASE_OBJ_BIND_H__
#define __MBASE_OBJ_BIND_H__

#include "composite/component.h"

#include <mlib/function.h>

typedef boost::function<void(Comp::Object*)> CompObjectFunctor;
void ForeachLinked(Project::MediaItem mi, CompObjectFunctor fnr);

namespace Composition {
typedef boost::function<void(FramedObj&)> FOFunctor;
}

void ForeachWithPoster(Project::MediaItem mi, Composition::FOFunctor fnr);

// Замечание: в принципе можно перебирать вручную все пункты меню (цикл по меню,
// затем по пунктам меню), однако могут быть проблемы при удалении меню/пунктов в процессе
// перебора

#endif // #ifndef __MBASE_OBJ_BIND_H__

