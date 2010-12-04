//
// mbase/project/table.h
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

#ifndef __MBASE_PROJECT_TABLE_H__
#define __MBASE_PROJECT_TABLE_H__

#include "media.h"
#include "menu.h"

void CheckObjectLinksEmpty();

namespace Project
{

// массив, позволяющий работать в 2 режимах:
// - вначале набираем
// - затем (если нужно), индексируем; вернуться в нормальный режим можно
//   только все удалив,- Clear()
template<class Item>
class Table: protected std::vector<Item>
{
    typedef std::vector<Item> MyParent;
    public:

        typedef typename MyParent::iterator    Itr;
        typedef typename MyParent::value_type  Val;
        typedef typename MyParent::reference   Ref;
        typedef typename MyParent::size_type   size_type;

              Table(): isIndexed(false) {}

              // вставка медиа 
        void  Insert(const Item& mi) { this->push_back(mi); }
              // удаление по одному не требуется
         //Itr  Erase(Itr itr) { return erase(itr); }

         Ref  operator[](size_type n) 
              { return MyParent::operator[](n); }
         Itr  Beg() { return MyParent::begin(); }
         Itr  End() { return MyParent::end(); }

        void  Clear() 
              { 
                  MyParent::clear();
                  idxLst.clear();
                  isIndexed = false;  
              }
         int  Size() { return MyParent::size(); }

              // перед использованием поиска необходимо провести индексацию
        void  Index()
              {
                  if( isIndexed )
                      return;
                  isIndexed = true;

                  int i = 0;
                  for( Itr itr = Beg(), end = End(); itr != end; ++itr, ++i )
                      idxLst.push_back(std::make_pair(*itr, i));
                  std::sort(idxLst.begin(), idxLst.end(), IdxLess);
              }

         int  Find(Item mi)
              {
                  ASSERT( isIndexed );
                  ASSERT( Size() == (int)idxLst.size() );
                  int idx = NO_HNDL;
                  typedef typename IndexList::iterator idx_iter;

                  idx_iter beg = idxLst.begin(), end = idxLst.end();
                  idx_iter itr = std::lower_bound(beg, end, IdxElem(mi, NO_HNDL), IdxLess);
                  if( (itr != end) && (mi == itr->first) )
                      idx = itr->second;
                  return idx;
              }
    protected:

        typedef typename std::pair<Item, int> IdxElem;
        typedef typename std::vector<IdxElem> IndexList;

                    IndexList  idxLst; // для получения позиции медиа в списке
                         bool  isIndexed;

static bool IdxLess(const IdxElem& e1, const IdxElem& e2)
{
    return e1.first < e2.first;
}

};

typedef Table<MediaItem> MediaList;
typedef Table<Menu>      MenuList;

class ADatabase: public Singleton<ADatabase>, public DataWare
{
    public:
                       ADatabase();

                 void  SetPalTvSystem(bool is_pal);
                       // глобальные параметры
                 bool& PalTvSystem() { return isPAL; }
           MenuParams& GetDefMP() { return defPrms; }
            MediaItem& FirstPlayItem() { return firstPlayItm; }
                       // список медиа
            MediaList& GetML() { return mdList; }
                       // список меню
             MenuList& GetMN() { return mnList; }

                 void  Clear(bool is_full = true);
                 void  ClearSettings();

                 void  Load(const std::string& fname,
                            const std::string& cur_dir = std::string()) throw (std::exception);
                 bool  Save();

                 bool  SaveAs(const std::string& fname,
                              const std::string& cur_dir = std::string());

                 void  SetProjectFName(const std::string& fname, 
                                       const std::string& cur_dir = std::string());
    const std::string& GetProjectFName() const { return prjFName; }
                 bool  IsProjectSet() const { return !prjFName.empty(); }

                       // загрузить/сохранить с afnr в качестве движка 
                 void  LoadWithFnr(const std::string& fname, ArchieveFnr afnr,
                                   const std::string& cur_dir = std::string());
                 bool  SaveWithFnr(ArchieveFnr afnr);

                       // непосредственно после загрузки и перед сохранением
                       // база находится в сложенном виде, т.е. списки медиа (mdList)
                       // и меню (mnList) заполнены - так удобнее сериализовать;
                       // однако при работе с ними управление ими передается соответ.
                       // браузерам - так удобнее непосредственно изменять данные;
                       // Особенности:
                       // - удаление с помощью DeleteMedia() реализуется их владельцем =>,
                       //   а сложенном виде вообще не нужно (ничего не делает).
                       // - в пределах mbase/project использование не нужно, и только Clear(true)
                       //   меняет это состояние
                 bool  OutState() { return isOut; }
                 void  SetOut(bool is_out)
                       {
                           isOut = is_out;
                           if( isOut )
                               ClearOrderArrs();
                       }

    protected:

             bool  isPAL;   // PAL vs NTSC
       MenuParams  defPrms; // размеры по умолчанию
        MediaItem  firstPlayItm; // First Play Media (FP-элемент)

        MediaList  mdList;
         MenuList  mnList;
      std::string  prjFName; // абсолютный путь до файла проекта

             bool  isOut;

void  ClearOrderArrs()
{
    mdList.Clear();
    mnList.Clear();
}

void  IndexOrderArrs()
{
    mdList.Index();
    mnList.Index();
}

};

inline ADatabase& AData()
{
    return ADatabase::Instance();
}

class DBCleanup
{
    public:
            DBCleanup(bool clear_before = true) 
            {
                CheckObjectLinksEmpty();
                if( clear_before ) 
                    AData().Clear(); 
            }
           ~DBCleanup()
            { AData().Clear(); }
};

// возвращаемые пути (readdir(), GtkFileChooser) надо переводить в utf8
std::string ConvertPathToUtf8(const std::string& path);
std::string ConvertPathFromUtf8(const std::string& path);

Menu MakeMenu(const std::string& name);
Menu MakeMenu(int old_sz); // авто-имя

void SaveFormattedUTF8Xml(xmlpp::Document& doc, const Glib::ustring& filename);

} // namespace Project

#endif // #ifndef __MBASE_PROJECT_TABLE_H__

