
/** 
 *  
 * COPY_N_PASTE_REALIZE from GnomeBaker, http://gnomebaker.sourceforge.net/
 *  
*/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/*
 * File: devices.h
 * Copyright: luke_biddell@yahoo.com
 * Created on: Mon Feb 24 21:51:18 2003
 */

#include "gb_devices.h"

#include <glib/gthread.h>
#include <glib/gmessages.h>
#include <glib/ghash.h>
/* 
#include <glib/gfileutils.h> 
*/ 
#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define GB_LOG_FUNC											\
	if(show_trace) g_print("[%p] [%s] [%d]\n", (gpointer)g_thread_self(), __FILE__, __LINE__);	\

#define GB_TRACE			\
	if(show_trace) g_print	\

gboolean show_trace = FALSE;

static gchar*
get_file_contents(const gchar *file)
{
	GB_LOG_FUNC
	g_return_val_if_fail(file != NULL, NULL);

	gchar *contents = NULL;
	if( !g_file_get_contents(file, &contents, NULL, NULL) )
		g_warning("get_file_contents - Failed to get contents of file [%s]", file);

	return contents;
}

gchar**
gbcommon_get_file_as_list(const gchar *file)
{
    gchar **ret = NULL;
    gchar *contents = get_file_contents(file);
    if( contents )
        ret = g_strsplit(contents, "\n", 0);

	g_free(contents);
	return ret;
}

void
devices_for_each(gpointer key, gpointer value, gpointer user_data)
{
	GB_TRACE("---- key [%s], value [%s]\n", (gchar*)key, (gchar*)value);
	g_free(key);
	g_free(value);
}

void
devices_get_ide_device(const gchar *device_node, const gchar *device_node_path,
					   gchar **model_name, gchar **device_id)
{
	GB_LOG_FUNC
	g_return_if_fail(device_node != NULL);
	g_return_if_fail(model_name != NULL);
	g_return_if_fail(device_id != NULL);
	GB_TRACE("devices_get_ide_device - probing [%s]\n", device_node);
	
	gchar *file = g_strdup_printf("/proc/ide/%s/model", device_node);
    gchar *contents = get_file_contents(file);
	if( contents )
	{
		g_strstrip(contents);
		*model_name = g_strdup(contents);
		*device_id = g_strdup(device_node_path);
		g_free(contents);
	}
	else
		g_warning("devices_get_ide_device - Failed to open %s", file);
	g_free(file);
}

static char* get_sysfs_attr(const gchar* dev, const gchar* attr)
{
	g_return_val_if_fail(dev != NULL, NULL);
    gchar *file = g_strdup_printf("/sys/block/%s/device/%s", dev, attr);
    
    gchar *ret = get_file_contents(file);
    if( ret )
        g_strstrip(ret);
    g_free(file);
    return ret;
}

static char* get_sysfs_modelname(const gchar* dev)
{
	gchar *model_name = NULL;
    gchar* vendor = get_sysfs_attr(dev, "vendor");
    gchar* model  = get_sysfs_attr(dev, "model");
    if( vendor && model )
        model_name = g_strdup_printf("%s %s", vendor, model);

    g_free(vendor);
    g_free(model);
    return model_name;
}

