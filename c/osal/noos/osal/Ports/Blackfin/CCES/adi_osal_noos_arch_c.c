/* $Revision: 31348 $
 * $Date: 2015-06-09 22:38:16 +0800 (Tue, 09 Jun 2015) $
******************************************************************************
Copyright (c), 2009-2013 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:      OSAL for no OS Platform specific functions for blackfin

*****************************************************************************/

/*=============  I N C L U D E S   =============*/


#pragma diag(suppress:misra_rule_6_3 : "The types used by EX_DISPATCHED_HANDLER_NESTED are defined by the RTL")
/*Rule 14.7 indicates that a function shall have a single exit point */
#pragma diag(suppress:misra_rule_14_7:"Allowing several point of exit (mostly for handling parameter error checking) increases the code readability and therefore maintainability")
/* Rule 17.4(Req): Array indexing shall be the only allowed form of pointer arithmetic. */
#pragma diag(suppress:misra_rule_17_4:"This source needs to use pointer indexing")

#include <stdlib.h>     /* for NULL definition */
#include <sys/platform.h>     /* for NULL definition */
#include "adi_osal.h"
#include "osal_common.h"
#include "adi_osal_arch.h"
#include "adi_osal_arch_internal.h"

/*=============  D E F I N E S  =============*/


#pragma file_attr(  "libGroup=adi_osal.h")
#pragma file_attr(  "libName=libosal")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")


/*
 * The following macros test an interrupt identifier (IID) to see whether it is:
 *  - a system interrupt (bit 20 is clear)
 *  - an exception (bit 20 is set and IVG is 3)
 *  - the non-maskable interrupt (bit 20 is set and IVG is 2)
 * other bits are don't cares.
 *
 * The organisation of the IID is:
 *
 * 31 30 29 28 27 26 25 24 23 22 21 |    20    | 19 18 17 16 | 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
 * < --------- reserved –------- > |  <SYS?>  |   < IVG >   | < ------------ SID ----------------->
 * Bitmasks: 0xFFE00000             |0x00100000|  0x000F0000 |              0x0000FFFF
 */
#pragma diag(push)
/* Rule 19.7(Adv): A function shall be used in preference to a function-like macro. */
#pragma diag(suppress:misra_rule_19_7:"This source needs to use function-like macros")
#define IID_IS_SYSTEM(iid) (((iid) & 0x00100000u) == 0u)
#define IID_IS_EXCEPTION(iid) (((iid) & 0x001F0000u) == 0x00130000u)
#define IID_IS_NMI(iid) (((iid) & 0x001F0000u) == 0x00120000u)
#pragma diag(pop)


/*=============  C O D E  =============*/

/*
 * These are the environment-specific interrupt wrappers.
 * I.e. these are the wrappers that are used for interrupts that
 * may call operating system APIs, and hence must support
 * rescheduling. In the no-RTOS OSAL they have essentially the
 * same implementation as the plain wrapper, since there is
 * no rescheduling in this environment. We let the compiler
 * generate the appropriate code for saving and restoring registers,
 * and for setting up the C runtime.
 */
EX_DISPATCHED_HANDLER_NESTED(_adi_osal_stdWrapper, iid,  index, arg)
{
	(_adi_osal_gHandlerTable[index])(iid, (void*) arg);
}

/*
 * On BF5xx we need a special wrapper for system interrupts.
 * This is because the runtime library will pass the IVG level in the
 * upper half of the IID (since that's what was passed down to
 * adi_rtl_register_dispatched_handler()), but we don't want that to be seen
 * at the SSL level, where system interrupt IIDs are plain SIDs and the
 * upper half is always zero. It is OSAL's job to zero the top half of
 * the IID, since there is no intervening layer between the OSAL
 * interrupts wrappers and the user (or device-driver) high-level handlers.
 */
#if !defined(__ADI_HAS_SEC__)
EX_DISPATCHED_HANDLER_NESTED(_adi_osal_systemWrapper, iid,  index, arg)
{
	iid &= 0x0000FFFFu; /* clear the top half of the IID */
	(_adi_osal_gHandlerTable[index])(iid, (void*) arg);
}
#endif


/*!
  ****************************************************************************
    @brief  Installs a high-level interrupt handler
    .

    @param[in] iid - ID of the interrupt to be handled
    @param[in] highLevelHandler - Function address of the handler
    @param[in] handlerArg - Generic argument to be passed to the handler

    @return ADI_OSAL_SUCCESS      - If handler is successfully installed
    @return ADI_OSAL_FAILED       - If failed to install handler
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid
                                    location

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_InstallHandler (
   uint32_t iid,
   ADI_OSAL_HANDLER_PTR highLevelHandler,
   void* handlerArg
)
{
    adi_dispatched_handler_t pfWrapper;
    int32_t index;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }
#endif /* OSAL_DEBUG */

    /*
     * In the real no-RTOS OSAL we won't actually need to distinguish
     * between "plain" and "nested", as there will be no difference in
     * handling between core interrupts and NMI&exceptions. It's expanded out
     * here to show how it will work in the OS cases.
     *
     * On BF5xx Blackfin (only), the no-RTOS OSAL *will* still need to
     * distinguish between system interrupts and the others. A specific
     * wrapper is needed to mask out the IVG level from the "augmented" IID,
     * so that only the SID is passed to the handler.
     */
    pfWrapper = &_adi_osal_stdWrapper;

    if (IID_IS_EXCEPTION(iid) || IID_IS_NMI(iid))
    {
        pfWrapper = &_adi_osal_plainWrapper;
    }

#if !defined(__ADI_HAS_SEC__) /* BF5xx Blackfin */
    if(IID_IS_SYSTEM(iid))
    {
        /* If we're on BF5xx blackfin and this is a system interrupt then
         * register the system wrapper. This will convert the iid to remove the
         * IVG level before passing it to the high-level handler.
         */

        pfWrapper = &_adi_osal_systemWrapper;
    }
#endif /* BF5xx Blackfin */


    index = adi_rtl_register_dispatched_handler (iid,
                                                 pfWrapper,
                                                 handlerArg);

    if (index < 0)
    {
        /* error */
        return ADI_OSAL_FAILED;
    }

    if (index >= (int)_adi_osal_gHandlerTableSize)
    {
        /* Register succeeded but OSAL's dispatch table is
         * too small for the returned index.
         */
        adi_rtl_unregister_dispatched_handler (iid);
        return ADI_OSAL_FAILED;
    }

    _adi_osal_gHandlerTable[index] = highLevelHandler;
    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief Disables interrupts to enable atomic execution of a critical region
    of code.

    Note that critical regions may be nested. A count is maintained to ensure a
    matching number of calls to adi_ExitCriticalRegion are made before
    restoring interrupts. Each critical region is also (implicitly) a scheduler
    lock.

    @see adi_osal_ExitCriticalRegion
*****************************************************************************/

void adi_osal_EnterCriticalRegion( void )
{

    adi_rtl_disable_interrupts();

    return;
}

/*!
  ****************************************************************************
    @brief Re-enables interrupts and restores the interrupt status.

    This function decrements the count of nested critical regions. Use it as a
    closing bracket to adi_osal_EnterCriticalRegion. 
    The Enter/ExitCriticalRegion sequence must be nestable

    @see adi_osal_EnterCriticalRegion
*****************************************************************************/

void adi_osal_ExitCriticalRegion( void )
{
    adi_rtl_reenable_interrupts();
}

/*
**
** EOF: 
**
*/
