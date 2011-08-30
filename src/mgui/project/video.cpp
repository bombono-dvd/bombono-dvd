//
// mgui/project/video.cpp
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

#include <mgui/_pc_.h>

#include "video.h"

#include <mgui/init.h>

namespace Project
{

static VideoItem ToVITransform(const Gtk::TreeRow& row) 
{
    return IsVideo( MediaStore::Get(row) );
}

static bool IsNotNull(const VideoItem& vi)
{
    return bool(vi);
}

bool IsTransVideo(VideoItem vi, bool require_vtc)
{
    return vi && (require_vtc ? RequireVideoTC(vi) : RequireTranscoding(vi));
}

template <class Filter>
fe::range<VideoItem> FilteredVideos(Filter filter_fnr)
{
    RefPtr<MediaStore> md_store = GetMediaStore();
    return fe::make_any( md_store->children() | fe::transformed(ToVITransform) | fe::filtered(filter_fnr) );
}

fe::range<VideoItem> AllVideos()
{
    return FilteredVideos(IsNotNull);
}

fe::range<VideoItem> AllTransVideosEx(bool require_vtc)
{
    return FilteredVideos(bb::bind(&IsTransVideo, _1, require_vtc));
}

fe::range<VideoItem> AllTransVideos()
{
    return AllTransVideosEx(false);
}

fe::range<VideoItem> AllVTCVideos()
{
    return AllTransVideosEx(true);
}


} // namespace Project


