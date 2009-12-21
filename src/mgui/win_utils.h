//
// mgui/win_utils.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2008-2009 Ilya Murav'jov
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

#ifndef __MGUI_WIN_UTILS_H__
#define __MGUI_WIN_UTILS_H__

#include <mlib/string.h>
#include "img_utils.h"

///////////////////////////////////////////////////////////////////////
// Cairo

class CairoStateSave
{
    public:
                CairoStateSave(Cairo::RefPtr<Cairo::Context>& cont): caiCont(cont)
                { caiCont->save(); }
               ~CairoStateSave()
                { caiCont->restore(); }
    private:
        Cairo::RefPtr<Cairo::Context>& caiCont;
};

//
// "Включить" отрисовку всех операций блока с прозрачностью alpha
// (также происходит сохранение контекста)
// 
class CairoAlphaPaint
{
    public:
                CairoAlphaPaint(CR::RefPtr<CR::Context>& cr, double alpha)
                    : caiCont(cr), alphaDgr(alpha)
                { caiCont->push_group(); }

               ~CairoAlphaPaint()
                { 
                    caiCont->pop_group_to_source();
                    caiCont->paint_with_alpha(alphaDgr);
                }
    private:
        CR::RefPtr<CR::Context>& caiCont;
                         double  alphaDgr; // степень прозрачности
};


namespace CR
{

template<typename T>
inline void RectClip(RefPtr<Context>& cr, const RectT<T>& rct)
{
    cr->rectangle(rct.lft, rct.top, rct.Width(), rct.Height());
    cr->clip();
}

// отрисовать картинку в прямоугольник plc (скалирование)
// скалирование в память:
//    CR::RefPtr<CR::ImageSurface> src, dst;
//    CR::RefPtr<CR::Context> cr = Cairo::Context::create(dst);
//    CR::Scale(cr, src, Rect(0, 0, 100, 100));
void Scale(CR::RefPtr<CR::Context> cr, CR::RefPtr<CR::ImageSurface> src,
           const Rect& plc);

struct Color
{
  static const double MinClr; // = 0.0;
  static const double MaxClr; // = 1.0;

    double  r;
    double  g;
    double  b;
    double  a;

            Color(): r(MinClr), g(MinClr), b(MinClr), a(MaxClr) {}
            Color(double red, double green, double blue, double alpha = MaxClr):
                r(red), g(green), b(blue), a(alpha) {}
            Color(const RGBA::Pixel& pxl);
  explicit  Color(const unsigned int rgba);

     Color& FromPixel(const RGBA::Pixel& pxl);
};

Color ShadeColor(const Color& clr, double shade_rat);

void SetColor(RefPtr<Context>& cr, const RGBA::Pixel& pxl);
void SetColor(RefPtr<Context>& cr, const unsigned int rgba);
void SetColor(RefPtr<Context>& cr, const Color& clr);

// установить цвет с яркостью shade
inline void SetColor(RefPtr<Context>& cr, const Color& clr, double shade)
{
    SetColor(cr, ShadeColor(clr, shade));
}

//
// Для точности отрисовки различных объектов с помощью Cairo надо знать
// некоторые хитрости. В частности, для четкого наложения изображений
// ("без размывания") координаты точки приложения должны быть целыми
// в координатах устройства.
// 
inline void AlignCairoCoords(RefPtr<Context> cr, double& x, double& y)
{
    cr->user_to_device(x, y);

    x = round(x); // =Round(x)
    y = round(y);

    cr->device_to_user(x, y);
}

// для точной отрисовки проводим пути по серединам пикселей
const double H_P = 0.5;

inline void HPTranslate(RefPtr<Context> cr)
{
    cr->translate(H_P, H_P);
}

inline DPoint DeviceToUser(RefPtr<Context> cr, const DPoint& phis_pos)
{
    DPoint res(phis_pos);

    cr->device_to_user(res.x, res.y);
    return res;
}

inline DPoint UserToDevice(RefPtr<Context> cr, const DPoint& user_pos)
{
    DPoint res(user_pos);

    cr->user_to_device(res.x, res.y);
    return res;
}

inline DRect DeviceToUser(RefPtr<Context> cr, const DRect& phis_rct)
{
    return DRect(CR::DeviceToUser(cr, phis_rct.A()), CR::DeviceToUser(cr, phis_rct.B()));
}

inline DRect UserToDevice(RefPtr<Context> cr, const DRect& user_rct)
{
    return DRect(CR::UserToDevice(cr, user_rct.A()), CR::UserToDevice(cr, user_rct.B()));
}

} // namespace CR

