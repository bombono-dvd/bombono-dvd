//
// mlib/gettext.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2010 Ilya Murav'jov
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

#ifndef __MLIB_GETTEXT_H__
#define __MLIB_GETTEXT_H__

#include "format.h"

//
// Вначале использовал заголовок gi18n.h, но зависимость от glib стала мешать
// (в данном контексте), потому реализовал С_() заново. Основная трудность в том,
// что описание "магического" байта \004 есть в коде glib, mozilla, apple, но только
// не у автора (gettext). ИМХО причина в том, что gettext разделен на 2 части,-
// библиотечная (куда входит libintl.h) включена в (e)(g)libc, а "утилитарная" 
// (с бинарями msgfmt, xgettext, ...) идет отдельно. Так вот, библиотечная часть
// ниф^Wничего не знает о том байте и вообще о контекстных заменах, а значит расшифровывать
// сделанное msgfmt достается клиенту. Смотрим код gettext (message.h/write-mo.c) и
// делаем соответствующие выводы (разработчики gettext - изверги и покровители 
// велосипедостроения).
//

//#include <glib/gi18n.h>
#include <libintl.h>
#include <string.h> // strlen()

const char* _context_gettext_(const char* msgid, size_t msgid_offset);

#define  _(String) gettext(String)
#define N_(String) (String)
#define C_(Context,String) _context_gettext_(Context "\004" String, strlen(Context) + 1)
#define NC_(Context, String) (String)

// макрос наподобие N_(), только для xgettext
#define F_(String)

// укороченный вариант для i18n + Boost.Format
boost::format BF_(const char* str);
boost::format BF_(const std::string& str);

std::string _dots_(const char* str);
std::string _semicolon_(const char* str);

// вариант _() с добавлением точек в конце для пунктов меню
#define DOTS_(str) _dots_(str).c_str()
#define SMCLN_(str) _semicolon_(str).c_str()

#endif // #ifndef __MLIB_GETTEXT_H__


