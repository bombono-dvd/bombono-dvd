#ifndef __MGUI_PROJECT_SERIALIZE_H__
#define __MGUI_PROJECT_SERIALIZE_H__

#include <mgui/init.h>

namespace Project
{

AStores& InitAStores();
void UpdateDVDSize();

class ConstructorApp;
bool CheckBeforeClosing(ConstructorApp& app);

void SetAppTitle(bool clear_change_flag = true);
void LoadApp(const std::string& fname);

void AddSrlActions(RefPtr<Gtk::ActionGroup> prj_actions);

} // namespace Project


#endif // #ifndef __MGUI_PROJECT_SERIALIZE_H__

