/*
 * Copyright (c) 2004-2007 Voltaire, Inc. All rights reserved.
 * Copyright (c) 2002-2006 Mellanox Technologies LTD. All rights reserved.
 * Copyright (c) 1996-2003 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * Abstract:
 *    Various OpenSM dumpers
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif				/* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iba/ib_types.h>
#include <complib/cl_qmap.h>
#include <complib/cl_debug.h>
#include <opensm/osm_opensm.h>
#include <opensm/osm_log.h>
#include <opensm/osm_node.h>
#include <opensm/osm_switch.h>
#include <opensm/osm_helper.h>
#include <opensm/osm_msgdef.h>
#include <opensm/osm_opensm.h>

struct dump_context {
	osm_opensm_t *p_osm;
	FILE *file;
};

static void dump_ucast_path_distribution(cl_map_item_t * p_map_item, void *cxt)
{
	osm_node_t *p_node;
	osm_node_t *p_remote_node;
	uint8_t i;
	uint8_t num_ports;
	uint32_t num_paths;
	ib_net64_t remote_guid_ho;
	osm_switch_t *p_sw = (osm_switch_t *) p_map_item;
	osm_opensm_t *p_osm = ((struct dump_context *)cxt)->p_osm;

	p_node = p_sw->p_node;
	num_ports = p_sw->num_ports;

	osm_log_printf(&p_osm->log, OSM_LOG_DEBUG,
		       "dump_ucast_path_distribution: "
		       "Switch 0x%" PRIx64 "\n"
		       "Port : Path Count Through Port",
		       cl_ntoh64(osm_node_get_node_guid(p_node)));

	for (i = 0; i < num_ports; i++) {
		num_paths = osm_switch_path_count_get(p_sw, i);
		osm_log_printf(&p_osm->log, OSM_LOG_DEBUG, "\n %03u : %u", i,
			       num_paths);
		if (i == 0) {
			osm_log_printf(&p_osm->log, OSM_LOG_DEBUG,
				       " (switch management port)");
			continue;
		}

		p_remote_node = osm_node_get_remote_node(p_node, i, NULL);
		if (p_remote_node == NULL)
			continue;

		remote_guid_ho =
		    cl_ntoh64(osm_node_get_node_guid(p_remote_node));

		switch (osm_node_get_remote_type(p_node, i)) {
		case IB_NODE_TYPE_SWITCH:
			osm_log_printf(&p_osm->log, OSM_LOG_DEBUG,
				       " (link to switch");
			break;
		case IB_NODE_TYPE_ROUTER:
			osm_log_printf(&p_osm->log, OSM_LOG_DEBUG,
				       " (link to router");
			break;
		case IB_NODE_TYPE_CA:
			osm_log_printf(&p_osm->log, OSM_LOG_DEBUG,
				       " (link to CA");
			break;
		default:
			osm_log_printf(&p_osm->log, OSM_LOG_DEBUG,
				       " (link to unknown node type");
			break;
		}

		osm_log_printf(&p_osm->log, OSM_LOG_DEBUG, " 0x%" PRIx64 ")",
			       remote_guid_ho);
	}

	osm_log_printf(&p_osm->log, OSM_LOG_DEBUG, "\n");
}

static void dump_ucast_routes(cl_map_item_t * p_map_item, void *cxt)
{
	const osm_node_t *p_node;
	osm_port_t *p_port;
	uint8_t port_num;
	uint8_t num_hops;
	uint8_t best_hops;
	uint8_t best_port;
	uint16_t max_lid_ho;
	uint16_t lid_ho, base_lid;
	boolean_t direct_route_exists = FALSE;
	osm_switch_t *p_sw = (osm_switch_t *) p_map_item;
	osm_opensm_t *p_osm = ((struct dump_context *)cxt)->p_osm;
	FILE *file = ((struct dump_context *)cxt)->file;

	p_node = p_sw->p_node;

	max_lid_ho = p_sw->max_lid_ho;

	fprintf(file, "__osm_ucast_mgr_dump_ucast_routes: "
		"Switch 0x%016" PRIx64 "\n"
		"LID    : Port : Hops : Optimal\n",
		cl_ntoh64(osm_node_get_node_guid(p_node)));
	for (lid_ho = 1; lid_ho <= max_lid_ho; lid_ho++) {
		fprintf(file, "0x%04X : ", lid_ho);

		p_port = cl_ptr_vector_get(&p_osm->subn.port_lid_tbl, lid_ho);
		if (!p_port) {
			fprintf(file, "UNREACHABLE\n");
			continue;
		}

		port_num = osm_switch_get_port_by_lid(p_sw, lid_ho);
		if (port_num == OSM_NO_PATH) {
			/*
			   This may occur if there are 'holes' in the existing
			   LID assignments.  Running SM with --reassign_lids
			   will reassign and compress the LID range.  The
			   subnet should work fine either way.
			 */
			fprintf(file, "UNREACHABLE\n");
			continue;
		}
		/*
		   Switches can lie about which port routes a given
		   lid due to a recent reconfiguration of the subnet.
		   Therefore, ensure that the hop count is better than
		   OSM_NO_PATH.
		 */
		if (p_port->p_node->sw) {
			/* Target LID is switch.
			   Get its base lid and check hop count for this base LID only. */
			base_lid = osm_node_get_base_lid(p_port->p_node, 0);
			base_lid = cl_ntoh16(base_lid);
			num_hops =
			    osm_switch_get_hop_count(p_sw, base_lid, port_num);
		} else {
			/* Target LID is not switch (CA or router).
			   Check if we have route to this target from current switch. */
			num_hops =
			    osm_switch_get_hop_count(p_sw, lid_ho, port_num);
			if (num_hops != OSM_NO_PATH) {
				direct_route_exists = TRUE;
				base_lid = lid_ho;
			} else {
				osm_physp_t *p_physp = p_port->p_physp;

				if (!p_physp || !p_physp->p_remote_physp ||
				    !p_physp->p_remote_physp->p_node->sw)
					num_hops = OSM_NO_PATH;
				else {
					base_lid =
					    osm_node_get_base_lid(p_physp->
								  p_remote_physp->
								  p_node, 0);
					base_lid = cl_ntoh16(base_lid);
					num_hops =
					    p_physp->p_remote_physp->p_node->
					    sw ==
					    p_sw ? 0 :
					    osm_switch_get_hop_count(p_sw,
								     base_lid,
								     port_num);
				}
			}
		}

		if (num_hops == OSM_NO_PATH) {
			fprintf(file, "UNREACHABLE\n");
			continue;
		}

		best_hops = osm_switch_get_least_hops(p_sw, base_lid);
		if (!p_port->p_node->sw && !direct_route_exists) {
			best_hops++;
			num_hops++;
		}

		fprintf(file, "%03u  : %02u   : ", port_num, num_hops);

		if (best_hops == num_hops)
			fprintf(file, "yes");
		else {
			best_port = osm_switch_recommend_path(p_sw, p_port, lid_ho, TRUE, NULL, NULL, NULL, NULL);	/* No LMC Optimization */
			fprintf(file, "No %u hop path possible via port %u!",
				best_hops, best_port);
		}

		fprintf(file, "\n");
	}
}

