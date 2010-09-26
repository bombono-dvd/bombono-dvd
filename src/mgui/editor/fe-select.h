#ifndef __MGUI_EDITOR_FE_SELECT_H__
#define __MGUI_EDITOR_FE_SELECT_H__

#include <mgui/menu-rgn.h>

#include <mlib/range/any_range.h>
#include <mlib/foreach.h>

//
// Iterating over selected objects; example:
//  boost_foreach( Comp::MediaObj* obj, SelectedMediaObjs(edt_area) )
//  {
//      res_mi = obj->MediaItem();
//      break;
//  }
//
fe::range<Comp::MediaObj*> SelectedMediaObjs();


#endif // __MGUI_EDITOR_FE_SELECT_H__

