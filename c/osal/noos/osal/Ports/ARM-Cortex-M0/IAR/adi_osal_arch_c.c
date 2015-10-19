/* $Revision: 31555 $
 * $Date: 2015-06-26 19:07:26 +0800 (Fri, 26 Jun 2015) $
******************************************************************************
Copyright (c), 2009-2012 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:      OSAL functions specific to Cortex-M4 which are not specific to an
            OSAL environment (valid for no-OS, ucos-III, etc)

Description:
              Operating System Abstraction Layer for the environment where
              there is no OS(Operating System) running.

*****************************************************************************/

/*=============  I N C L U D E S   =============*/


#include <stdlib.h>     /* for NULL definition */
#include <limits.h>
#include "adi_osal.h"
#include "adi_osal_arch.h"
#include "adi_osal_arch_internal.h"
#include "osal_common.h"

/*=============  D E F I N E S  =============*/


/* exceptions are exactly numbered -1 through -15,
   while overloaded (shared) interrupts will have the
   0x80000000 bit set (-FS), but so will exceptions.
   So check for bits 30:20 being set to flag (negative)
   exception values... not just an arithmetic less than
   zero test.  Bits 30:20 span from just adter the TC
   position, to (but not including) the MS bit of SI
   subindexing scheme (never more than 9 SI):
      Bit  31    = Set to 1 if the interrupt is shared
      Bits 30:16 = Bit position of the interrupt status bit in the PADS SISTATn register
      Bits 15:0  = Shared interrupt number
*/
#define IID_IS_EXCEPTION(iid) ((int32_t)iid & 0x7ff00000)


/* interrupt sharing (Tesla M0) uses different interrupt wrapper
   shared interrupts are non-exceptions that have the 0x80000000
   bit set
*/
#define IID_IS_SHARED(iid)  ((int32_t)iid & 0x80000000)


static void _adi_osal_plainWrapper (void);

/*=============  C O D E  =============*/

/*!
  ****************************************************************************
   @internal

   @brief This function abstracts the creation of a heap with the information
    provided.

   @details Creates a heap with the information provided. The user ID might be
            in use by application code. For this reason, if heap_install fails
            OSAL will change the user ID 10 times before giving up and
            returning an error

   Parameters:
   @param[in] pHeapMemory - Pointer to the allocated memory
   @param[in] nHeapMemorySize  - Size of memory to be allocated

   @return ADI_OSAL_SUCCESS    - if the heap was created successfully
   @return ADI_OSAL_MEM_ALLOC_FAILED  - if the heap could not be created

   @endinternal

*****************************************************************************/

ADI_OSAL_STATUS _adi_osal_HeapInstall(uint32_t *pHeapMemory, uint32_t nHeapMemorySize)
{
   return ADI_OSAL_SUCCESS;
}

/*!
  ****************************************************************************
   @internal

   @brief Abstracts which heap to use for dynamic memory allocations

   @param[out] ppData - Pointer that will store the allocated memory pointer
   @param[in] nSize  - Size of memory to be allocated

   @return ADI_OSAL_SUCCESS - if the memory was allocated correctly
   @return ADI_OSAL_MEM_ALLOC_FAILED  - if the memory could not be allocated

   @endinternal

*****************************************************************************/

ADI_OSAL_STATUS _adi_osal_MemAlloc(void** ppData, uint32_t nSize)
{
    void* pMemory;

   pMemory = malloc(nSize);

    if (pMemory != NULL)
    {
        *ppData = pMemory;
        return (ADI_OSAL_SUCCESS);
    }
    else
    {
        return (ADI_OSAL_MEM_ALLOC_FAILED);
    }
}



/*!
  ****************************************************************************
   @internal

   @brief Abstracts which heap to use to free dynamic memory

   @param[in] pData - Memory area to be freed (same argument as free)

   @endinternal

*****************************************************************************/

