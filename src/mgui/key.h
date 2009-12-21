//
// mgui/key.h
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007-2009 Ilya Murav'jov
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

#ifndef __MGUI_KEY_H__
#define __MGUI_KEY_H__

#include <boost/function.hpp>
#include <map>
#include <list>

// Маска используемых модификаторов
// Так как в некоторых случаях (!) всегда почему-то может быть нажат модификатор
// GDK_MOD2_MASK, то вместо GDK_MODIFIER_MASK приходится делать свою маску
const guint SH_CTRL_ALT_MASK = GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK ;

// с Shift или без, но без Ctrl и Alt
inline bool CanShiftOnly(guint state)
{
    return ( (state|GDK_SHIFT_MASK) & SH_CTRL_ALT_MASK ) == 
                   (GDK_SHIFT_MASK) ;
}

inline bool IsLeftButton(GdkEventButton* event)
{
    return event->button == 1;
}

inline bool IsRightButton(GdkEventButton* event)
{
    return event->button == 3;
}

////////////////////////////////////////////////////////
// CallHotKey()

template<typename HKObject>
struct HK
{
    // из-за того, что Boost.Lambda не может замыкать ссылки на 
    // астрактные типы, пришлось делать через указатели
    typedef boost::function<void(HKObject*, GdkEventKey*)> Functor;

    struct Entry
    {
               guint  keyval;
               guint  modifiers;
             Functor  fnr;

        GdkKeymapKey* keys;
                gint  n_keys;

               Entry(guint k_, guint m_, const Functor& f_)
                   : keyval(k_), modifiers(m_), fnr(f_), keys(0) {}
              ~Entry() { g_free(keys); }
    };

    typedef ptr::shared<Entry> Ptr;
    // множество (hardware_keycode, Entry)
    struct Map: public std::multimap< guint, Ptr>
    {
        // определяем типы для выведения типов в AppendHK()
        typedef HKObject                       Object;
        typedef typename HK<HKObject>::Functor Functor;
    };

    static  Ptr  FindEntry(Map& map, GdkEventKey* event, GdkKeymap* keymap);
    static void  Append(Map& map, guint keyval, guint modifiers, const Functor& fnr,
                        GdkKeymap* keymap);

}; // struct HK

// COPY_N_PASTE_ETALON
// FindEntry() адаптирована из Gtk-функции _gtk_key_hash_lookup()
// как основной алгоритм диспетчеризации горячих клавиш

//GSList *
//_gtk_key_hash_lookup (GtkKeyHash      *key_hash,
//		      guint16          hardware_keycode,
//		      GdkModifierType  state,
//		      GdkModifierType  mask,
//		      gint             group)

