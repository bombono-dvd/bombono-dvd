//
// mlib/sdk/memmem.h
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

#ifndef __MLIB_SDK_MEMMEM_H__
#define __MLIB_SDK_MEMMEM_H__

#include <cfg/config.h>

#include <stdint.h>
#include <mlib/tech.h>

typedef unsigned const char ucchar_t;

///////////////////////////////////////////////////////////
// Knuth-Morris-Pratt algorithm

// инициализация таблицы kmp_next
inline void PreKMP(ucchar_t* x, int m, int kmp_next[] )
{
    for( int i = 0, j = kmp_next[0] = -1; i<m ;  )
    {
        while( j > -1 && x[i] != x[j] ) 
            j = kmp_next[j]; 

        i++, j++; 
        kmp_next[i] = (x[i] == x[j]) ? kmp_next[j] : j ;
    }
}

inline ucchar_t* KMPMemMem(ucchar_t* haystack, int hs_len, 
                           ucchar_t* needle, int ndl_len)
{
    const int XSIZE = 32;
    ucchar_t* y = haystack;
    int n = hs_len;
    ucchar_t* x = needle;
    int m = ndl_len;


    // не меньше m должно быть
    ASSERT( XSIZE >= m );
    int kmp_next[XSIZE+1]; 

    PreKMP(x, m, kmp_next); 
    // сам поиск
    for( int i=0, j=0 ; i<n ; )
    {
        while( j>-1 && x[j] != y[i] ) 
            j = kmp_next[j];

        i++, j++; 
        if( j>=m )
        {
            // нашли очередное вхождение, OUTPUT( i - j );

            // в данном случае нам нужно первое вхождение
            return (y+(i-j));
            //j = kmp_next[j]; 
        }
    }

    // если дошли, значит нет вхождений
    return 0;
}

///////////////////////////////////////////////////////////
// Boyer-Moore algorithm (from Python)

/*
Copyright Python stringlib' authors
*/

// COPY_N_PASTE_ETALON

// #define FAST_COUNT 0
// #define FAST_SEARCH 1

inline ucchar_t* PyMemMem(ucchar_t* s, int n,
                          ucchar_t* p, int m /*, int mode*/)
{
    long mask;
    int skip  = 0; 
    //int count = 0;
    int i, j, mlast, w;

    w = n - m;

    if(w < 0)
        return 0;

    /* look for special cases */
    if(m <= 1)
    {
        if(m <= 0)
            return 0;
//         /* use special case for 1-character strings */
//         if(mode == FAST_COUNT)
//         {
//             for(i = 0; i < n; i++)
//                 if(s[i] == p[0])
//                     count++;
//             return count;
//         }
//         else
        {
            for(i = 0; i < n; i++)
                if(s[i] == p[0])
                    return s+i;
        }
        return 0;
    }

    mlast = m - 1;

    /* create compressed boyer-moore delta 1 table */
    skip = mlast - 1;
    /* process pattern[:-1] */
    for(mask = i = 0; i < mlast; i++)
    {
        mask |= (1 << (p[i] & 0x1F));
        if(p[i] == p[mlast])
            skip = mlast - i - 1;
    }
    /* process pattern[-1] outside the loop */
    mask |= (1 << (p[mlast] & 0x1F));

    for(i = 0; i <= w; i++)
    {
        /* note: using mlast in the skip path slows things down on x86 */
        if(s[i+m-1] == p[m-1])
        {
            /* candidate match */
            for(j = 0; j < mlast; j++)
                if(s[i+j] != p[j])
                    break;
            if(j == mlast)
            {
//                 /* got a match! */
//                 if(mode != FAST_COUNT)
//                     return i;
//                 count++;
//                 i = i + mlast;
//                 continue;
                return s+i;
            }
            /* miss: check if next character is part of pattern */
            if(!(mask & (1 << (s[i+m] & 0x1F))))
                i = i + m;
            else
                i = i + skip;
        }
        else
        {
            /* skip: check if next character is part of pattern */
            if(!(mask & (1 << (s[i+m] & 0x1F))))
                i = i + m;
        }
    }

//     if(mode != FAST_COUNT)
//         return -1;
//     return count;
    return 0;
}

///////////////////////////////////////////////////////////////////
//
// Поиск по образцу размером 3 байта, 0xXXXXXX00
// т.е. последний байт не учитывается
//
inline ucchar_t* MemPat3(ucchar_t* buf, int len, uint32_t pattern) 
{
    register uint32_t sample = 0xffffff00;
    register uint32_t pat = pattern & 0xffffff00;

    register ucchar_t* cur = buf;
    register ucchar_t* end = buf + len;

    for( ; cur<end ;  )
    {
        sample |= *cur++;
        sample <<= 8;

        if( sample == pat )
            return cur-3;
    }
    return 0;
}

inline ucchar_t* MemMem(ucchar_t* s, int n,
                        ucchar_t* p, int m)
{
    return PyMemMem(s, n, p, m);
}


#endif // #ifndef __MLIB_SDK_MEMMEM_H__

