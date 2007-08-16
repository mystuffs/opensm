/*
 * Copyright (c) 2004-2006 Voltaire, Inc. All rights reserved.
 * Copyright (c) 2002-2005 Mellanox Technologies LTD. All rights reserved.
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
 *    Implementation of osm_state_mgr_ctrl_t.
 * This object represents the State Manager Controller object.
 * This object is part of the opensm family of objects.
 *
 * Environment:
 *    Linux User Mode
 *
 * $Revision: 1.5 $
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif				/* HAVE_CONFIG_H */

#include <string.h>
#include <opensm/osm_state_mgr_ctrl.h>
#include <opensm/osm_msgdef.h>

/**********************************************************************
 **********************************************************************/
void __osm_state_mgr_ctrl_disp_callback(IN void *context, IN void *p_data)
{
	/* ignore return status when invoked via the dispatcher */
	osm_state_mgr_process(((osm_state_mgr_ctrl_t *) context)->p_mgr,
			      (osm_signal_t) (p_data));
}

/**********************************************************************
 **********************************************************************/
void osm_state_mgr_ctrl_construct(IN osm_state_mgr_ctrl_t * const p_ctrl)
{
	memset(p_ctrl, 0, sizeof(*p_ctrl));
	p_ctrl->h_disp = CL_DISP_INVALID_HANDLE;
}

/**********************************************************************
 **********************************************************************/
void osm_state_mgr_ctrl_destroy(IN osm_state_mgr_ctrl_t * const p_ctrl)
{
	CL_ASSERT(p_ctrl);
	cl_disp_unregister(p_ctrl->h_disp);
}

/**********************************************************************
 **********************************************************************/
ib_api_status_t
osm_state_mgr_ctrl_init(IN osm_state_mgr_ctrl_t * const p_ctrl,
			IN osm_state_mgr_t * const p_mgr,
			IN osm_log_t * const p_log,
			IN cl_dispatcher_t * const p_disp)
{
	ib_api_status_t status = IB_SUCCESS;

	OSM_LOG_ENTER(p_log, osm_state_mgr_ctrl_init);

	osm_state_mgr_ctrl_construct(p_ctrl);
	p_ctrl->p_log = p_log;

	p_ctrl->p_mgr = p_mgr;
	p_ctrl->p_disp = p_disp;

	p_ctrl->h_disp = cl_disp_register(p_disp,
					  OSM_MSG_NO_SMPS_OUTSTANDING,
					  __osm_state_mgr_ctrl_disp_callback,
					  p_ctrl);

	if (p_ctrl->h_disp == CL_DISP_INVALID_HANDLE) {
		osm_log(p_log, OSM_LOG_ERROR,
			"osm_state_mgr_ctrl_init: ERR 3401: "
			"Dispatcher registration failed\n");
		status = IB_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

      Exit:
	OSM_LOG_EXIT(p_log);
	return (status);
}
