//
// mlib/tests/test_common.cpp
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

////////////////////////////////////////////////////////////////////////////// 
// COPY_N_PASTE from libiberty (gcc)
// 
// сделал свою версию, чтобы gcc/ld не выдавал ненужных предупреждений
// "the use of tmpnam dangerous, better use 'mkstemp'"
// (потому как используем только в тестах)
// 
/*

@deftypefn Supplemental char* tmpnam (char *@var{s})

This function attempts to create a name for a temporary file, which
will be a valid file name yet not exist when @code{tmpnam} checks for
it.  @var{s} must point to a buffer of at least @code{L_tmpnam} bytes,
or be @code{NULL}.  Use of this function creates a security risk, and it must
not be used in new projects.  Use @code{mkstemp} instead.

@end deftypefn

*/

#include "test_common.h"

#include <stdio.h>
#include <unistd.h>

#ifndef L_tmpnam
#define L_tmpnam 100
#endif
#ifndef P_tmpdir
#define P_tmpdir "/usr/tmp"
#endif

static char tmpnam_buffer[L_tmpnam];
static int tmpnam_counter;

char* TmpNam(char* s, const char* prefix)
{
    int pid = getpid ();

    if(s == NULL)
        s = tmpnam_buffer;

    /*  Generate the filename and make sure that there isn't one called
        it already.  */

    while(1)
    {
        FILE *f;
        sprintf (s, "%s/%s%x.%x", P_tmpdir, prefix ? prefix : "t", pid, tmpnam_counter);
        f = fopen (s, "r");
        if(f == NULL)
            break;
        tmpnam_counter++;
        fclose (f);
    }

    return s;
}

std::string TmpFileNames::CreateName(const std::string& prefix)
{
   const char* res = TmpNam(0, prefix.c_str());
   std::string tmp(res);
   namArr.push_back( tmp );

   return std::string(res);
}

void TmpFileNames::ClearTemps()
{
   // удаляем файлы
   if( toRemove )
       for( NamArr::iterator iter=namArr.begin(), end = namArr.end() ; iter != end ; ++iter )
           remove( iter->c_str() );
   namArr.clear();
}
