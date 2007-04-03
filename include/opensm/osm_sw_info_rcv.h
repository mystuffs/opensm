/*
 * Copyright (c) 2004, 2005 Voltaire, Inc. All rights reserved.
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
 * 	Declaration of osm_si_rcv_t.
 *	This object represents the SwitchInfo Receiver object.
 *	attribute from a node.
 *	This object is part of the OpenSM family of objects.
 *
 * Environment:
 * 	Linux User Mode
 *
 * $Revision: 1.4 $
 */

#ifndef _OSM_SI_RCV_H_
#define _OSM_SI_RCV_H_

#include <complib/cl_passivelock.h>
#include <opensm/osm_base.h>
#include <opensm/osm_madw.h>
#include <opensm/osm_subnet.h>
#include <opensm/osm_req.h>
#include <opensm/osm_state_mgr.h>
#include <opensm/osm_log.h>

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else /* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif /* __cplusplus */

BEGIN_C_DECLS

/****h* OpenSM/Switch Info Receiver
* NAME
*	Switch Info Receiver
*
* DESCRIPTION
*	The Switch Info Receiver object encapsulates the information
*	needed to receive the SwitchInfo attribute from a node.
*
*	The Switch Info Receiver object is thread safe.
*
*	This object should be treated as opaque and should be
*	manipulated only through the provided functions.
*
* AUTHOR
*	Steve King, Intel
*
*********/

/****s* OpenSM: Switch Info Receiver/osm_si_rcv_t
* NAME
*	osm_si_rcv_t
*
* DESCRIPTION
*	Switch Info Receiver structure.
*
*	This object should be treated as opaque and should
*	be manipulated only through the provided functions.
*
* SYNOPSIS
*/
typedef struct _osm_si_rcv
{
	osm_subn_t	*p_subn;
	osm_log_t	*p_log;
	osm_req_t	*p_req;
	osm_state_mgr_t	*p_state_mgr;
	cl_plock_t	*p_lock;
} osm_si_rcv_t;
/*
* FIELDS
*	p_subn
*		Pointer to the Subnet object for this subnet.
*
*	p_log
*		Pointer to the log object.
*
*	p_req
*		Pointer to the Request object.
*
*	p_state_mgr
*		Pointer to the State Manager object.
*
*	p_lock
*		Pointer to the serializing lock.
*
* SEE ALSO
*	Switch Info Receiver object
*********/

/****f* OpenSM: Switch Info Receiver/osm_si_rcv_construct
* NAME
*	osm_si_rcv_construct
*
* DESCRIPTION
*	This function constructs a Switch Info Receiver object.
*
* SYNOPSIS
*/
void osm_si_rcv_construct(
	IN osm_si_rcv_t* const p_ctrl );
/*
* PARAMETERS
*	p_ctrl
*		[in] Pointer to a Switch Info Receiver object to construct.
*
* RETURN VALUE
*	This function does not return a value.
*
* NOTES
*	Allows calling osm_si_rcv_init, osm_si_rcv_destroy,
*	and osm_si_rcv_is_inited.
*
*	Calling osm_si_rcv_construct is a prerequisite to calling any other
*	method except osm_si_rcv_init.
*
* SEE ALSO
*	Switch Info Receiver object, osm_si_rcv_init,
*	osm_si_rcv_destroy, osm_si_rcv_is_inited
*********/

/****f* OpenSM: Switch Info Receiver/osm_si_rcv_destroy
* NAME
*	osm_si_rcv_destroy
*
* DESCRIPTION
*	The osm_si_rcv_destroy function destroys the object, releasing
*	all resources.
*
* SYNOPSIS
*/
void osm_si_rcv_destroy(
	IN osm_si_rcv_t* const p_ctrl );
/*
* PARAMETERS
*	p_ctrl
*		[in] Pointer to the object to destroy.
*
* RETURN VALUE
*	This function does not return a value.
*
* NOTES
*	Performs any necessary cleanup of the specified
*	Switch Info Receiver object.
*	Further operations should not be attempted on the destroyed object.
*	This function should only be called after a call to
*	osm_si_rcv_construct or osm_si_rcv_init.
*
* SEE ALSO
*	Switch Info Receiver object, osm_si_rcv_construct,
*	osm_si_rcv_init
*********/

/****f* OpenSM: Switch Info Receiver/osm_si_rcv_init
* NAME
*	osm_si_rcv_init
*
* DESCRIPTION
*	The osm_si_rcv_init function initializes a
*	Switch Info Receiver object for use.
*
* SYNOPSIS
*/
ib_api_status_t osm_si_rcv_init(
	IN osm_si_rcv_t* const p_ctrl,
	IN osm_subn_t* const p_subn,
	IN osm_log_t* const p_log,
	IN osm_req_t* const p_req,
	IN osm_state_mgr_t* const p_state_mgr,
	IN cl_plock_t* const p_lock );
/*
* PARAMETERS
*	p_ctrl
*		[in] Pointer to an osm_si_rcv_t object to initialize.
*
*	p_subn
*		[in] Pointer to the Subnet object for this subnet.
*
*	p_log
*		[in] Pointer to the log object.
*
*	p_req
*		[in] Pointer to an osm_req_t object.
*
*	p_state_mgr
*		[in] Pointer to the State Manager object.
*
*	p_lock
*		[in] Pointer to the OpenSM serializing lock.
*
* RETURN VALUES
*	IB_SUCCESS if the Switch Info Receiver object was initialized
*	successfully.
*
* NOTES
*	Allows calling other Switch Info Receiver methods.
*
* SEE ALSO
*	Switch Info Receiver object, osm_si_rcv_construct,
*	osm_si_rcv_destroy, osm_si_rcv_is_inited
*********/

/****f* OpenSM: Switch Info Receiver/osm_si_rcv_is_inited
* NAME
*	osm_si_rcv_is_inited
*
* DESCRIPTION
*	Indicates if the object has been initialized with osm_si_rcv_init.
*
* SYNOPSIS
*/
boolean_t osm_si_rcv_is_inited(
	IN const osm_si_rcv_t* const p_ctrl );
/*
* PARAMETERS
*	p_ctrl
*		[in] Pointer to an osm_si_rcv_t object.
*
* RETURN VALUES
*	TRUE if the object was initialized successfully,
*	FALSE otherwise.
*
* NOTES
*	The osm_si_rcv_construct or osm_si_rcv_init must be
*	called before using this function.
*
* SEE ALSO
*	Switch Info Receiver object, osm_si_rcv_construct,
*	osm_si_rcv_init
*********/

/****f* OpenSM: Switch Info Receiver/osm_si_rcv_process
* NAME
*	osm_si_rcv_process
*
* DESCRIPTION
*	Process the SwitchInfo attribute.
*
* SYNOPSIS
*/
void osm_si_rcv_process(
	IN void *context,
	IN void *data );
/*
* PARAMETERS
*	context
*		[in] Pointer to an osm_si_rcv_t object.
*
*	data
*		[in] Pointer to the MAD Wrapper containing the MAD
*		that contains the node's SwitchInfo attribute.
*
* RETURN VALUES
*	CL_SUCCESS if the SwitchInfo processing was successful.
*
* NOTES
*	This function processes a SwitchInfo attribute.
*
* SEE ALSO
*	Switch Info Receiver, Switch Info Response Controller
*********/

END_C_DECLS

#endif	/* _OSM_SI_RCV_H_ */