void
devices_get_scsi_device(const gchar *device_node, const gchar *device_node_path,
						gchar **model_name, gchar **device_id)
{
	GB_LOG_FUNC
	g_return_if_fail(device_node != NULL);
	g_return_if_fail(model_name != NULL);
	g_return_if_fail(device_id != NULL);
	GB_TRACE("devices_add_scsi_device - probing [%s]\n", device_node);

	gchar **device_strs = NULL, **devices = NULL;
	if((devices = gbcommon_get_file_as_list("/proc/scsi/sg/devices")) == NULL)
	{
		g_warning("devices_get_scsi_device - Failed to open /proc/scsi/sg/devices");
        *model_name = get_sysfs_modelname(device_node);
	}
	else if((device_strs = gbcommon_get_file_as_list("/proc/scsi/sg/device_strs")) == NULL)
	{
		g_warning("devices_get_scsi_device - Failed to open /proc/scsi/sg/device_strs");
	}
	else
	{
		const gint scsicdromnum = atoi(&device_node[strlen(device_node) - 1]);
		gint cddevice = 0;
		gchar **device = devices;
		gchar **device_str = device_strs;
		while((*device != NULL) && (*device_str) != NULL)
		{
			if((strcmp(*device, "<no active device>") != 0) && (strlen(*device) > 0))
			{
				gint scsihost, scsiid, scsilun, scsitype;
				if(sscanf(*device, "%d\t%*d\t%d\t%d\t%d",
					   &scsihost, &scsiid, &scsilun, &scsitype) != 4)
				{
					g_warning("devices_get_scsi_device - Error reading scsi information from /proc/scsi/sg/devices");
				}
				/* 5 is the magic number according to lib-nautilus-burn */
				else if(scsitype == 5)
				{
					/* is the device the one we are looking for */
					if(cddevice == scsicdromnum)
					{
						gchar vendor[9], model[17];
						if(sscanf(*device_str, "%8c\t%16c", vendor, model) == 2)
						{
							vendor [8] = '\0';
							g_strstrip(vendor);

							model [16] = '\0';
							g_strstrip(model);

							*model_name = g_strdup_printf("%s %s", vendor, model);
							*device_id = g_strdup_printf("%d,%d,%d", scsihost, scsiid, scsilun);
							break;
						}
					}
					++cddevice;
				}
			}
			++device_str;
			++device;
		}
	}

	g_strfreev(devices);
	g_strfreev(device_strs);
}

/*
void
devices_write_device_to_gconf(const gint device_number, const gchar *device_name,
	const gchar *device_id, const gchar *device_node, const gchar *mount_point,
	const gint capabilities)
{
	GB_LOG_FUNC
	gchar *device_name_key = g_strdup_printf(GB_DEVICE_NAME, device_number);
	gchar *device_id_key = g_strdup_printf(GB_DEVICE_ID, device_number);
	gchar *device_node_key = g_strdup_printf(GB_DEVICE_NODE, device_number);
	gchar *device_mount_key = g_strdup_printf(GB_DEVICE_MOUNT, device_number);
	gchar *device_capabilities_key = g_strdup_printf(GB_DEVICE_CAPABILITIES, device_number);

	preferences_set_string(device_name_key, device_name);
	preferences_set_string(device_id_key, device_id);
	preferences_set_string(device_node_key, device_node);
	preferences_set_string(device_mount_key, mount_point);
	preferences_set_int(device_capabilities_key, capabilities);

	g_free(device_name_key);
	g_free(device_id_key);
	g_free(device_node_key);
	g_free(device_mount_key);
	g_free(device_capabilities_key);
	GB_TRACE("devices_write_device_to_gconf - Added [%s] [%s] [%s] [%s]\n",
		  device_name, device_id, device_node, mount_point);
}
*/

/*
void
devices_add_device(const gchar *device_name, const gchar *device_id,
				   const gchar *device_node, const gint capabilities)
{
	GB_LOG_FUNC
	g_return_if_fail(device_name != NULL);
	g_return_if_fail(device_id != NULL);
	g_return_if_fail(device_node != NULL);
    gchar *mount_point = NULL;

	* Look for the device in /etc/fstab *
	gchar* *fstab = gbcommon_get_file_as_list("/etc/fstab");
	gchar **line = fstab;
	while((line != NULL) && (*line != NULL))
	{
		g_strstrip(*line);
		if((*line)[0] != '#') * ignore commented out lines *
		{
			gchar node[64], mount[64];
			if(sscanf(*line, "%s\t%s", node, mount) == 2)
			{
				GB_TRACE("devices_add_device - node [%s] mount [%s]\n", node, mount);
				if(g_ascii_strcasecmp(node, device_node) == 0)
				{
					mount_point = g_strdup(mount);
				}
				else
				{
					* try to resolve the device_node in case it's a
						symlink to the device we are looking for *

                    *
					gchar *link_target = g_new0(gchar, PATH_MAX);
					realpath(node, link_target);
					if(g_ascii_strcasecmp(link_target, device_node) == 0)
					{
						GB_TRACE("devices_add_device - node [%s] is link to [%s]\n", node, link_target);
						mount_point = g_strdup(mount);
					}
                    g_free(link_target); 
                    * 
				}

				if(mount_point != NULL)
					break;
			}
		}
		++line;
	}

	g_strfreev(fstab);

	++device_addition_index;
	devices_write_device_to_gconf(device_addition_index, device_name, device_id,
		  device_node, mount_point, capabilities);
    *
    g_free(mount_point);
    
    printf("device=%s, model=%s\n", device_node, device_name);
}
*/

