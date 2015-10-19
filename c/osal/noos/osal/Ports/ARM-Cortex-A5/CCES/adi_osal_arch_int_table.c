/* $Revision: 26543 $
 * $Date: 2014-07-30 22:54:06 +0800 (Wed, 30 Jul 2014) $
******************************************************************************
Copyright (c), 2009-2011 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:      OSAL interrupt table definition on ARM Cortex A Series

Description:
           This is the definition of the OSAL interrupt table in which pointers
           to handlers written in C for the given interrupt are stored.

*****************************************************************************/

/*=============  I N C L U D E S   =============*/
#include "adi_osal.h"
#include "adi_osal_arch_internal.h"


/*=============  D E F I N E S  =============*/


/*!
    @internal
    @var _adi_osal_gHandlerTableSize
	The size of the OSAL dispatch table. The size needs to be large enough for 
    any index that we can get back from adi_rtl_register_dispatched_handler(). 
    At the moment this table needs to be mapped into a section which is in
    a CPLB locked area.
    @endinternal
 */
uint32_t _adi_osal_gHandlerTableSize =  (uint32_t) ADI_DISPATCHED_VECTOR_TABLE_SIZE;

/*!
    @internal
    @var _adi_osal_gHandlerTable
    This is the OSAL dispatch table. It is an array of function pointers, of
    the type corresponding to OSAL's plain C interrupt handlers (i.e. the
    high-level handlers). The table needs to be large enough for any index
    that we can get back from register_dispatched_handler().  
    @endinternal
 */

ADI_OSAL_HANDLER_PTR _adi_osal_gHandlerTable[ADI_DISPATCHED_VECTOR_TABLE_SIZE];

/*
**
** EOF: 
**
*/