template<typename HKObject> typename HK<HKObject>::Ptr
HK<HKObject>::FindEntry(Map& map, GdkEventKey* event, GdkKeymap* keymap)
{
    //GHashTable *keycode_hash = key_hash_get_keycode_hash (key_hash);
    //GSList *keys = g_hash_table_lookup (keycode_hash, GUINT_TO_POINTER ((guint)hardware_keycode));
    //GSList *results = NULL;
    //GSList *l;
    typedef std::list<Ptr> List;
    List results;

    guint16 hardware_keycode = event->hardware_keycode;
    guint state = event->state;
    guint mask  = gtk_accelerator_get_default_mod_mask();
    guint group = event->group;

    bool have_exact = false;
    guint keyval;
    gint effective_group;
    gint level;
    GdkModifierType consumed_modifiers;

    /* We don't want Caps_Lock to affect keybinding lookups.
     */
    state &= ~GDK_LOCK_MASK;

    gdk_keymap_translate_keyboard_state(keymap,
                                        hardware_keycode, (GdkModifierType)state, group,
                                        &keyval, &effective_group, &level, &consumed_modifiers);

    GTK_NOTE (KEYBINDINGS,
              g_message ("Looking up keycode = %u, modifiers = 0x%04x,\n"
                         "    keyval = %u, group = %d, level = %d, consumed_modifiers = 0x%04x",
                         hardware_keycode, state, keyval, effective_group, level, consumed_modifiers));

    typedef typename Map::iterator Itr; 
    std::pair<Itr, Itr> be = map.equal_range(hardware_keycode);
    //if (keys)
    for( Itr itr = be.first; itr != be.second; ++itr )
    {
        //GSList *tmp_list = keys;
        //while (tmp_list)
        //{
        //    tmp_list = tmp_list->next;
        //}

        //GtkKeyHashEntry *entry = tmp_list->data;
        ptr::shared<Entry> entry = itr->second;
        guint xmods, vmods;

        /* If the virtual super, hyper or meta modifiers are present,
         * they will also be mapped to some of the mod2 - mod5 modifiers,
         * so we compare them twice, ignoring either set.
         */
        xmods = GDK_MOD2_MASK|GDK_MOD3_MASK|GDK_MOD4_MASK|GDK_MOD5_MASK;
        vmods = GDK_SUPER_MASK|GDK_HYPER_MASK|GDK_META_MASK;

        if( (entry->modifiers & ~consumed_modifiers & mask & ~vmods) == (state & ~consumed_modifiers & mask & ~vmods) ||
            (entry->modifiers & ~consumed_modifiers & mask & ~xmods) == (state & ~consumed_modifiers & mask & ~xmods) )
        {
            gint i;

            if (keyval == entry->keyval) /* Exact match */
            {
                GTK_NOTE (KEYBINDINGS,
                          g_message ("  found exact match, keyval = %u, modifiers = 0x%04x",
                                     entry->keyval, entry->modifiers));

                if( !have_exact )
                    //g_slist_free (results);
                    //results = NULL;
                    results.clear();

                have_exact = true;
                //results = g_slist_prepend (results, entry);
                results.push_back(entry);
            }

            if (!have_exact)
            {
                for (i = 0; i < entry->n_keys; i++)
                {
                    if (entry->keys[i].keycode == hardware_keycode &&
                        entry->keys[i].level == level) /* Match for all but group */
                    {
                        GTK_NOTE (KEYBINDINGS,
                                  g_message ("  found group = %d, level = %d",
                                             entry->keys[i].group, entry->keys[i].level));

                        //results = g_slist_prepend (results, entry);
                        results.push_back(entry);
                        break;
                    }
                }
            }
        }
    }

    //results = sort_lookup_results (results);
    //for (l = results; l; l = l->next)
    //    l->data = ((GtkKeyHashEntry *)l->data)->value;
    //
    //return results;

    // :TODO: реализовать сортировку как в Gtk
    return results.empty() ? Ptr() : results.front();
}

template<typename HKObject>
void HK<HKObject>::Append(Map& map, guint keyval, guint modifiers, const Functor& fnr,
                              GdkKeymap* keymap)
{
    typedef typename Map::value_type Pair;
    keyval = gdk_keyval_to_lower(keyval);

    GdkKeymapKey* keys;
    gint n_keys;
    if( gdk_keymap_get_entries_for_keyval(keymap, keyval, &keys, &n_keys) )
    {
        ptr::shared<Entry> ent = new Entry(keyval, modifiers, fnr);
        ent->keys   = keys;
        ent->n_keys = n_keys;
        for( int i=0; i<n_keys; i++ )
            map.insert(Pair(keys[i].keycode, ent));
    }
}

// Для подключения механизма обработки горячих клавиш достаточно:
// 1) реализовать функцию FillHotKeyMap() для данного типа объектов,
//  в которой добавляем обработчики с помощью функции AppendHK()
// 2) вызывать по событию 

template<typename HKObject>
void CallHotKey(HKObject& obj, GdkEventKey* event)
{
    GdkKeymap* keymap = gdk_keymap_get_for_display(gdk_drawable_get_display(event->window));

    typedef typename HK<HKObject>::Map Map;
    static Map map; // заполняется один раз
    if( map.empty() )
        FillHotKeyMap(map, keymap);

    typedef typename HK<HKObject>::Ptr Ptr;
    if( Ptr ent = HK<HKObject>::FindEntry(map, event, keymap) )
        ent->fnr(&obj, event);
}

template<typename HKMap>
void AppendHK(HKMap& map, guint keyval, guint modifiers,
              const typename HKMap::Functor& fnr, GdkKeymap* keymap)
{
    typedef typename HKMap::Object HKObject;
    HK<HKObject>::Append(map, keyval, modifiers, fnr, keymap);
}

#endif // __MGUI_KEY_H__