using CR::H_P;

///////////////////////////////////////////////////////////////////////
// Разное

// :DEPRECATED: - Gtk::manage() ничем не хуже
// этот вспомогательный класс полезен для всех виджет-контейнеров как
// замена хранению экземляров этих самых виджетов в самом объекте
// например, часто в окне создается много вспомогательных виджетов вроде Gtk::[V|H]Box,
// которые, кроме как при сборке, нигде не упоминаются; тогда такие виджеты создаем с
// new, оборачиваем Own() и "забываем" про них.
class WidgetOwner
{
    typedef std::vector<Gtk::Object*>::iterator g_iterator;
    public:

    std::vector<Gtk::Object*> chiArr;
    
       ~WidgetOwner()
        {
            for( g_iterator it = chiArr.begin(), end = chiArr.end(); it != end; ++it )
                delete *it;
        }

        template<typename T>
        T* Own(T* obj)
        {
            if( Gtk::Object* g_obj = obj )
                chiArr.push_back(g_obj);
            return obj;
        }
};

// чтобы события приходили - мышка, изменение размеров, ...
void AddStandardEvents(Gtk::Widget& wdg);


inline RGBA::Pixel::ClrType GColorToCType(gushort val)
{
    return RGBA::Pixel::ClrType(val >> 8);
}

std::string ColorToString(const unsigned int rgba);

#define GetStyleColor_Declare(ClrName)    \
RGBA::Pixel GetStyleColor ## ClrName(Gtk::StateType typ, Gtk::Widget& wdg); \
/**/

//
// RGBA::Pixel GetStyleColor[Fg|Bg|...](Gtk::StateType typ, Gtk::Widget& wdg)
// Получить цвет по виджету и состоянию
// 
GetStyleColor_Declare(Fg)
GetStyleColor_Declare(Bg)
GetStyleColor_Declare(Light)
GetStyleColor_Declare(Dark)
GetStyleColor_Declare(Mid)
GetStyleColor_Declare(Text)
GetStyleColor_Declare(Base)
GetStyleColor_Declare(Text_aa)

CR::Color GetBGColor(Gtk::Widget& wdg);
CR::Color GetBorderColor(Gtk::Widget& wdg);

inline double GetDPI(Gtk::Widget& wdg)
{
    return gdk_screen_get_resolution(wdg.get_screen()->gobj());
}

void SetCursorForWdg(Gtk::Widget& wdg, Gdk::Cursor* curs = 0);

inline void GrabFocus(Gtk::Widget& wdt)
{
    if( !wdt.has_focus() )
        wdt.grab_focus();
}

// сделать кнопку "умолчальной" (граница будет более жирной)
void SetDefaultButton(Gtk::Button& btn);
// создание подсказок - из 
Gtk::Tooltips& TooltipFactory();
void SetTip(Gtk::Widget& wdg, const char* tooltip);
// удобная функция упаковки виджета в рамку
Gtk::Frame& PackWidgetInFrame(Gtk::Widget& wdg, Gtk::ShadowType st,
                              const std::string& label = std::string());

Gtk::Button* CreateButtonWithIcon(const char* label, const Gtk::BuiltinStockID& stock_id,
                                  const char* tooltip = "", Gtk::BuiltinIconSize icon_sz = Gtk::ICON_SIZE_BUTTON);

// чтоб Glib/Gtk инициализовалась + стили проекта
void InitGtkmm(int argc = 0, char** argv = 0);

#endif // __MGUI_WIN_UTILS_H__

