//
// mgui/project/browser.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2010 Ilya Murav'jov
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

#include "browser.h"
#include "handler.h"
#include "dnd.h"
#include "mconstructor.h" // GetAStores()

#include <mgui/sdk/packing.h>
#include <mgui/dialog.h>
#include <mgui/key.h>
#include <mgui/execution.h>
#include <mgui/gettext.h>
#include <mgui/prefs.h> // PreferencesPath()

#include <mbase/resources.h>
#include <mbase/project/menu.h>
#include <mbase/project/table.h>  // AData()

#include <mlib/read_stream.h>
#include <mlib/sdk/logger.h>

namespace Project
{

Gtk::TreePath& ObjectStore::LocalPath(const Gtk::TreeIter& itr) const
{
    return Project::LocalPath(GetMedia(itr).get());
}

bool ObjectStore::drag_data_received_vfunc(
    const TreeModel::Path& dst_path, const Gtk::SelectionData& data)
{
    bool do_drop = MyParent::drag_data_received_vfunc(dst_path, data);
    if( do_drop )
    {
        //
        // Учитываем, что медиа:
        // - вставлено на новое место dst_path, но еще не удалено
        //   со старого места;
        // - если переносили вниз, то старое место - src_path,
        //   иначе - src_path+1
        //
        ASSERT( dst_path.size() == 1 );
        Gtk::TreePath src_path = GetSourcePath(data);
        Gtk::TreePath old_path = LocalPath(get_iter(dst_path));
        if( old_path != src_path )
        {
            LOG_ERR << "Error on dragging media, old != src: " << old_path.to_string() 
                    << " != " << src_path.to_string() << io::endl;
            ASSERT(0);
        }
        SyncOnDragReceived(dst_path, src_path, MakeRefPtr(this));
    }
    return do_drop;
}

// только для путей верхнего уровня
Gtk::TreePath& LocalPath(Media* mi)
{
    return mi->GetData<Gtk::TreePath>();
}

// переиндексируем все, начиная с этого
void ReindexFrom(RefPtr<ObjectStore> os, const Gtk::TreeIter& from_itr)
{
    if( !from_itr )
        return;

    Gtk::TreeIter itr(from_itr);
    Gtk::TreePath path = os->get_path(itr);
    for( ; itr; ++itr, path.next() )
        os->LocalPath(itr) = path;
}

void SyncOnDragReceived(const Gtk::TreePath& dst_path, const Gtk::TreePath& src_path, RefPtr<ObjectStore> os)
{
    // направление перемещения
    int src_num = src_path[0];
    int dst_num = dst_path[0];

    if( (src_num != dst_num) && (src_num+1 != dst_num) )
    {
        if( src_num < dst_num )
        {
            // вниз
            Gtk::TreeIter itr = os->get_iter(src_path);
            for( Gtk::TreePath path = src_path, prev_path = src_path; 
                 path.next(), ++itr, path <= dst_path ; prev_path = path )
                os->LocalPath(itr) = prev_path;
        }
        else
        {
            // вверх
            Gtk::TreeIter itr = os->get_iter(dst_path);
            for( Gtk::TreePath path = dst_path; path <= src_path ; path.next(), ++itr )
                os->LocalPath(itr) = path;
        }
        InvokeOn(GetMedia(os, dst_path), "Reorder");
    }
}

Gtk::TreePath GetSourcePath(const Gtk::SelectionData& data)
{
    Gtk::TreePath src_path;
    bool true_ = Gtk::TreePath::get_from_selection_data(data, src_path);
    ASSERT_OR_UNUSED( true_ );

    return src_path;
}

const char* ConfirmQuestions[] = {
    N_("Do you really want to delete \"%1%\" from Media List?"),
    N_("Do you really want to delete chapter \"%1%\"?"),
    N_("Do you really want to delete menu \"%1%\"?")
};

class DelConfirmationStrVis: public ObjVisitor
{
    public:
        std::string templStr;