static void dump_mcast_routes(cl_map_item_t * p_map_item, void *cxt)
{
	osm_switch_t *p_sw = (osm_switch_t *) p_map_item;
	FILE *file = ((struct dump_context *)cxt)->file;
	osm_mcast_tbl_t *p_tbl;
	int16_t mlid_ho = 0;
	int16_t mlid_start_ho;
	uint8_t position = 0;
	int16_t block_num = 0;
	boolean_t first_mlid;
	boolean_t first_port;
	const osm_node_t *p_node;
	uint16_t i, j;
	uint16_t mask_entry;
	char sw_hdr[256];
	char mlid_hdr[32];

	p_node = p_sw->p_node;

	p_tbl = osm_switch_get_mcast_tbl_ptr(p_sw);

	sprintf(sw_hdr, "\nSwitch 0x%016" PRIx64 "\n"
		"LID    : Out Port(s)\n",
		cl_ntoh64(osm_node_get_node_guid(p_node)));
	first_mlid = TRUE;
	while (block_num <= p_tbl->max_block_in_use) {
		mlid_start_ho = (uint16_t) (block_num * IB_MCAST_BLOCK_SIZE);
		for (i = 0; i < IB_MCAST_BLOCK_SIZE; i++) {
			mlid_ho = mlid_start_ho + i;
			position = 0;
			first_port = TRUE;
			sprintf(mlid_hdr, "0x%04X :",
				mlid_ho + IB_LID_MCAST_START_HO);
			while (position <= p_tbl->max_position) {
				mask_entry =
				    cl_ntoh16((*p_tbl->
					       p_mask_tbl)[mlid_ho][position]);
				if (mask_entry == 0) {
					position++;
					continue;
				}
				for (j = 0; j < 16; j++) {
					if ((1 << j) & mask_entry) {
						if (first_mlid) {
							fprintf(file, "%s",
								sw_hdr);
							first_mlid = FALSE;
						}
						if (first_port) {
							fprintf(file, "%s",
								mlid_hdr);
							first_port = FALSE;
						}
						fprintf(file, " 0x%03X ",
							j + (position * 16));
					}
				}
				position++;
			}
			if (first_port == FALSE)
				fprintf(file, "\n");
		}
		block_num++;
	}
}

