
#include <mgui/_pc_.h>

#include "execution.h"
#include "dialog.h"
#include "gettext.h"

#include <mlib/tech.h>
#include <signal.h> // SIGTERM


namespace Execution
{

void Stop(GPid& pid)
{
    ASSERT( pid != NO_HNDL );
    kill(pid, SIGTERM);
}

void Data::StopExecution(const std::string& what)
{
    // COPY_N_PASTE - тупо сделал содержимое сообщений как у "TSNAMI-MPEG DVD Author"
    // А что делать - нафига свои придумывать, если смысл один и тот же
    if( Gtk::RESPONSE_YES == MessageBox(BF_("You are about to cancel %1%. Are you sure?") % what 
                                            % bf::stop, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO) )
    {
        userAbort = true;
        if( pid != NO_HNDL ) // во время выполнения внешней команды
            Stop(pid);
    }
}

static bool PulseProgress(Gtk::ProgressBar& prg_bar)
{
    prg_bar.pulse();
    return true;
}

Pulse::Pulse(Gtk::ProgressBar& prg_bar) 
{ 
    tm.Connect(bl::bind(&PulseProgress, boost::ref(prg_bar)), 500); 
}

Pulse::~Pulse() 
{
    tm.Disconnect(); 
}

} // namespace Execution