        virtual  void  Visit(StillImageMD&)   { templStr = ConfirmQuestions[0]; }
        virtual  void  Visit(VideoMD&)        { templStr = ConfirmQuestions[0]; }
        virtual  void  Visit(VideoChapterMD&) { templStr = ConfirmQuestions[1]; }
        virtual  void  Visit(MenuMD&)         { templStr = ConfirmQuestions[2]; }
};

static std::string GetDelConfirmationStr(MediaItem md)
{
    DelConfirmationStrVis vis;
    md->Accept(vis);
    return BF_(vis.templStr) % md->mdName % bf::stop;
}

bool ConfirmDeleteMedia(MediaItem mi)
{
    return Gtk::RESPONSE_OK == MessageBox(GetDelConfirmationStr(mi), 
                                          Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
}

void DeleteMedia(RefPtr<ObjectStore> os, const Gtk::TreeIter& itr)
{
    LOG_INF << "Delete Media!" << io::endl;
    MediaItem mi = os->GetMedia(itr);

    Gtk::TreeIter next_itr = os->erase(itr);
    if( !IsChapter(mi) )
        ReindexFrom(os, next_itr);

    DeleteMedia(mi);
}

bool IsControlKey(int state)
{
    return (state & SH_CTRL_ALT_MASK) == GDK_CONTROL_MASK;
}

MediaItem GetCurMedia(ObjectBrowser& brw)
{
    MediaItem mi;
    Gtk::TreePath path = GetCursor(brw);
    if( !path.empty() )
        mi = GetMedia(brw.GetObjectStore(), path);
    return mi;
}

class RedrawThumbnailVis: public ObjVisitor
{
    public:
    virtual   void  Visit(MenuMD& obj);
    virtual   void  Visit(VideoMD& obj);

    virtual   void  Visit(StillImageMD&)   { ASSERT(0); } // не нужно
    virtual   void  Visit(VideoChapterMD&) { ASSERT(0); }
};

RefPtr<MediaStore> GetMediaStore()
{
    return GetAStores().mdStore;
}

void RedrawThumbnailVis::Visit(VideoMD& obj)
{
    void FillThumbnail(RefPtr<MediaStore> ms, StorageItem si);
    FillThumbnail(GetMediaStore(), &obj);
}

void RedrawThumbnailVis::Visit(MenuMD& obj)
{
    void FillThumbnail(const Gtk::TreeIter& itr, RefPtr<MenuStore> ms, bool force_thumb = false);
    RefPtr<MenuStore> ms = GetAStores().mnStore;
    FillThumbnail(ms->get_iter(LocalPath(&obj)), ms, true);
}

void RedrawThumbnail(MediaItem mi)
{
    if( mi )
    {
        RedrawThumbnailVis vis;
        mi->Accept(vis);
    }
}

bool ObjectBrowser::on_key_press_event(GdkEventKey* event)
{
    // :TODO: "key_binding" - см. meditor_text.cpp
    switch( event->keyval )
    {
    case GDK_Delete:  case GDK_KP_Delete:
        if( CanShiftOnly(event->state) )
            DeleteMedia();
        break;
    case GDK_E: case GDK_e:
        if( IsControlKey(event->state) ) // :DOC: Ctrl+E = установка начального (Entrance) медиа
            if( MediaItem mi = GetCurMedia(*this) )
            {
                if( IsVideo(mi) || IsMenu(mi) )
                {
                    MediaItem& fp = AData().FirstPlayItem();
                    MediaItem old_fp = fp;
                    if( fp != mi )
                    {
                        fp = mi;
                        RedrawThumbnail(fp);
                    }
                    else
                        fp = MediaItem();
                    RedrawThumbnail(old_fp);
                }
                else
                    MessageBox(_("First-Play media can be Video or Menu only."), 
                               Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
            }
        break;
    default:
        break;
    }

    return MyParent::on_key_press_event(event);
}

bool ObjectBrowser::on_drag_motion(const RefPtr<Gdk::DragContext>& context, int x, int y, guint time)
{
    if( drag_dest_find_target(context) == MediaItemDnDTVType())
    {
        // не такой тип только для внешнего использования
        context->drag_refuse(time);
        return false;
    }
    return MyParent::on_drag_motion(context, x, y, time);
}

Gtk::TreePath GetCursor(Gtk::TreeView& brw)
{
    Gtk::TreePath pth;
    Gtk::TreeViewColumn* col = 0;
    brw.get_cursor(pth, col);
    ValidatePath(pth);

    return pth;
}

Gtk::TreeIter InsertByPos(RefPtr<ObjectStore> os, const Gtk::TreePath& pth,
                          bool after)
{
    Gtk::TreeIter itr;
    if( !pth.empty() )
    {
        itr = os->get_iter(pth);
        if( after )
            ++itr;
    }
    if( !itr )
        itr = os->children().end();
    return os->insert(itr);
}

void GoToPos(ObjectBrowser& brw, const Gtk::TreePath& pth)
{
    brw.set_cursor(pth);
    brw.grab_focus();
}

Gtk::TreeIter GetSelectPos(Gtk::TreeView& tv)
{
    RefPtr<Gtk::TreeSelection> sel = tv.get_selection();
    Gtk::TreeIter res;
    if( sel->get_mode() == Gtk::SELECTION_MULTIPLE )
    {
        boost_foreach( const Gtk::TreePath& pth, sel->get_selected_rows() )
        {
            res = tv.get_model()->get_iter(pth);
            break;
        }
    }
    else
        res = sel->get_selected();
    return res;
}

Gtk::HButtonBox& CreateMListButtonBox()
{
    Gtk::HButtonBox& hb = *Gtk::manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_START, 2));
    //hb.set_child_min_width(70);
    hb.set_name("MListButtonBox");

    return hb;
}

std::string MediaItemDnDTVType() { return "DnDTreeView<"DND_MI_NAME">"; }

void SetupBrowser(ObjectBrowser& brw, int dnd_column, bool is_media_brw)
{
    // если хотим "полосатый" браузер
    //brw.set_rules_hint();
    brw.set_headers_visible(is_media_brw);
    brw.get_selection()->set_mode(is_media_brw ? Gtk::SELECTION_MULTIPLE : Gtk::SELECTION_BROWSE);

    // * DND
    //brw.set_reorderable(true); // не нужно, для DndTreeView автоматом включено
    if( dnd_column != -1 )
        brw.add_object_drag(dnd_column, MediaItemDnDTVType());
}

static void OnTitleEdited(RefPtr<ObjectStore> os, const Glib::ustring& path_str, 
                          const Glib::ustring& new_title)
{
    MediaItem mi = GetMedia(os, Gtk::TreePath(path_str));
    DoNameChange(mi, new_title);
}

static std::string RenderMediaName(MediaItem mi)
{
    return mi->mdName;
}

void SetupNameRenderer(Gtk::TreeView::Column& name_cln, Gtk::CellRendererText& rndr, 
                     RefPtr<ObjectStore> os)
{
    name_cln.pack_start(rndr);
    rndr.property_editable() = true;
    rndr.signal_edited().connect( bb::bind(&OnTitleEdited, os, _1, _2) );
    SetRendererFnr(name_cln, rndr, os, &RenderMediaName);
}

} // namespace Project

