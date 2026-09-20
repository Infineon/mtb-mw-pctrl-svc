/***************************************************************************//**
* \file cy_ppca_shm.h
* \brief
*   This file contains the inline function, macros and constants for inter-core
*   communication based on shared memory implementation.
*
********************************************************************************
* \copyright
* (c) 2025-2026, Infineon Technologies AG or an affiliate of
* Infineon Technologies AG.
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/**
 * \defgroup group_shared_memory Shared Memory
 * \{
 * The shared memory feature in this library enables fast and direct data
 * exchange between processor cores in a multi-core system by providing a
 * configurable region of memory accessible to all participating cores.
 *
 * The library abstracts the complexity of shared memory management and offers
 * APIs for initializing, reading, and writing shared memory variables and blocks.
 *
 * \defgroup group_shared_memory_macros Macros
 * Macros for defining shared memory variables.
 * \defgroup group_shared_memory_config Configuration
 * The configuration is defined in \c cy_ppca_shm_config.h. Copy that template
 * file to the \c shared/ folder of your multi-core application, remove the
 * \c \#warning line, and adjust the \c CONFIG_* defines to match your needs.
 * Add the folder to every core project Makefile with
 * <tt>SEARCH += ./../shared</tt> so the project-specific copy is picked up
 * instead of the library template.
 * \defgroup group_shared_memory_data_structures Data Structures
 * Data structures for shared memory block access.
 * \defgroup group_shared_memory_functions Functions
 * Functions for shared memory variable and block access.
 * \} */


#ifndef CY_PPCA_SHM_H
#define CY_PPCA_SHM_H

#include "cy_ppca_shm_common.h"


#if CONFIG_USE_DOUBLE_BUFFER
#include "cy_ppca_shm_db.h"
#endif

#if CONFIG_USE_TRIPLE_BUFFER
#include "cy_ppca_shm_tb.h"
#endif

#if CONFIG_USE_MEMORY_POOL
#include "cy_ppca_shm_pool.h"
#endif

/* Set to 1 to re-enable deprecated macro names (__SM_ATTR__ etc.).
 * Disabled by default: those identifiers start with __ and are reserved
 * by the C standard (MISRA C-2012 Rules 21.1 and 21.2).
 * New code should use CY_SECTION_PPCA_SHM and related macros directly. */
#ifndef CY_PPCA_SHM_ENABLE_OLD_API_COMPATIBILITY
#define CY_PPCA_SHM_ENABLE_OLD_API_COMPATIBILITY   1
#endif
#if CY_PPCA_SHM_ENABLE_OLD_API_COMPATIBILITY
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 21.1', 4, \
                             'Deprecated macros retained for backward API compatibility; new code should use CY_SECTION_PPCA_SHM directly.')
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 21.2', 4, \
                             'Deprecated macros retained for backward API compatibility; new code should use CY_SECTION_PPCA_SHM directly.')
    #define __SM_ATTR__                     CY_SECTION_PPCA_SHM volatile
    #define __SM_ATTR_SUB__(subsection)     CY_SECTION_PPCA_SHM_SUB(subsection) volatile
    #define __SM_ATTR_NV__                  CY_SECTION_PPCA_SHM
    #define __SM_ATTR_SUB_NV__(subsection)  CY_SECTION_PPCA_SHM_SUB(subsection)
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 21.2')
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 21.1')
    #define sm_init                         Cy_PPCA_SharedMem_Init
    #define sm_memcpy32                     Cy_PPCA_SharedMem_MemCpy32
    #define sm_memset32                     Cy_PPCA_SharedMem_MemSet32
    #define sm_read                         Cy_PPCA_SharedMem_Read
    #define sm_write                        Cy_PPCA_SharedMem_Write
    #define sm_set_bits                     Cy_PPCA_SharedMem_SetBits
    #define sm_clr_bits                     Cy_PPCA_SharedMem_ClrBits
    #define sm_chk_clr_bits                 Cy_PPCA_SharedMem_CheckClrBits
    #define sm_channel_t                    cy_ppca_shm_channel_t
    #define sm_blk_t                        cy_ppca_shm_blk_t
    #define sm_blk_init                     Cy_PPCA_SharedMem_Blk_Init
    #define sm_blk_lock_acquire             Cy_PPCA_SharedMem_Blk_LockAcquire
    #define sm_blk_lock_release             Cy_PPCA_SharedMem_Blk_LockRelease
    #define sm_blk_write                    Cy_PPCA_SharedMem_Blk_Write
    #define sm_blk_read                     Cy_PPCA_SharedMem_Blk_Read

    #define sm_db_blk_t                     cy_ppca_shm_db_blk_t
    #define sm_db_blk_init                  Cy_PPCA_SharedMem_DB_Init
    #define sm_db_blk_write                 Cy_PPCA_SharedMem_DB_Write
    #define sm_db_blk_read                  Cy_PPCA_SharedMem_DB_Read
    #define sm_tb_blk_t                     cy_ppca_shm_tb_blk_t
    #define sm_tb_blk_init                  Cy_PPCA_SharedMem_TB_Init
    #define sm_tb_blk_get_write             Cy_PPCA_SharedMem_TB_GetWriteBuf
    #define sm_tb_blk_update_write          Cy_PPCA_SharedMem_TB_UpdateWriteBuf
    #define sm_tb_blk_get_read              Cy_PPCA_SharedMem_TB_GetReadBuf
    #define sm_tb_blk_write                 Cy_PPCA_SharedMem_TB_Write
    #define sm_tb_blk_read                  Cy_PPCA_SharedMem_TB_Read
