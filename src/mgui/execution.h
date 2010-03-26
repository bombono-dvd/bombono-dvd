#ifndef __MGUI_EXECUTION_H__
#define __MGUI_EXECUTION_H__

#include <mlib/const.h>
#include <mgui/timer.h>

namespace Execution {

struct Data 
{
    GPid  pid;
    bool  userAbort; // пользователь сам отменил

            Data() { Init(); }

      void  Init() 
      {
          pid = NO_HNDL;
          userAbort = false;
      }
      void  StopExecution(const std::string& what);
};

class Pulse
{
    public:
    Pulse(Gtk::ProgressBar& prg_bar);
   ~Pulse();

    protected:
    Timer tm;
};

} // namespace Exection

#endif // #ifndef __MGUI_EXECUTION_H__

