#ifndef __MGUI_EDITOR_FE_SELECT_H__
#define __MGUI_EDITOR_FE_SELECT_H__

#include <mgui/menu-rgn.h>

#include <mlib/range/any_range.h>
#include <mlib/foreach.h>

fe::range<Comp::MediaObj*> SelectedMediaObjs(MenuRegion& mn_rgn, const int_array& sel_arr);


#endif // __MGUI_EDITOR_FE_SELECT_H__

