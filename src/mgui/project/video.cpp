
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

bool IsTransVideo(VideoItem vi)
{
    return vi && RequireTranscoding(vi);
}

template <class Filter>
fe::range<VideoItem> FilteredVideos(Filter filter_fnr)
{
    RefPtr<MediaStore> md_store = GetAStores().mdStore;
    return fe::make_any( md_store->children() | fe::transformed(ToVITransform) | fe::filtered(filter_fnr) );
}

fe::range<VideoItem> AllVideos()
{
    return FilteredVideos(IsNotNull);
}

fe::range<VideoItem> AllTransVideos()
{
    return FilteredVideos(IsTransVideo);
}

} // namespace Project


