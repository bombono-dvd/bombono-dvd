//
// mlib/read_stream.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2010 Ilya Murav'jov
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

#ifndef __MLIB_READ_STREAM_H__
#define __MLIB_READ_STREAM_H__

#include "stream.h"
#include "filesystem.h"

#include <boost/function.hpp>

typedef boost::function<char*(char* buf, int buf_len)> ReadFunctor;
// чтение порциями по STRM_BUF_SZ
// fnr должен возвращать либо buf, либо 0 (если хотим прекратить чтение)
bool ReadStream(ReadFunctor fnr, io::stream& strm, io::pos len);
// указатель чтения должен быть в начале потока
bool ReadAllStream(ReadFunctor fnr, io::stream& strm);
std::string ReadAllStream(const fs::path& path);

// записать в fname содержимое str
void WriteAllStream(const fs::path& path, const std::string& str);

// Пример: скопировать n байт из одного потока в другой
//     ReadStream(MakeWriter(dst_strm), src_strm, n);
ReadFunctor MakeWriter(io::stream& dst_strm);

// MakeBufShifter() - последовательное заполнение буфера: buf += buf_len;
//
// Замечание: несовместим с ReadStream()!
// Функтор ReadFunctor стоит рассматривать не как строгий интерфейс,
// а как "разъем", через который можно взаимодейстовать совместимым между собой
// источникам и получателям
ReadFunctor MakeBufShifter();

#endif // #ifndef __MLIB_READ_STREAM_H__

