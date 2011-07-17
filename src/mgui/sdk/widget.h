//
// mgui/sdk/widget.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2009-2010 Ilya Murav'jov
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

#ifndef __MGUI_SDK_WIDGET_H__
#define __MGUI_SDK_WIDGET_H__

#include <mbase/pixel.h>

#include <mlib/function.h>

void SetPercent(Gtk::ProgressBar& bar, double percent, const std::string& info = std::string());

// "прокрутить" очередь сообщений
void IteratePendingEvents();

void SetScaleSecondary(Gtk::HScale& scl);

inline void SetAlign(Gtk::Label& lbl, bool is_left = true)
{
    lbl.set_alignment(is_left ? 0.0 : 1.0, 0.5);
}

typedef boost::function<void(GtkWidget*)> GtkWidgetFunctor;
void ForAllWidgets(GtkWidget* wdg, const GtkWidgetFunctor& fnr);

Gtk::Alignment& NewPaddingAlg(int top, int btm, int lft, int rgt);

Gtk::ScrolledWindow& PackInScrolledWindow(Gtk::Widget& wdg, bool need_hz = false);

// удобная функция упаковки виджета в рамку
Gtk::Frame& NewManagedFrame(Gtk::ShadowType st = Gtk::SHADOW_ETCHED_IN, 
                            const std::string& label = std::string());
Gtk::Frame& PackWidgetInFrame(Gtk::Widget& wdg, Gtk::ShadowType st,
                              const std::string& label = std::string());

Gtk::Label& NewMarkupLabel(const std::string& label, bool use_underline = false);
Gtk::Label& NewBoldLabel(const std::string& label);

std::string BoldItalicText(const std::string& txt);
Gtk::Label& NewBoldItalicLabel(const std::string& label, bool use_underline = false);

//
// GtkRadioButton, GtkRadioMenuItem и др. "radio"-виджеты отрабатывают toggled на оба состояния;
// реально же действие хочется выполнить только на выбор.
//
template<typename RadioItem>
class RadioToggleCaller
{
    public:
                RadioToggleCaller(RadioItem& itm, const ActionFunctor& fnr): itm_(itm), fnr_(fnr) {}

          void  operator()()
          {
              if( itm_.get_active() )
                  fnr_();
          }

    private:
              RadioItem& itm_;
           ActionFunctor fnr_;
};

template<class RadioItem>
void SetForRadioToggle(RadioItem& itm, const ActionFunctor& fnr)
{
    // в заголовках не используем Boost.Lambda
    //itm.signal_toggled().connect(bl::bind(&OnRadioToggled<RadioItem>, boost::ref(itm), fnr));
    itm.signal_toggled().connect(RadioToggleCaller<RadioItem>(itm, fnr));
}

void ConfigureSpin(Gtk::SpinButton& btn, double val, double max, double min = 1.0, int step = 1);

RGBA::Pixel GetColor(const Gtk::ColorButton& btn);
void SetColor(Gtk::ColorButton& btn, const RGBA::Pixel& pxl);
void ConfigureRGBAButton(Gtk::ColorButton& btn, const RGBA::Pixel& pxl);

bool SetEnabled(Gtk::Widget& wdg, bool is_enabled);

Gtk::VBox& PackParaBox(Gtk::VBox& vbox);
Gtk::VBox& PackParaBox(Gtk::VBox& vbox, const char* name);

bool SetFilename(Gtk::FileChooser& fc, const std::string& fpath);

#endif // #ifndef __MGUI_SDK_WIDGET_H__

