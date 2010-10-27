
#include <mgui/_pc_.h>

#include "widget.h"
#include <mgui/img_utils.h>

void SetScaleSecondary(Gtk::HScale& scl)
{
    scl.property_draw_value() = false;
    scl.property_can_focus()  = false;
}

void ConfigureSpin(Gtk::SpinButton& btn, double val, double max)
{
    // по мотивам gtk_spin_button_new_with_range()
    int step = 1;
    btn.configure(*Gtk::manage(new Gtk::Adjustment(val, 1, max, step, 10*step, 0)), step, 0);
    btn.set_numeric(true);
}

RGBA::Pixel GetColor(const Gtk::ColorButton& btn)
{
    RGBA::Pixel pxl(RGBA::ColorToPixel(btn.get_color()));
    pxl.alpha = RGBA::FromGdkComponent(btn.get_alpha());
    return pxl;
}

void SetColor(Gtk::ColorButton& btn, const RGBA::Pixel& pxl)
{
    btn.set_color(RGBA::PixelToColor(pxl));
    btn.set_alpha(RGBA::ToGdkComponent(pxl.alpha));
}

void ConfigureRGBAButton(Gtk::ColorButton& btn, const RGBA::Pixel& pxl)
{
    btn.set_use_alpha();
    SetColor(btn, pxl);
}

