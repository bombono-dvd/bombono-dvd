//
// mlib/sdk/system.h
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

#ifndef __MLIB_SDK_SYSTEM_H__
#define __MLIB_SDK_SYSTEM_H__

#include <mlib/stream.h>

// получить размер памяти, занимаемой программой (в байтах)
int GetMemSize();

// получить время, прошедшее с начала работы процесса (в секундах)
// при этом подсчитывается только время работы процесса, поэтому,
// например, вызовы sleep() влияния не окажут
// Замечания:
//  - используется clock();
//  - переполнение счетчика (clock_t) происходит примерно через 4000сек,
//    т.е. данные адекватны в пределах чуть более часа. 
double GetClockTime();

#endif // #ifndef __MLIB_SDK_SYSTEM_H__