void PackHSeparator(Gtk::VBox& vbox)
{
    vbox.pack_start(NewManaged<Gtk::HSeparator>(), Gtk::PACK_SHRINK);
}

Gtk::ScrolledWindow& PackInScrolledWindow(Gtk::Widget& wdg, bool need_hz)
{
    Gtk::ScrolledWindow& scr_win = *Gtk::manage(new Gtk::ScrolledWindow);
    scr_win.set_shadow_type(Gtk::SHADOW_NONE); //SHADOW_OUT); //IN);
    scr_win.set_policy( need_hz ? Gtk::POLICY_AUTOMATIC : Gtk::POLICY_NEVER, 
                        Gtk::POLICY_AUTOMATIC );

    scr_win.add(wdg);
    return scr_win;
}


//////////////////////////////////////////////////////////////////////////
// DnD для URI

struct UriDropFunctorImpl
{
    UriDropFunctorImpl(const UriDropFunctor& f_): fnr(f_) {}

    void operator()(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, 
                    const Gtk::SelectionData& data, guint , guint time)
    {
        //
        // В функции get_uris() проверяется тип dnd (подходит только "text/uri-list"),
        // поэтому доп. проверки нет.
        // Замечание: судя по коду ardour (см. функцию convert_drop_to_paths()), раньше были
        // "дропы" и с типом "text/uri-list" (от Nautilus'а) - если будут проблемы можно 
        // попробовать convert_drop_to_paths() вместо get_uris()
        //

        URIList uris = data.get_uris();
        if( !uris.empty() )
        {
            StringList paths;
            bool is_new;
            for( URIList::iterator itr = uris.begin(), end = uris.end(); itr != end ; ++itr )
                paths.push_back(Uri2LocalPath(*itr, is_new));

            //io::stream out_strm("../ttt", iof::out);
            //for( StringList::iterator itr = paths.begin(), end = paths.end(); itr != end; ++itr )
            //    out_strm << "filepath = " << *itr << io::endl;

            context->drag_finish(true, false, time);

            fnr(paths, Point(x, y));
        }
    }
    protected:
        UriDropFunctor fnr;
};