GHashTable*
devices_get_cdrominfo(gchar **proccdrominfo, gint deviceindex)
{
	GB_LOG_FUNC
	g_return_val_if_fail(proccdrominfo != NULL, NULL);
	g_return_val_if_fail(deviceindex >= 1, NULL);

	GB_TRACE("devices_get_cdrominfo - looking for device [%d]\n", deviceindex);
	GHashTable *ret = NULL;
	gchar **info = proccdrominfo;
	while(*info != NULL)
	{
		g_strstrip(*info);
		if(strlen(*info) > 0)
		{
			if(strstr(*info, "drive name:") != NULL)
				ret = g_hash_table_new(g_str_hash, g_str_equal);

			if(ret != NULL)
			{
				gint columnindex = 0;
				gchar *key = NULL;
				gchar **columns = g_strsplit_set(*info, "\t", 0);
				gchar **column = columns;
				while(*column != NULL)
				{
					g_strstrip(*column);
					if(strlen(*column) > 0)
					{
						if(columnindex == 0)
							key = *column;
						else if(columnindex == deviceindex)
							g_hash_table_insert(ret, g_strdup(key), g_strdup(*column));
						++columnindex;
					}
					++column;
				}
                g_strfreev(columns);

				/* We must check if we found the device index we were
				 looking for */
				if(columnindex <= deviceindex)
				{
					GB_TRACE("devices_get_cdrominfo - Requested device index [%d] is out of bounds. "
						  "All devices have been read.\n", deviceindex);
					g_hash_table_destroy(ret);
					ret = NULL;
					break;
				}
			}
		}
		++info;
	}

	return ret;
}

gint
exec_run_cmd(const gchar *cmd, gchar **output)
{
    /* 
    printf("cmd: '%s'\n", cmd);
    */

    GB_LOG_FUNC
    g_return_val_if_fail(cmd != NULL, -1);

    gchar *std_out = NULL;
    gchar *std_err = NULL;
    gint exit_code = -1;
    GError *error = NULL;
    if(g_spawn_command_line_sync(cmd, &std_out, &std_err, &exit_code, &error))
    {
        *output = g_strconcat(std_out, std_err, NULL);
        g_free(std_out);
        g_free(std_err);
    }
    else if(error != NULL)
    {
        g_warning("exec_run_cmd - error [%s] spawning command [%s]", error->message, cmd);
        g_error_free(error);
    }
    else
    {
        g_warning("exec_run_cmd - Unknown error spawning command [%s]", cmd);
    }
    GB_TRACE("exec_run_cmd - [%s] returned [%d]\n", cmd, exit_code);
    return exit_code;
}

