//
// mlib/const.h
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

#ifndef __MLIB_CONST_H__
#define __MLIB_CONST_H__

// Различные константы

// базовый размер для буферов файлов, демиксеров и т.д. 
const int STRM_BUF_SZ     = 4096;
const int MAX_STRM_BUF_SZ = STRM_BUF_SZ*256; // 1Mb

// ввод-вывод, для C-функций: open(), write(), lseek()
const int NO_HNDL  = -1; // неправильный хэндл, несущ. номер в массиве и т.д.
const int IN_HNDL  = 0;  // stdin
const int OUT_HNDL = 1;  // stdout

// разные константы
#define M_E     2.7182818284590452354	/* e */
#define M_PI    3.14159265358979323846	/* pi */
#define M_SQRT2	1.41421356237309504880	/* sqrt(2) */

#endif // #ifndef __MLIB_CONST_H__

