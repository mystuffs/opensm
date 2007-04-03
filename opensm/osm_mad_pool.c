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
 *    Implementation of osm_mad_pool_t.
 * This object represents a pool of management datagram (MAD) objects.
 * This object is part of the opensm family of objects.
 *
 * Environment:
 *    Linux User Mode
 *
 * $Revision: 1.5 $
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <opensm/osm_mad_pool.h>
#include <opensm/osm_madw.h>
#include <opensm/osm_log.h>
#include <vendor/osm_vendor_api.h>

#define OSM_MAD_POOL_MIN_SIZE 256
#define OSM_MAD_POOL_GROW_SIZE 256


/**********************************************************************
 **********************************************************************/
cl_status_t
__osm_mad_pool_ctor(
  IN void* const p_object,
  IN void* context,
  OUT cl_pool_item_t** const pp_pool_item )
{
  osm_madw_t *p_madw = p_object;

  UNUSED_PARAM( context );
  osm_madw_construct( p_madw );
  /* CHECK THIS.  DOCS DON'T DESCRIBE THIS OUT PARAM. */
  *pp_pool_item = &p_madw->pool_item;
  return( CL_SUCCESS );
}

/**********************************************************************
 **********************************************************************/
void
osm_mad_pool_construct(
  IN osm_mad_pool_t* const p_pool )
{
  CL_ASSERT( p_pool );

  memset( p_pool, 0, sizeof(*p_pool) );
  cl_qlock_pool_construct( &p_pool->madw_pool );
}

/**********************************************************************
 **********************************************************************/
void
osm_mad_pool_destroy(
  IN osm_mad_pool_t* const p_pool )
{
  CL_ASSERT( p_pool );

  /* HACK: we still rarely see some mads leaking - so ignore this */
  /* cl_qlock_pool_destroy( &p_pool->madw_pool ); */
}

/**********************************************************************
 **********************************************************************/
ib_api_status_t
osm_mad_pool_init(
  IN osm_mad_pool_t* const p_pool,
  IN osm_log_t* const p_log )
{
  ib_api_status_t status;

  OSM_LOG_ENTER( p_log, osm_mad_pool_init );

  p_pool->p_log = p_log;

  status = cl_qlock_pool_init(
    &p_pool->madw_pool,
    OSM_MAD_POOL_MIN_SIZE,
    0,
    OSM_MAD_POOL_GROW_SIZE,
    sizeof( osm_madw_t ),
    __osm_mad_pool_ctor,
    NULL,
    p_pool );
  if( status != IB_SUCCESS )
  {
    osm_log( p_log, OSM_LOG_ERROR,
             "osm_mad_pool_init: ERR 0702: "
             "Grow pool initialization failed (%s)\n",
             ib_get_err_str(status) );
    goto Exit;
  }

 Exit:
  OSM_LOG_EXIT( p_log );
  return( status );
}

/**********************************************************************
 **********************************************************************/
osm_madw_t*
osm_mad_pool_get(
  IN osm_mad_pool_t* const p_pool,
  IN osm_bind_handle_t h_bind,
  IN const uint32_t total_size,
  IN const osm_mad_addr_t* const p_mad_addr )
{
  osm_madw_t *p_madw;
  ib_mad_t *p_mad;

  OSM_LOG_ENTER( p_pool->p_log, osm_mad_pool_get );

  CL_ASSERT( h_bind != OSM_BIND_INVALID_HANDLE );
  CL_ASSERT( total_size );

  /*
    First, acquire a mad wrapper from the mad wrapper pool.
  */
  p_madw = (osm_madw_t*)cl_qlock_pool_get( &p_pool->madw_pool );
  if( p_madw == NULL )
  {
    osm_log( p_pool->p_log, OSM_LOG_ERROR,
             "osm_mad_pool_get: ERR 0703: "
             "Unable to acquire MAD wrapper object\n" );
    goto Exit;
  }

  osm_madw_init( p_madw, h_bind, total_size, p_mad_addr );

  /*
    Next, acquire a wire mad of the specified size.
  */
  p_mad = osm_vendor_get( h_bind, total_size, &p_madw->vend_wrap );
  if( p_mad == NULL )
  {
    osm_log( p_pool->p_log, OSM_LOG_ERROR,
             "osm_mad_pool_get: ERR 0704: "
             "Unable to acquire wire MAD\n" );

    /* Don't leak wrappers! */
    cl_qlock_pool_put( &p_pool->madw_pool, (cl_pool_item_t*)p_madw );
    p_madw = NULL;
    goto Exit;
  }

  cl_atomic_inc( &p_pool->mads_out );
  /*
    Finally, attach the wire MAD to this wrapper.
  */
  osm_madw_set_mad( p_madw, p_mad );

  osm_log( p_pool->p_log, OSM_LOG_DEBUG,
           "osm_mad_pool_get: Acquired p_madw = %p, p_mad = %p, "
           "size = %u\n", p_madw, p_madw->p_mad, total_size );

 Exit:
  OSM_LOG_EXIT( p_pool->p_log );
  return( p_madw );
}

