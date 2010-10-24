
#include <mgui/_pc_.h>

#include "widget.h"

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