void  _adi_osal_MemFree(void* pData)
{
    free(pData);
}

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
    ADI_NVIC_HANDLER pfWrapper;
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
     * handling between system exceptions and interrupts.
     */
    pfWrapper = &_adi_osal_stdWrapper;

    if (IID_IS_EXCEPTION(iid))
    {
        pfWrapper = &_adi_osal_plainWrapper;
    }
    else if (IID_IS_SHARED(iid))
    {
        pfWrapper = &_adi_osal_sharedWrapper;  // shared interrupt disambiguator
    }

    index = adi_nvic_RegisterHandler(iid, pfWrapper);

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
        adi_nvic_UnRegisterHandler (iid);
        return ADI_OSAL_FAILED;
    }

    if (IID_IS_SHARED(iid)) {

        // all shared interrupts use a common subdispatcher: _adi_osal_sharedWrapper().
        // store registration handler, callback arg and iid for subdispatcher
        // note: _adi_osal_gSharedHandlerTable indexing EXCLUDES the exception space.
        uint32_t ivtIndex = iid & 0x0000ffff;

        // subindex is from iid (bits 30:16)
        uint32_t siIndex = (iid & 0x7fff0000) >> 16;

        // store the handler, iid and handler arg for a shared interrupt
        _adi_osal_gSharedHandlerTable[ivtIndex].siVectors[siIndex].pfOSALHandler = highLevelHandler;
        _adi_osal_gSharedHandlerTable[ivtIndex].siVectors[siIndex].pOSALArg      = handlerArg;
        _adi_osal_gSharedHandlerTable[ivtIndex].siVectors[siIndex].iid           = iid;

    } else {

        // dedicated interrupts go through usual OSAL wrapper for dispatch
        _adi_osal_gHandlerTable[index].pfOSALHandler = highLevelHandler;
        _adi_osal_gHandlerTable[index].pOSALArg      = handlerArg;
    }
    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief  Activates a high-level interrupt handler
    .

    @param[in] iid - ID of the interrupt to be handled

    @return ADI_OSAL_SUCCESS      - If handler is successfully activated
    @return ADI_OSAL_FAILED       - If failed to activate handler

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_ActivateHandler (uint32_t iid)
{
    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief  Deactivates a high-level interrupt handler

    @param[in] iid - ID of the interrupt to be handled

    @return ADI_OSAL_SUCCESS      - If handler is successfully deactivated
    @return ADI_OSAL_FAILED       - If failed to deactivate handler

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_DeactivateHandler (uint32_t iid)
{
    return ADI_OSAL_SUCCESS;
}


/*!
  ****************************************************************************
    @brief  Uninstalls a high-level interrupt handler
    .

    @param[in] iid - ID of the interrupt to be handled

    @return ADI_OSAL_SUCCESS      - If handler is successfully uninstalled
    @return ADI_OSAL_FAILED       - If failed to uninstall handler
    @return ADI_OSAL_CALLER_ERROR - If function is invoked from an invalid
                                    location

*****************************************************************************/
ADI_OSAL_STATUS
adi_osal_UninstallHandler (uint32_t iid)
{
    int32_t index;

#ifdef OSAL_DEBUG
    if (CALLED_FROM_AN_ISR)
    {
        return ADI_OSAL_CALLER_ERROR;
    }
#endif /* OSAL_DEBUG */

    index = adi_nvic_UnRegisterHandler (iid);

    if (index < 0)
    {
        /* Error - the IID doesn't correspond to a registered handler */
        return ADI_OSAL_FAILED;
    }

#ifdef OSAL_DEBUG
    if (index >= (int)_adi_osal_gHandlerTableSize)
    {
        /* OSAL's dispatch table is too small for the returned index.
         * Something would have to go badly wrong for this to happen
         * (e.g. memory corruption) as the index should always be the
         * same as when the handler was registered.
         */
        return ADI_OSAL_FAILED;
    }
#endif /* OSAL_DEBUG */

    if (IID_IS_SHARED(iid)) {

        // all shared interrupts use a common subdispatcher: _adi_osal_sharedWrapper().
        // store registration handler, callback arg and iid for subdispatcher
        // note: _adi_osal_gSharedHandlerTable indexing EXCLUDES the exception space.
        uint32_t ivtIndex = iid & 0x0000ffff;

        // subindex is from iid (bits 30:16)
        uint32_t siIndex = (iid & 0x7fff0000) >> 16;

        /* remove from shared interrupt table */
        _adi_osal_gSharedHandlerTable[ivtIndex].siVectors[siIndex].pfOSALHandler = NULL;
        _adi_osal_gSharedHandlerTable[ivtIndex].siVectors[siIndex].pOSALArg      = NULL;

    } else {

        /* remove from standard handler table */
        _adi_osal_gHandlerTable[index].pfOSALHandler = NULL;
        _adi_osal_gHandlerTable[index].pOSALArg      = NULL;
    }

    return ADI_OSAL_SUCCESS;
}



/*
 * This is the wrapper that is used for interrupts that cannot
 * support OS context-switching (on uC/OS, at least). At present
 * it is used for exceptions and the non-maskable interrupt.
 */
static void _adi_osal_plainWrapper (void)
{
    /* Get the interrupt number */
    uint32_t nIntNum = __get_IPSR();

    _adi_osal_gHandlerTable[nIntNum].pfOSALHandler(ADI_NVIC_IRQ_SID(nIntNum),  _adi_osal_gHandlerTable[nIntNum].pOSALArg);
}


/*
**
** EOF:
**
*/
