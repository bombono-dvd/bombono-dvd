//
// mbase/project/handler.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#ifndef __MBASE_PROJECT_HANDLER_H__
#define __MBASE_PROJECT_HANDLER_H__

#include "media.h"

namespace Project
{

typedef ptr::shared<ObjVisitor> MHandler;

void RegisterHandler(MHandler hdlr, const char* action);
void InvokeOn(MediaItem mi, const char* action);

typedef boost::function<void(MediaItem mi, const char* action)> HookHandler;
void RegisterHook(HookHandler hook);

//
// Регистрация обработчиков
// 

#define DEFINE_REG_INV_HANDLER( action )           \
inline void Register##action(MHandler hdlr)     \
{                                               \
    RegisterHandler(hdlr, #action);             \
}                                               \
                                                \
inline void Invoke##action(MediaItem mi)        \
{                                               \
    InvokeOn(mi, #action);                      \
}                                               \
/**/

DEFINE_REG_INV_HANDLER( OnInsert )
DEFINE_REG_INV_HANDLER( OnChange )
DEFINE_REG_INV_HANDLER( OnDelete )

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_HANDLER_H__