void ConnectOnDropUris(Gtk::Widget& dnd_wdg, const UriDropFunctor& fnr)
{
    dnd_wdg.signal_drag_data_received().connect(UriDropFunctorImpl(fnr));
}

// picture.jpg
// picture#2.jpg
// picture#3.jpg
// ...
static std::string CloneName(const std::string& basename, int i)
{
    std::string res;
    if( i == 1 )
        res = basename;
    else
    {
        std::string suffix = "#" + Int2Str(i);
        const char* name = basename.c_str();
        const char* dot  = FindExtDot(name);
        if( dot )
            res = std::string(name, dot) + suffix + std::string(dot);
        else
            res = basename + suffix;
    }
    return res;
}

static char* CompareWithStream(char* buf, int sz, char* tmp_buf, io::stream& strm)
{
    int sz2 = strm.raw_read(tmp_buf, sz);

    bool res = (sz == sz2) && memcmp(buf, tmp_buf, sz) == 0;
    return res ? buf : 0 ;
}

// завершение асинхронной операции планировать невозможно, даже после ее отмены;
// потому нужно хранить данные о ситуации в куче (во избежание порчи стека)
struct IOJob
{
    // :TRICKY: часть данных используется только в одной из операций
    RefPtr<Gio::File> obj; // copy_async: нужен для удаления результатов неудачной операции
RefPtr<Gio::FileInfo> inf; // query_info_async
                bool  copyRes; // copy_async
                
                bool  userAbort;
                bool  exitDone;
         std::string  errStr;
    
    IOJob(RefPtr<Gio::File> obj_): copyRes(true), exitDone(false), obj(obj_), userAbort(false) {}
};

typedef ptr::shared<IOJob> IOJobPtr;

static void DoExit(IOJobPtr ij)
{
    bool& ed = ij->exitDone;
    if( !ed )
    {
        ed = true;
        Gtk::Main::quit();
    }
}

void SafeRemove(const std::string& fpath)
{
    try
    {
        fs::remove(fpath);
    }
    catch( const std::exception& )
    {}
}

struct GErrorTwin
{
    GError* gErr;
    
    GErrorTwin(): gErr(0) {}
   ~GErrorTwin()
    {
        if( gErr )
            g_error_free(gErr);
    }
};

GError* Ptr(GErrorTwin& gt) {  return gt.gErr; }
GError** PP(GErrorTwin& gt) {  return &gt.gErr; }

static void ExitJob(IOJobPtr ij, GErrorTwin& gt)
{
    if( Ptr(gt) )
        ij->errStr = Ptr(gt)->message;
    DoExit(ij);
}

static void OnAsyncCopyEnd(RefPtr<Gio::AsyncResult> ar, IOJobPtr ij)
{
    bool& res = ij->copyRes;
    RefPtr<Gio::File> dst = ij->obj;
    //// Gtkmm завел моду всегда кидать исключения в случае пользовательских проблем
    //try
    //{
    //    res = dst->copy_finish(ar);
    //}
    //catch(const Glib::Error& err)
    //{
    //    res = false;
    //}

    // ar всегда GSimpleAsyncResult, потому любой GFile - для галочки
    GErrorTwin gt;
    res = g_file_copy_finish(dst->gobj(), Glib::unwrap(ar), PP(gt));
    if( !res )
        SafeRemove(dst->get_path());

    ExitJob(ij, gt);
}

static void OnAsyncQueryInfoEnd(RefPtr<Gio::AsyncResult> ar, IOJobPtr ij)
{
    GErrorTwin gt;
    ij->inf = Glib::wrap(g_file_query_info_finish(ij->obj->gobj(), Glib::unwrap(ar), PP(gt)));

    ExitJob(ij, gt);
}

static void OnCancelDownload(IOJobPtr ij, RefPtr<Gio::Cancellable> cancellable)
{
    ij->userAbort = true;
    cancellable->cancel();
    DoExit(ij);
}

static std::string gcharToStr(gchar* buf)
{
    std::string res(buf ? buf : "");
    g_free(buf);
    return res;
}

struct AsyncRunner
{
               IOJobPtr  ij;
ptr::shared<Gtk::Dialog> dlg;               
RefPtr<Gio::Cancellable> cancellable;
       Execution::Pulse  pls;
    
    AsyncRunner(const std::string& addr, IOJobPtr ij_);    
   ~AsyncRunner();    
};

