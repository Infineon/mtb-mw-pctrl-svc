/***************************************************************************//**
* \file cy_ppca_shm_common.h
* \version 0.1.0
* \brief
*   This file contains the common definition of the shared memory library
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

#ifndef CY_PPCA_SHM_COMMON_H
#define CY_PPCA_SHM_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "cmsis_compiler.h"
#include "cy_device.h"
#include "cy_utils.h"

#include "cy_ppca_shm_config.h"


/******************************************************************************/
// Common shared memory declaration header
#if defined(__ARMCC_VERSION)
#define SM_ICC_SECTION                  ".bss.cy_ppca_shm"
#else
#define SM_ICC_SECTION                  ".cy_ppca_shm"
#endif
#define SM_ICC_INT                      ".lib"
#define SM_ICC_SUBSECTION(subsection)   SM_ICC_SECTION subsection

/** \addtogroup group_shared_memory_macros
 * \{
 */

#ifdef DOXYGEN
/**
 * \brief Access attribute that places a variable in the PPCA shared memory section.
 *
 * Annotate a variable declaration with this macro to place it in the
 * \c .cy_ppca_shm linker section with 4-byte alignment, making it accessible
 * to all cores in the system.
 *
 * \note Example usage:
 * \code
 * CY_SECTION_PPCA_SHM volatile uint32_t my_shared_var;
 * \endcode
 */
#define CY_SECTION_PPCA_SHM

#endif /* DOXYGEN */

/* Toolchain specific definition of shared memory attribute */
#if defined(__ARMCC_VERSION)
    #define CY_SECTION_PPCA_SHM                 __attribute__((used)) \
                                                CY_ALIGN(4) \
                                                CY_SECTION(SM_ICC_SECTION)
    #define CY_SECTION_PPCA_SHM_SUB(subsection) __attribute__((used)) \
                                                CY_ALIGN(4) \
                                                CY_SECTION(SM_ICC_SECTION subsection)
#elif defined (__GNUC__)
    #define CY_SECTION_PPCA_SHM                 CY_ALIGN(4) \
                                                CY_SECTION(SM_ICC_SECTION)
    #define CY_SECTION_PPCA_SHM_SUB(subsection) CY_ALIGN(4) \
                                                CY_SECTION(SM_ICC_SECTION subsection)
#elif defined (__ICCARM__)
    #define CY_SECTION_PPCA_SHM                 CY_PRAGMA(data_alignment=4) \
                                                CY_PRAGMA(location=SM_ICC_SECTION)
    #define CY_SECTION_PPCA_SHM_SUB(subsection) CY_PRAGMA(data_alignment=4) \
                                                CY_PRAGMA(location=SM_ICC_SECTION subsection)
#else // if defined(__ARMCC_VERSION)
    #error "An unsupported toolchain"
#endif /* defined(__ARMCC_VERSION) */

/** \} group_shared_memory_macros */


#if defined(CY_DEVICE_PSC3_P8)
    #define SHM_ADDR_MASK           (0x0000FFFF)
    #define MAIN_CPU_ADDR_MASK      (0x43050000)
    #define PPCA_CPU_ADDR_MASK      (0x20040000)
    #define SHM_RELATIVE_ADDR(x)    ((uint32_t)x & SHM_ADDR_MASK)
    #define MAIN_CPU_ADDR(x)        (uint32_t*)(SHM_RELATIVE_ADDR(x) | MAIN_CPU_ADDR_MASK)
    #define PPCA_CPU_ADDR(x)        (uint32_t*)(SHM_RELATIVE_ADDR(x) | PPCA_CPU_ADDR_MASK)
#endif


/******************************************************************************/
// Configuration of implementation mechanism

// CONFIG - Use double buffer mechanism for shared memory block access
#ifndef CONFIG_USE_DOUBLE_BUFFER
    #define CONFIG_USE_DOUBLE_BUFFER 0
#endif

// CONFIG - Use triple buffer mechanism for shared memory block access
#ifndef CONFIG_USE_TRIPLE_BUFFER
    #define CONFIG_USE_TRIPLE_BUFFER 0
#endif


/** \addtogroup group_shared_memory_config
 * \{
 */

/******************************************************************************/
// Configuration of use of memory pool.

/**
 * \brief CONFIG - Enable or disable the use of memory pool for dynamic allocation
 *
 * Configuration option to enable or disable the use of memory pool.
 *
 */