/**********************************************************************
 **********************************************************************/
osm_madw_t*
osm_mad_pool_get_wrapper(
  IN osm_mad_pool_t* const p_pool,
  IN osm_bind_handle_t h_bind,
  IN const uint32_t total_size,
  IN const ib_mad_t* const p_mad,
  IN const osm_mad_addr_t* const p_mad_addr )
{
  osm_madw_t *p_madw;

  OSM_LOG_ENTER( p_pool->p_log, osm_mad_pool_get_wrapper );

  CL_ASSERT( h_bind != OSM_BIND_INVALID_HANDLE );
  CL_ASSERT( total_size );
  CL_ASSERT( p_mad );

  /*
    First, acquire a mad wrapper from the mad wrapper pool.
  */
  p_madw = (osm_madw_t*)cl_qlock_pool_get( &p_pool->madw_pool );
  if( p_madw == NULL )
  {
    osm_log( p_pool->p_log, OSM_LOG_ERROR,
             "osm_mad_pool_get_wrapper: ERR 0705: "
             "Unable to acquire MAD wrapper object\n" );
    goto Exit;
  }

  /*
    Finally, initialize the wrapper object.
  */
  cl_atomic_inc( &p_pool->mads_out );
  osm_madw_init( p_madw, h_bind, total_size, p_mad_addr );
  osm_madw_set_mad( p_madw, p_mad );

  osm_log( p_pool->p_log, OSM_LOG_DEBUG,
           "osm_mad_pool_get_wrapper: Acquired p_madw = %p, p_mad = %p "
           "size = %u\n", p_madw, p_madw->p_mad, total_size );

 Exit:
  OSM_LOG_EXIT( p_pool->p_log );
  return( p_madw );
}

/**********************************************************************
 **********************************************************************/
osm_madw_t*
osm_mad_pool_get_wrapper_raw(
  IN osm_mad_pool_t* const p_pool )
{
  osm_madw_t *p_madw;

  OSM_LOG_ENTER( p_pool->p_log, osm_mad_pool_get_wrapper_raw );

  p_madw = (osm_madw_t*)cl_qlock_pool_get( &p_pool->madw_pool );

  osm_log( p_pool->p_log, OSM_LOG_DEBUG,
           "osm_mad_pool_get_wrapper_raw: "
           "Getting p_madw = %p\n", p_madw );

  osm_madw_init( p_madw, 0, 0, 0 );
  osm_madw_set_mad( p_madw, 0 );
  cl_atomic_inc( &p_pool->mads_out );

  OSM_LOG_EXIT( p_pool->p_log );
  return( p_madw );
}

/**********************************************************************
 **********************************************************************/
void
osm_mad_pool_put(
  IN osm_mad_pool_t* const p_pool,
  IN osm_madw_t* const p_madw )
{
  OSM_LOG_ENTER( p_pool->p_log, osm_mad_pool_put );

  CL_ASSERT( p_madw );

  osm_log( p_pool->p_log, OSM_LOG_DEBUG,
           "osm_mad_pool_put: Releasing p_madw = %p, p_mad = %p\n",
           p_madw, p_madw->p_mad );

  /*
    First, return the wire mad to the pool
  */
  if( p_madw->p_mad )
    osm_vendor_put( p_madw->h_bind, &p_madw->vend_wrap );

  /*
    Return the mad wrapper to the wrapper pool
  */
  cl_qlock_pool_put( &p_pool->madw_pool, (cl_pool_item_t*)p_madw );
  cl_atomic_dec( &p_pool->mads_out );

  OSM_LOG_EXIT( p_pool->p_log );
}