#endif /* CY_PPCA_SHM_ENABLE_OLD_API_COMPATIBILITY */

/** \addtogroup group_shared_memory_functions
 * \{
 */

/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Init
****************************************************************************//**
*
* \brief Initializes the shared memory block data access.
*
* \note This should be called in the main core. Optional for PPCA cores.
*
*******************************************************************************/
void Cy_PPCA_SharedMem_Init(void);



/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Read
****************************************************************************//**
*
* \brief Reads a 32-bit value from a shared memory variable.
*
* \param var_addr
* The pointer to the variable in the shared memory region.
*
* \return
* 32-bit value read from shared memory.
*
* \note The variable must be declared at a 32-bit aligned address, e.g.,
*       __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_PPCA_SharedMem_Read(volatile uint32_t* var_addr)
{
    return *var_addr;
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Write
****************************************************************************//**
*
* \brief Writes a 32-bit value to a shared memory variable.
*
* \param var_addr
* The pointer to the variable in the shared memory region.
*
* \param value
* 32-bit value to be written.
*
* \note The variable must be declared at a 32-bit aligned address, e.g.,
*       __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_Write(volatile uint32_t* var_addr, uint32_t value)
{
    *var_addr = value;
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_SetBits
****************************************************************************//**
*
* \brief Perform a read-modify-write operation to set bits in a shared memory variable.
*
* \param var_addr
* The pointer to the variable in the shared memory region.
*
* \param bitmask
* 32-bit value that contains the bits to be set.
*
* \return
* 32-bit value old value before update
*
* \note The variable must be declared at a 32-bit aligned address, e.g.,
*       __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_PPCA_SharedMem_SetBits(volatile uint32_t* var_addr, uint32_t bitmask)
{
    uint32_t old_value;
    do
    {
        old_value = __LDREXW(var_addr);
    } while (__STREXW(old_value | bitmask, var_addr) != 0U);
    __DMB();

    return old_value;
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_ClrBits
****************************************************************************//**
*
* \brief Perform a read-modify-write operation to clear bits in a shared memory variable.
*
* \param var_addr
* The pointer to the variable in the shared memory region.
*
* \param bitmask
* 32-bit value that contains the bits to be cleared.
*
* \return
* 32-bit value old value before update
*
* \note The variable must be declared at a 32-bit aligned address, e.g.,
*       __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_PPCA_SharedMem_ClrBits(volatile uint32_t* var_addr, uint32_t bitmask)
{
    uint32_t old_value;
    do
    {
        old_value = __LDREXW(var_addr);
    } while (__STREXW(old_value & (uint32_t)(~bitmask), var_addr) != 0U);
    __DMB();

    return old_value;
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_CheckClrBits
****************************************************************************//**
*
* \brief Check if the bits are set and perform read-modify-write operation to clear bits in a shared memory variable.
*
* \param var_addr
* The pointer to the variable in the shared memory region.
*
* \param bitmask
* 32-bit value that contains the bits to be cleared.
*
* \return
* true if the bits were set before clearing, false otherwise.
*
* \note The variable must be declared at a 32-bit aligned address, e.g.,
*       __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE bool Cy_PPCA_SharedMem_CheckClrBits(volatile uint32_t* var_addr, uint32_t bitmask)
{
    uint32_t old_value;
    uint32_t new_value;
    bool result = false;

    do
    {
        old_value = __LDREXW(var_addr);
        if ((old_value & bitmask) != 0U)
        {
            new_value = old_value & (uint32_t)(~bitmask);
            result = true;
        }
        else
        {
            new_value = old_value;
            result = false;
        }
    } while (__STREXW(new_value, var_addr) != 0U);
    __DMB();

    return result;
}


/** \} group_shared_memory_functions */


/******************************************************************************/
// CONFIG - Use lock for shared memory block access
//
//  The data structure is accessed directly as is, with a lock to ensure
//  mutual exclusive access
//
//  The lock is implemented by IPC block or MUTEX
//
/** \addtogroup group_shared_memory_data_structures
 * \{
 */

/**
 * \brief Locking channel index type for shared memory block access.
 * \note The actual lock implementation depends on the configuration of \c CONFIG_USE_IPC_LOCK
 *   or \c CONFIG_USE_MUTEX_LOCK.
 */
typedef uint32_t cy_ppca_shm_channel_t;


/** \} group_shared_memory_data_structures */


#if CONFIG_USE_IPC_LOCK
extern IPC_STRUCT_Type* ipc_lock[CONFIG_USE_LOCK_MAX_CHAL_NUM];

// Workaround for PPCA IPC
    #if defined(CY_DEVICE_PSC3_P8)
        #define SM_IPC_STRUCT_PTR(ipcIndex)  ((IPC_STRUCT_Type*)&PPCA->PPCA_IPC.STRUCT[ipcIndex])
    #else
        #define SM_IPC_STRUCT_PTR(ipcIndex)  ((IPC_STRUCT_Type*)(IPC_BASE + (sizeof(IPC_STRUCT_Type) * (ipcIndex))))
    #endif
#endif

#if CONFIG_USE_MUTEX_LOCK
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 8.15', 1, \
                             'Alignment difference is due to CY_SECTION_PPCA_SHM attribute on the definition in cy_ppca_shm_common.c; declaration and definition refer to the same object.')
extern volatile mutex_t mutex[CONFIG_USE_LOCK_MAX_CHAL_NUM];
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 8.15')
#endif


/** \addtogroup group_shared_memory_functions
 * \{
 */

/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Blk_LockAcquire
****************************************************************************//**
*
* \brief Acquires a lock for shared memory block data access.
*
* \param chnl
* The channel to be locked.
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_Blk_LockAcquire(cy_ppca_shm_channel_t chnl)
{
    #if CONFIG_USE_IPC_LOCK
    while (Cy_IPC_Drv_LockAcquire(ipc_lock[chnl]) != CY_IPC_DRV_SUCCESS)
    {
    }
    #endif

    #if CONFIG_USE_MUTEX_LOCK
    while (mutex_lock(&mutex[chnl]) != true)
    {
    }
    #endif
}


/*******************************************************************************
* Function Name: sm_blk_lock_release
****************************************************************************//**
*
* \brief Releases a lock for shared memory block data access.
*
* \param chnl
* The channel to be unlocked.
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_Blk_LockRelease(cy_ppca_shm_channel_t chnl)
{
    #if CONFIG_USE_IPC_LOCK
    (void)Cy_IPC_Drv_LockRelease(ipc_lock[chnl], CY_IPC_NO_NOTIFICATION);
    #endif

    #if CONFIG_USE_MUTEX_LOCK
    mutex_unlock(&mutex[chnl]);
    #endif
}


#if CONFIG_USE_MEMORY_POOL == 0
/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Blk_Write
****************************************************************************//**
*
* \brief Writes a block of data to a shared memory variable.
*
* \param sm_blk
* The pointer to the variable in the shared memory region.
*
* \param data_ptr
* The pointer to the block data.
*
* \param size
* The size of the data block.
*
* \param chnl
* Channel for data sharing.
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_Blk_Write(volatile void* sm_blk, const void* data_ptr, uint32_t size,
                                                 cy_ppca_shm_channel_t chnl)
{
    Cy_PPCA_SharedMem_Blk_LockAcquire(chnl);
    Cy_PPCA_SharedMem_MemCpy32(sm_blk, data_ptr, size);
    Cy_PPCA_SharedMem_Blk_LockRelease(chnl);
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Blk_Read
****************************************************************************//**
*
* \brief Reads a block of data from a shared memory variable.
*
* \param sm_blk
* The pointer to the variable in the shared memory region.
*
* \param data_ptr
* The pointer to the block data.
*
* \param size
* The size of the data block.
*
* \param chnl
* Channel for data sharing.
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_Blk_Read(volatile void* sm_blk, void* data_ptr, uint32_t size,
                                                cy_ppca_shm_channel_t chnl)
{
    Cy_PPCA_SharedMem_Blk_LockAcquire(chnl);
    Cy_PPCA_SharedMem_MemCpy32(data_ptr, sm_blk, size);
    Cy_PPCA_SharedMem_Blk_LockRelease(chnl);
}


#else // if CONFIG_USE_MEMORY_POOL == 0


/**
 * \brief shared memory block type definition
 */
typedef struct
{
    cy_ppca_shm_channel_t   chnl;
    void*                   data;
    #if defined(CY_DEVICE_PSC3_P8)
    void*                   data_ppca;
    #endif
} cy_ppca_shm_blk_t;



/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Blk_Init
****************************************************************************//**
*
* \brief Initializes a shared memory block for data access from a mempool
*
* \param blk_ptr
* The pointer to the shared memory block.
*
* \param size
* The size of the shared memory block.
*
* \param chnl
* The channel for data sharing.
*
*******************************************************************************/
void Cy_PPCA_SharedMem_Blk_Init(volatile cy_ppca_shm_blk_t* blk_ptr, uint32_t size, cy_ppca_shm_channel_t chnl);



/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Blk_Write
****************************************************************************//**
*
* \brief Writes a block of data to a shared memory variable using mempool block descriptor
*
* \param sm_blk
* The pointer to the shared memory block.
*
* \param data_ptr
* The pointer to the block data.
*
* \param size
* The size of the data block.
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_Blk_Write(volatile cy_ppca_shm_blk_t* sm_blk, const void* data_ptr,
                                                 uint32_t size)
{
    cy_ppca_shm_channel_t chnl = sm_blk->chnl;
    Cy_PPCA_SharedMem_Blk_LockAcquire(chnl);

    #if defined(CY_DEVICE_PSC3_P8)
    #if CORE_NAME_CM33_0 == 1
    Cy_PPCA_SharedMem_MemCpy32(sm_blk->data, data_ptr, size);
    #else
    Cy_PPCA_SharedMem_MemCpy32(sm_blk->data_ppca, data_ptr, size);
    #endif
    #else
    Cy_PPCA_SharedMem_MemCpy32(sm_blk->data, data_ptr, size);
    #endif

    Cy_PPCA_SharedMem_Blk_LockRelease(chnl);
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Blk_Read
****************************************************************************//**
*
* \brief Reads a block of data from a shared memory variable using mempool block descriptor.
*
* \param sm_blk
* The pointer to the shared memory block.
*
* \param data_ptr
* The pointer to the block data.
*
* \param size
* The size of the data block.
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_Blk_Read(volatile cy_ppca_shm_blk_t* sm_blk, void* data_ptr, uint32_t size)
{
    cy_ppca_shm_channel_t chnl = sm_blk->chnl;
    Cy_PPCA_SharedMem_Blk_LockAcquire(chnl);

    #if defined(CY_DEVICE_PSC3_P8)
    #if CORE_NAME_CM33_0 == 1
    Cy_PPCA_SharedMem_MemCpy32(data_ptr, sm_blk->data, size);
    #else
    Cy_PPCA_SharedMem_MemCpy32(data_ptr, sm_blk->data_ppca, size);
    #endif
    #else
    Cy_PPCA_SharedMem_MemCpy32(data_ptr, sm_blk->data, size);
    #endif

    Cy_PPCA_SharedMem_Blk_LockRelease(chnl);
}


#endif /* CONFIG_USE_MEMORY_POOL */

/** \} group_shared_memory_functions */



#endif /* CY_PPCA_SHM_H */
