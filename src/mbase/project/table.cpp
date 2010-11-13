//
// mbase/project/table.cpp
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

#include <mbase/_pc_.h>

#include "table.h"
#include "archieve.h"
#include "archieve-sdk.h"
#include "srl-common.h"
#include "handler.h"
#include "theme.h"

#include <mbase/resources.h>


const char* APROJECT_VERSION = "0.8.1";

namespace Project
{

void Media::Serialize(Archieve& ar)
{
    ar & NameValue("Name", mdName);
    SerializeImpl(ar);
}

// struct MDAddress_
// {
//     int pos;
//         MDAddress_(): pos(NO_HNDL) {}
// };

// void MediaList::Insert(const MediaItem& mi)
// {
//     push_back(mi);
//     mi->GetData<MDAddress_>().pos = size()-1;
// }

VideoMD::Itr ChapterPos(ChapterItem chp)
{
    VideoMD::ListType& chapters = GetList(chp);
    // :WISH: find перебирает весь массив, и при небольшом кол-ве все хорошо
    // но, по-правильному, хочется более эффективный алгоритм
    VideoMD::Itr itr = std::find(chapters.begin(), chapters.end(), chp);
    ASSERT( itr != chapters.end() );

    return itr;
}

static void DeleteChapterImpl(VideoMD::ListType& chp_lst, VideoMD::Itr itr)
{
    chp_lst.erase(itr);
}

void DeleteChapter(VideoMD::ListType& chp_lst, VideoMD::Itr itr)
{
    InvokeOnDelete(*itr);
    DeleteChapterImpl(chp_lst, itr);
}

// за удаление "браузерных объектов" ответственны их владельцы
class DeleterVis: public ObjVisitor
{
    public:

        //virtual  void  Visit(StillImageMD& obj) { DeleteFromML(obj); }
        //virtual  void  Visit(VideoMD& obj)      { DeleteFromML(obj); }
        virtual  void  Visit(VideoChapterMD& obj);
        //virtual  void  Visit(MenuMD& obj);

    protected:

