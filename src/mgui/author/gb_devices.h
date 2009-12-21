
#ifndef __MGUI_AUTHOR_GB_DEVICES_H__
#define __MGUI_AUTHOR_GB_DEVICES_H__

#include <glib.h>

G_BEGIN_DECLS

/* Capabilities of devices */
static const gint DC_WRITE_CDR = 0x1;
static const gint DC_WRITE_CDRW = 0x2;
static const gint DC_WRITE_DVDR = 0x4;
static const gint DC_WRITE_DVDRAM = 0x8;

void devices_probe_busses();
void devices_clear_devicedata();
/** 
 * device_name - человеческое имя привода
 * device_id   - путь привода в стиле
 *   cdrecord-программ (для scsi-устройств
 *   отличается от device_node)
 * device_node - путь привода, в том числе
 *   для growisofs
 * capabilities - флаги DC_XXX
*/
void devices_add_device(const gchar *device_name, const gchar *device_id,
                        const gchar *device_node, const gint capabilities);


G_END_DECLS

#endif /* __MGUI_AUTHOR_OUTPUT_H__ */