static void dump_lid_matrix(cl_map_item_t * p_map_item, void *cxt)
{
	osm_switch_t *p_sw = (osm_switch_t *) p_map_item;
	osm_opensm_t *p_osm = ((struct dump_context *)cxt)->p_osm;
	FILE *file = ((struct dump_context *)cxt)->file;
	osm_node_t *p_node = p_sw->p_node;
	unsigned max_lid = p_sw->max_lid_ho;
	unsigned max_port = p_sw->num_ports;
	uint16_t lid;
	uint8_t port;

	fprintf(file, "Switch: guid 0x%016" PRIx64 "\n",
		cl_ntoh64(osm_node_get_node_guid(p_node)));
	for (lid = 1; lid <= max_lid; lid++) {
		osm_port_t *p_port;
		if (osm_switch_get_least_hops(p_sw, lid) == OSM_NO_PATH)
			continue;
		fprintf(file, "0x%04x:", lid);
		for (port = 0; port < max_port; port++)
			fprintf(file, " %02x",
				osm_switch_get_hop_count(p_sw, lid, port));
		p_port = cl_ptr_vector_get(&p_osm->subn.port_lid_tbl, lid);
		if (p_port)
			fprintf(file, " # portguid 0x%" PRIx64,
				cl_ntoh64(osm_port_get_guid(p_port)));
		fprintf(file, "\n");
	}
}

static void dump_ucast_lfts(cl_map_item_t * p_map_item, void *cxt)
{
	osm_switch_t *p_sw = (osm_switch_t *) p_map_item;
	osm_opensm_t *p_osm = ((struct dump_context *)cxt)->p_osm;
	FILE *file = ((struct dump_context *)cxt)->file;
	osm_node_t *p_node = p_sw->p_node;
	unsigned max_lid = p_sw->max_lid_ho;
	unsigned max_port = p_sw->num_ports;
	uint16_t lid;
	uint8_t port;

	fprintf(file, "Unicast lids [0x0-0x%x] of switch Lid %u guid 0x%016"
		PRIx64 " (\'%s\'):\n",
		max_lid, osm_node_get_base_lid(p_node, 0),
		cl_ntoh64(osm_node_get_node_guid(p_node)), p_node->print_desc);
	for (lid = 0; lid <= max_lid; lid++) {
		osm_port_t *p_port;
		port = osm_switch_get_port_by_lid(p_sw, lid);

		if (port >= max_port)
			continue;

		fprintf(file, "0x%04x %03u # ", lid, port);

		p_port = cl_ptr_vector_get(&p_osm->subn.port_lid_tbl, lid);
		if (p_port) {
			p_node = p_port->p_node;
			fprintf(file, "%s portguid 0x016%" PRIx64 ": \'%s\'",
				ib_get_node_type_str(osm_node_get_type(p_node)),
				cl_ntoh64(osm_port_get_guid(p_port)),
				p_node->print_desc);
		} else
			fprintf(file, "unknown node and type");
		fprintf(file, "\n");
	}
	fprintf(file, "%u lids dumped\n", max_lid);
}

/**********************************************************************
 **********************************************************************/
static void dump_qmap(osm_opensm_t * p_osm, FILE * file,
		      cl_qmap_t * map, void (*func) (cl_map_item_t *, void *))
{
	struct dump_context dump_context;

	dump_context.p_osm = p_osm;
	dump_context.file = file;

	cl_qmap_apply_func(map, func, &dump_context);
}

static void dump_qmap_to_file(osm_opensm_t * p_osm, const char *file_name,
			      cl_qmap_t * map,
			      void (*func) (cl_map_item_t *, void *))
{
	char path[1024];
	FILE *file;

	snprintf(path, sizeof(path), "%s/%s",
		 p_osm->subn.opt.dump_files_dir, file_name);

	file = fopen(path, "w");
	if (!file) {
		osm_log(&p_osm->log, OSM_LOG_ERROR,
			"dump_qmap_to_file: "
			"cannot create file \'%s\': %s\n",
			path, strerror(errno));
		return;
	}

	dump_qmap(p_osm, file, map, func);

	fclose(file);
}

/**********************************************************************
 **********************************************************************/

void osm_dump_mcast_routes(osm_opensm_t * osm)
{
	if (osm_log_is_active(&osm->log, OSM_LOG_ROUTING)) {
		/* multicast routes */
		dump_qmap_to_file(osm, "opensm.mcfdbs",
				  &osm->subn.sw_guid_tbl, dump_mcast_routes);
	}
}

void osm_dump_all(osm_opensm_t * osm)
{
	if (osm_log_is_active(&osm->log, OSM_LOG_ROUTING)) {
		/* unicast routes */
		dump_qmap_to_file(osm, "opensm-lid-matrix.dump",
				  &osm->subn.sw_guid_tbl, dump_lid_matrix);
		dump_qmap_to_file(osm, "opensm-lfts.dump",
				  &osm->subn.sw_guid_tbl, dump_ucast_lfts);
		if (osm_log_is_active(&osm->log, OSM_LOG_DEBUG))
			dump_qmap(osm, NULL, &osm->subn.sw_guid_tbl,
				  dump_ucast_path_distribution);
		dump_qmap_to_file(osm, "opensm.fdbs",
				  &osm->subn.sw_guid_tbl, dump_ucast_routes);
		/* multicast routes */
		dump_qmap_to_file(osm, "opensm.mcfdbs",
				  &osm->subn.sw_guid_tbl, dump_mcast_routes);
	}
}