gboolean
devices_parse_cdrecord_output(const gchar *buffer, const gchar *bus_name)
{
	GB_LOG_FUNC
	g_return_val_if_fail(buffer != NULL, FALSE);
	g_return_val_if_fail(bus_name != NULL, FALSE);
	gboolean ok = TRUE;

	gchar **lines = g_strsplit(buffer, "\n", 0);
	gchar **line = lines;
	while(*line != NULL)
	{
		/*	Ignore stuff like my camera which cdrecord detects...
		'OLYMPUS ' 'D-230           ' '1.00' Removable Disk */
		if(strstr(*line, "Removable Disk") == NULL)
		{
			gchar vendor[9], model[17], device_id[6];

			if(sscanf(*line, "\t%5c\t  %*d) '%8c' '%16c'", device_id, vendor, model) == 3)
			{
				vendor[8] = '\0';
				model[16] = '\0';
				device_id[5] = '\0';

				gchar *device = NULL;

				/* Copy the bus id stuff ie 0,0,0 to the device struct.
				   If the bus is NULL it's SCSI  */
				if(g_ascii_strncasecmp(bus_name, "SCSI", 4) == 0)
					device = g_strdup(device_id);
				else if(g_ascii_strncasecmp(bus_name, "/dev", 4) == 0)
					device = g_strdup(bus_name);
				else
					device = g_strconcat(bus_name, ":", device_id, NULL);

				gchar *displayname = g_strdup_printf("%s %s", g_strstrip(vendor),
					   g_strstrip(model));

				devices_add_device(displayname, device, "", 0);

				g_free(displayname);
				g_free(device);
			}
		}
		++line;
	}

	g_strfreev (lines);

	return ok;
}

gboolean
devices_probe_bus(const gchar *bus)
{
	GB_LOG_FUNC
	gboolean ok = FALSE;
	g_return_val_if_fail(bus != NULL, FALSE);

	gchar command[32] = "cdrecord -scanbus";
	if(g_ascii_strncasecmp(bus, "SCSI", 4) != 0)
	{
		strcat(command, " dev=");
		strcat(command, bus);
	}

	gchar *buffer = NULL;
    exec_run_cmd(command, &buffer);
	if(buffer == NULL)
		g_warning("devices_probe_bus - Failed to scan the scsi bus");
	else if(!devices_parse_cdrecord_output(buffer, bus))
		g_warning("devices_probe_bus - failed to parse cdrecord output");
	else
		ok = TRUE;
	g_free(buffer);

	return ok;
}

void
devices_probe_busses()
{
	GB_LOG_FUNC

	devices_clear_devicedata();

#ifdef __linux__
/* #if 0 */

	gchar **info = NULL;
	if((info = gbcommon_get_file_as_list("/proc/sys/dev/cdrom/info")) == NULL)
	{
		g_warning("devices_probe_busses - Failed to open /proc/sys/dev/cdrom/info");
	}
	else
	{
		gint devicenum = 1;
		GHashTable *devinfo = NULL;
		while((devinfo = devices_get_cdrominfo(info, devicenum)) != NULL)
		{
			const gchar *device = (const gchar*)g_hash_table_lookup(devinfo, "drive name:");
			gchar *device_node_path = g_strdup_printf("/dev/%s", device);

			gchar *model_name = NULL, *device_id = NULL;

			if(device[0] == 'h')
				devices_get_ide_device(device, device_node_path, &model_name, &device_id);
			else
				devices_get_scsi_device(device, device_node_path, &model_name, &device_id);

			gint capabilities = 0;
			if(g_ascii_strcasecmp((const gchar*)g_hash_table_lookup(devinfo, "Can write CD-R:"), "1") == 0)
				capabilities |= DC_WRITE_CDR;
			if(g_ascii_strcasecmp((const gchar*)g_hash_table_lookup(devinfo, "Can write CD-RW:"), "1") == 0)
				capabilities |= DC_WRITE_CDRW;
			if(g_ascii_strcasecmp((const gchar*)g_hash_table_lookup(devinfo, "Can write DVD-R:"), "1") == 0)
				capabilities |= DC_WRITE_DVDR;
			if(g_ascii_strcasecmp((const gchar*)g_hash_table_lookup(devinfo, "Can write DVD-RAM:"), "1") == 0)
				capabilities |= DC_WRITE_DVDRAM;

			devices_add_device(model_name, device_id, device_node_path, capabilities);

			g_free(model_name);
			g_free(device_id);
			g_free(device_node_path);
			g_hash_table_foreach(devinfo, devices_for_each, NULL);
			g_hash_table_destroy(devinfo);
			devinfo = NULL;
			++devicenum;
		}
	}

	g_strfreev(info);

#else

	devices_probe_bus("SCSI");
	devices_probe_bus("ATAPI");
	devices_probe_bus("ATA");

#endif

}