        //void DeleteFromML(Media& md);
};

void DeleterVis::Visit(VideoChapterMD& obj)
{
    ChapterItem chp(&obj);
    DeleteChapterImpl(GetList(chp), ChapterPos(chp));
}

// void DeleterVis::Visit(MenuMD& obj)
// {
//     MenuList& md_list = AData().GetMN();
//     MenuList::Itr itr = md_list.find(&obj);
//     ASSERT( itr != md_list.end() );
//
//     md_list.erase(itr);
// }

// void DeleterVis::DeleteFromML(Media& md)
// {
//     // :TODO: получается, что от AData требуется только
//     // хранение данных, без всякой структуры => лучше всего
//     // подойдет контейнер std::set<MediaItem>
//     // (как MenuList; тогда не надо хранить никаких
//     // MDAddress_ для удаления элементов)
//     MediaList& md_list = AData().GetML();
//     int pos = md.GetData<MDAddress_>().pos;
//     ASSERT( pos != NO_HNDL );
//     ASSERT( pos < (int)md_list.Size() );
//
//     MediaList::Itr itr = md_list.Beg() + pos;
//     md_list.Erase(itr);
//     // переиндексируем за ним
//     for( MediaList::Itr end = md_list.End(); itr != end; ++itr )
//         (*itr)->GetData<MDAddress_>().pos--;
// }

void DeleteMedia(MediaItem mi)
{
    InvokeOnDelete(mi);

    DeleterVis vis;
    mi->Accept(vis);
}

NameValueT<Media> LoadMedia(Archieve& ar, MediaList& md_list)
{
    MediaItem md;

    std::string type = GetValue(ar, "Type");
    if( type == "StillPicture" )
        md = new StillImageMD;
    else if( type == "Video" )
        md = new VideoMD;

    if( !md )
        throw std::runtime_error("Unknown Media type: " + type);
    md_list.Insert(md);

    return NameValue("Media", *md);
}

// либо пусто, либо полный путь
void SerializePath(Archieve& ar, const char* tag_name, std::string& fpath)
{
    //ar & NameValue("Path", mdPath);
    fs::path rel_to_dir = fs::path(AData().GetProjectFName()).branch_path();
    if( ar.IsLoad() )
    {
        ar >> NameValue(tag_name, fpath);

        if( !fpath.empty() )
        {
            fs::path pth(fpath);
            if( !pth.is_complete() )
                fpath = (rel_to_dir/fpath).string();
        }
    }
    else // IsSave
    {
        std::string res;
        if( !fpath.empty() )
        {
    
            fs::path pth(fpath);
            MakeRelativeToDir(pth, rel_to_dir);
            res = pth.string();
        }

        ar << NameValue(tag_name, res);
    }
}

void StorageMD::SerializeImpl(Archieve& ar)
{
    ////ar & NameValue("Path", mdPath);
    //fs::path rel_to_dir = fs::path(AData().GetProjectFName()).branch_path();
    //if( ar.IsLoad() )
    //{
    //    ar >> NameValue("Path", mdPath);
    //
    //    fs::path pth(mdPath);
    //    if( !pth.is_complete() )
    //        mdPath = (rel_to_dir/mdPath).string();
    //}
    //else // IsSave
    //{
    //    fs::path pth(mdPath);
    //    MakeRelativeToDir(pth, rel_to_dir);
    //
    //    ar << NameValue("Path", pth.string());
    //}
    if( ar.IsSave() )
        ASSERT( !mdPath.empty() );
    SerializePath(ar, "Path", mdPath);
}

void StillImageMD::SerializeImpl(Archieve& ar)
{
    MyParent::SerializeImpl(ar);
    // :TODO: выделить для всех "файлов", в StorageMD::SerializeImpl: 
    //  ar << NameValue("Type", TypeString());
    if( ar.IsSave() )
        ar << NameValue("Type", "StillPicture");
}

NameValueT<VideoChapterMD> LoadChapter(VideoMD* vd)
{
    ChapterItem ci = VideoChapterMD::CreateChapter(vd, 0);
    return NameValue("Part", *ci);
}

void Serialize(Archieve& ar, PostAction& pa)
{
    ar("Type", pa.paTyp);
    SerializeReference(ar, "Ref", pa.paLink);
}

void SerializePostAction(Archieve& ar, PostAction& pa)
{
    ar("PostAction", pa);
}

void VideoMD::SerializeImpl(Archieve& ar)
{
    MyParent::SerializeImpl(ar);
    if( ar.IsSave() )
        ar << NameValue("Type", "Video");

    // * главы
    {
        ArchieveStackFrame asf(ar, "Parts");
        if( ar.IsLoad() )
        {
            ArchieveFunctor<VideoChapterMD> fnr =
                MakeArchieveFunctor<VideoChapterMD>( bb::bind(&LoadChapter, this) );
            LoadArray(ar, fnr);
        }
        else // IsSave
        {
            for( Itr itr = chpLst.begin(), end = chpLst.end(); itr != end; ++itr )
                ar << NameValue("Part", **itr);
        }
    }

    SerializePostAction(ar, pAct);
}

void VideoChapterMD::SerializeImpl(Archieve& ar)
{
    // :TODO: записывать в формате чч:мм:сс:кк
    ar & NameValue("", chpTime);
}

void Save(Archieve& ar, MediaList& md_list)
{
    for( MediaList::Itr itr = md_list.Beg(), end = md_list.End(); itr != end; ++itr )
        ar << NameValue("Media", **itr);
}

void Load(Archieve& ar, MediaList& md_list)
{
    ArchieveFunctor<Media> fnr =
        MakeArchieveFunctor<Media>( bb::bind(&LoadMedia, boost::ref(ar), boost::ref(md_list)) );
    LoadArray(ar, fnr);
}

ADatabase::ADatabase(): isPAL(true), defPrms(isPAL), isOut(false)
{}

void ADatabase::SetPalTvSystem(bool is_pal)
{
    isPAL = is_pal;
    defPrms = MenuParams(is_pal);
}

void ADatabase::Clear(bool is_full) 
{
   ClearOrderArrs();

   if( is_full )
   {
       ClearSettings();
       DataWare::Clear();
       SetOut(false);
   }
}

void ADatabase::ClearSettings()
{
    firstPlayItm = MediaItem();
    SetProjectFName(std::string());  // пусто
}

void ADatabase::SetProjectFName(const std::string& fname, const std::string& cur_dir)
{
    if( fname.empty() )
        prjFName = fname; // файл проекта не установлен
    else
    {
        fs::path abs_path = MakeAbsolutePath(fname, cur_dir);
        prjFName = abs_path.string();
    }
}

bool ADatabase::SaveWithFnr(ArchieveFnr afnr)
{
    ASSERT( !isOut );
    if( !IsProjectSet() )
        return false;
    IndexOrderArrs();

    xmlpp::Document doc;
    xmlpp::Element* root_node = doc.create_root_node("AProject");
    root_node->set_attribute("Version", APROJECT_VERSION);
    root_node->add_child_comment("This document is for " APROGRAM_PRINTABLE_NAME " program");

    DoSaveArchieve(root_node, afnr);
    doc.write_to_file_formatted(ConvertPathFromUtf8(prjFName));
    return true;
}

void ADatabase::LoadWithFnr(const std::string& fname, ArchieveFnr afnr,
                            const std::string& cur_dir)
{
    Clear(false);
    SetProjectFName(fname, cur_dir);
    if( !IsProjectSet() ) // пустой проект
        return;

    DoLoadArchieve(ConvertPathFromUtf8(prjFName), afnr, "AProject");
}

//
// ThemeDirList
// 

ThemeDirList::ThemeDirList()
{
    // директории по умолчанию
    std::string user_frames = GetConfigDir() + "/frames";
    if( CreateDirsQuiet(user_frames) )
        AddDir(user_frames);

    AddDir(GetDataDir() + "/frames");
}

static bool IsFrameDir(const fs::path& f_dir)
{
    return fs::is_directory(f_dir) && fs::exists(f_dir/"frame.png");
}

fs::path FindThemePath(const std::string& theme_name)
{
    ThemeDirList& lst = ThemeDirList::Instance();
    for( ThemeDirList::iterator itr = lst.begin(), end = lst.end(); itr != end; ++itr )
    {
        fs::path pth = fs::path(*itr) / theme_name;
        if( fs::exists(pth) && IsFrameDir(pth) )
            return pth;
    }
    throw std::runtime_error("Cant find theme " + theme_name);
    return fs::path();
}

void GetThemeList(Str::List& t_lst)
{
    t_lst.clear();
    ThemeDirList& lst = ThemeDirList::Instance();
    for( ThemeDirList::iterator itr = lst.begin(), end = lst.end(); itr != end; ++itr )
    {
        fs::path dir = fs::path(*itr);
        if( fs::is_directory(dir) )
        {
            for( fs::directory_iterator itr(dir), end; itr != end; ++itr )
                if( IsFrameDir(*itr) )
                    t_lst.push_back(itr->leaf());
        }
    }

    // * сортируем и удаляем дублирование
    std::sort(t_lst.begin(), t_lst.end());
    t_lst.resize(std::unique(t_lst.begin(), t_lst.end()) - t_lst.begin());
}


} // namespace Project