AsyncRunner::AsyncRunner(const std::string& addr, IOJobPtr ij_)
    : ij(ij_)
{
    std::string dlg_title = BF_("Getting \"%1%\"") % addr % bf::stop;
    dlg = MakeDialog(dlg_title.c_str(), 400, -1, 0);

    Gtk::ProgressBar& prg_bar = NewManaged<Gtk::ProgressBar>();
    Gtk::VBox& box = *dlg->get_vbox();
    PackStart(box, prg_bar);

    cancellable = Gio::Cancellable::create();
    dlg->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dlg->signal_response().connect(bb::bind(&OnCancelDownload, ij, cancellable));

    // :TODO: можно отложить показ диалога на N(=5) секунд, если все происходит быстро
    dlg->show_all();

    Execution::ApplyPB(pls, prg_bar);
}

AsyncRunner::~AsyncRunner()
{
    Gtk::Main::run();
}

static IOJobPtr CreateIOJob(RefPtr<Gio::File> obj)
{
    return new IOJob(obj);
}

std::string Uri2LocalPath(const std::string& uri_fname, bool& is_new)
{
    is_new = false;
    //std::string fpath;
    //bool is_local = true;
    //try
    //{
    //    fpath = Glib::filename_from_uri(uri_fname);
    //}
    //catch(const Glib::Error& err)
    //{
    //    is_local = false;
    //}
    
    GErrorTwin gt;
    std::string fpath = gcharToStr(g_filename_from_uri(uri_fname.c_str(), 0, PP(gt)));
    bool is_local = !Ptr(gt);

    if( !is_local )
    {
        std::string basename;
        RefPtr<Gio::File> src = Gio::File::create_for_uri(uri_fname);
        
        bool is_qi_ok = false;
        {
            IOJobPtr ij = CreateIOJob(src);
            {
                AsyncRunner ar(uri_fname, ij);
                src->query_info_async(bb::bind(&OnAsyncQueryInfoEnd, _1, ij), ar.cancellable,
                                      G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
            }
                
            RefPtr<Gio::FileInfo>& inf = ij->inf;
            if( ij->userAbort )
                ;
            else if( inf )
            {    
                basename = inf->get_display_name();
                if( basename.size() )
                    is_qi_ok = true;
                else
                    ErrorBox(boost::format("Empty display_name for \"%1%\"") % uri_fname % bf::stop);
            }
            else
                ErrorBox(BF_("Can't get information about \"%1%\": %2%") % uri_fname % ij->errStr % bf::stop);
            
        }
        
        if( is_qi_ok )
        {
            ASSERT( basename.size() );
            const std::string& cache_dir = GetCacheDir();
            std::string tmp_path;
            for( int i=1; ; i++ )
            {
                fs::path tmp = fs::path(cache_dir) / CloneName(basename, i);
                if( !fs::exists(tmp) )
                {
                    tmp_path = tmp.string();
                    break;
                }
            }
            ASSERT( tmp_path.size() );

            RefPtr<Gio::File> dst = Gio::File::create_for_path(tmp_path);
            IOJobPtr ij = CreateIOJob(dst);
            {
                AsyncRunner ar(uri_fname, ij);
                // :TODO: показывать скорость, кол-во сделанного, куда и т.д.
                src->copy_async(dst, bb::bind(&OnAsyncCopyEnd, _1, ij), ar.cancellable);
            }

            if( ij->userAbort )
                ;
            else if( ij->copyRes )
            {
                std::string content_dir = PreferencesPath("content");
                CreateDirsQuiet(content_dir);
                // ищем подходящий путь для хранения
                for( int i=1; ; i++ )
                {
                    fs::path pth = fs::path(content_dir) / CloneName(basename, i);
                    fpath = pth.string();
                    if( !fs::exists(pth) )
                    {
                        fs::rename(tmp_path, fpath);
                        is_new = true;
                        break;
                    }
                    else
                    {
                        io::stream strm(fpath.c_str());
                        io::stream tmp_strm(tmp_path.c_str());

                        // сравниваем по содержимому
                        if( StreamSize(strm) == StreamSize(tmp_strm) )
                        {
                            char tmp_buf[STRM_BUF_SZ];
                            bool is_break = ReadAllStream(bb::bind(&CompareWithStream, _1, _2, tmp_buf, b::ref(tmp_strm)), strm);
                            if( !is_break )
                            {
                                SafeRemove(tmp_path);
                                break;
                            }
                        }
                    }
                }
            }
            else
                ErrorBox(BF_("Can't get \"%1%\": %2%") % uri_fname % ij->errStr % bf::stop);
        }
    }
    return fpath;
}

