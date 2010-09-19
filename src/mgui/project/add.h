
#ifndef __MGUI_PROJECT_ADD_H__
#define __MGUI_PROJECT_ADD_H__

#include "media-browser.h"

#include <mlib/string.h>

namespace Project
{

// интерактивный вариант TryAddMedia()
void TryAddMedias(const Str::List& paths, MediaBrowser& brw,
                  Gtk::TreePath& brw_pth, bool insert_after);
// desc - метка происхождения, добавления
void TryAddMediaQuiet(const std::string& fname, const std::string& desc);

// заполнить медиа в браузере
void PublishMedia(const Gtk::TreeIter& itr, RefPtr<MediaStore> ms, MediaItem mi);
void PublishMediaStore(RefPtr<MediaStore> ms);
void MediaBrowserAdd(MediaBrowser& brw, Gtk::FileChooser& fc);

void MuxAddStreams(const std::string& src_fname);

// ограничиваем возможность вставки верхним уровнем
// want_ia - где хотим вставить (dnd)
// возвращает - куда надо вставить (до или после)
bool ValidateMediaInsertionPos(Gtk::TreePath& brw_pth, bool want_ia = true);

} // namespace Project

namespace DVD {

void RunImport(Gtk::Window& par_win, const std::string& dvd_path = std::string());

} // namespace DVD

#endif // #ifndef __MGUI_PROJECT_ADD_H__