#ifndef CONFIG_USE_MEMORY_POOL
    #define CONFIG_USE_MEMORY_POOL 0
#endif

/**
 * \brief CONFIG - Define the total size of the shared memory pool in bytes
 *
 * Configuration option to set the total size of the shared memory pool in bytes.
 * This is applicable only when CONFIG_USE_MEMORY_POOL is set to 1.
 *
 */
#ifndef CONFIG_SHARED_MEM_POOL_SIZE
    #define CONFIG_SHARED_MEM_POOL_SIZE 4096
#endif


/******************************************************************************/
// Configuration of locking mechanism

/**
 * \brief CONFIG - Use IPC lock for shared memory block access
 *
 * Configuration option to enable or disable the use of IPC lock for shared memory block access.
 *
 * \note Only one of \c CONFIG_USE_IPC_LOCK or \c CONFIG_USE_MUTEX_LOCK should be enabled.
 */
#ifndef CONFIG_USE_IPC_LOCK
    #define CONFIG_USE_IPC_LOCK    0
#endif

/**
 * \brief CONFIG - Use mutex lock for shared memory block access
 *
 * Configuration option to enable or disable the use of mutex lock for shared memory block access.
 *
 * \note Only one of \c CONFIG_USE_IPC_LOCK or \c CONFIG_USE_MUTEX_LOCK should be enabled.
 */
#ifndef CONFIG_USE_MUTEX_LOCK
    #define CONFIG_USE_MUTEX_LOCK  1
#endif

/** \cond INTERNAL */
#define CONFIG_USE_LOCK      (CONFIG_USE_IPC_LOCK || CONFIG_USE_MUTEX_LOCK)
/** \endcond */

#if CONFIG_USE_IPC_LOCK
#include "cy_pdl.h"
#endif

#if CONFIG_USE_MUTEX_LOCK
#include "mutex.h"
#endif

/**
 * \brief CONFIG - Base lock number
 *
 * Configuration option to set the base lock number for shared memory block access.
 * For IPC Lock, this is the base IPC channel number used for locking.
 * For Mutex Lock, this is the base mutex index.
 */
#ifndef CONFIG_USE_LOCK_BASE_CHNL
    #if CONFIG_USE_IPC_LOCK
        #define CONFIG_USE_LOCK_BASE_CHNL CY_IPC_CHAN_USER
    #endif

    #if CONFIG_USE_MUTEX_LOCK
        #define CONFIG_USE_LOCK_BASE_CHNL 0
    #endif
#endif

/**
 * \brief CONFIG - Max number of locks
 *
 * Configuration option to set the maximum number of locks for shared memory block access.
 *
 */
#ifndef CONFIG_USE_LOCK_MAX_CHAL_NUM
    #define CONFIG_USE_LOCK_MAX_CHAL_NUM 4
#endif

/** \} group_shared_memory_config */


/** \addtogroup group_shared_memory_functions
 * \{
 */

/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_MemSet32
****************************************************************************//**
*
* \brief memset32 for shared memory
*        Only 32-bit aligned is supported across main core and PPCA bridge
*
* \param dest
* The pointer to the shared memory block.
*
* \param val
* The value to be set in the memory block.
*
* \param len
* The size of the data block.
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_MemSet32(volatile void* dest, uint32_t val, size_t len)
{
    volatile uint32_t* ptr = (volatile uint32_t*)dest;
    uint32_t len32 = (uint32_t)(len / sizeof(uint32_t));
    while (len32 > 0U)
    {
        *ptr = val;
        ptr++;
        len32--;
    }
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_MemCpy32
****************************************************************************//**
*
* \brief memcpy32 for shared memory
*        Only 32-bit aligned is supported across main core and PPCA bridge
*
* \param dest
* The pointer to the destination data block. May point to shared memory (volatile)
* or local memory.
*
* \param src
* The pointer to the source data block. May point to shared memory (volatile)
* or local memory.
*
* \param len
* The size of the data block.
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_MemCpy32(volatile void* dest, const volatile void* src, size_t len)
{
    volatile uint32_t* d = (volatile uint32_t*)dest;
    const volatile uint32_t* s = (const volatile uint32_t*)src;
    uint32_t len32 = (uint32_t)(len / sizeof(uint32_t));

    while (len32 > 0U)
    {
        *d = *s;
        d++;
        s++;
        len32--;
    }
}


/** \} group_shared_memory_functions */

#endif /* CY_PPCA_SHM_COMMON_H */
