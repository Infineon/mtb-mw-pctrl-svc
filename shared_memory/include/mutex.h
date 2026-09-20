/***************************************************************************//**
* \file mutex.h
* \brief
*   This file contains the inline function, macros and constants for mutex
*   implementation.
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
 * \defgroup group_mutex Mutex
 * \{
 * On multi-core system, inter-core synchronization ensures that different
 * processor cores can safely coordinate access to shared resources, such as
 * memory or peripherals, without conflicts or data corruption. Besides the IPC
 * hardware block available in MCU, lightweight mutexes based on atomic operations
 * are provided from this library to provide efficient and reliable synchronization
 * between cores.
 *
 * An atomic operation is a type of operation that completes in a single step
 * relative to other threads or cores. This means it cannot be interrupted or
 * observed in an incomplete state, ensuring data consistency when multiple
 * cores access shared resources.
 *
 * On ARM Cortex-M processors, atomic operations for inter-core synchronization
 * are typically implemented using exclusive load/store instructions (LDREX/STREX).
 * For atomic operations to work on multi-core systems, these instructions work
 * together with the "Global Exclusive Monitor" which is a hardware feature that
 * tracks exclusive accesses across all cores in a multi-core system.
 * - When a core executes LDREX, it marks the address as "exclusively accessed."
 * - If no other core writes to that address, a subsequent STREX by the same core will succeed, atomically updating the
 * value.
 * - If another core writes to the address in between, the STREX will fail, and the operation must be retried.
 *
 * Note that the address must be accessible by all cores, e.g located in a shared memory region
 *
 * By enabling the Global Exclusive Monitor, the system ensures that atomic
 * operations using LDREX/STREX are reliable across all cores, making them suitable
 * for implementing mutexes and other synchronization primitives in shared memory.
 *
 * \defgroup group_mutex_macros Macros
 * Macros for defining mutex states and operations.
 * \defgroup group_mutex_data_structures Data Structures
 * Data structures for mutex type definition.
 * \defgroup group_mutex_functions Functions
 * Functions for mutex operations such as lock and unlock.
 * \} */


#ifndef MUTEX_H
#define MUTEX_H

#include <stdbool.h>
#include <stdint.h>

#include "cmsis_compiler.h"
#include "cy_utils.h"

/* The CMSIS v2 definition of __CLREX uses an asm expression: (__asm volatile(...))
 * which IAR does not support in expression context (only as a statement).
 * Override with a statement-safe do-while form that still carries the memory
 * clobber (workaround for IAR EWARM-11901). */
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 20.5', 1, \
                             '#undef is required to replace CMSIS __CLREX with IAR-compatible form (EWARM-11901).')
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 21.1', 2, \
                             '__CLREX is a CMSIS-defined name that must be overridden for IAR compatibility.')
#if defined(__ICCARM__)
    #undef __CLREX
    #define __CLREX() do { __asm volatile ("CLREX" ::: "memory"); } while(0)
#endif
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 21.1')
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 20.5')


/** \addtogroup group_mutex_macros
 * \{
 */

/**
 * \brief Mutex locked state. Defined as 1 for efficient atomic operations.
 */
#define MUTEX_LOCKED   (1u)

/**
 * \brief Mutex unlocked state. Defined as 0 for efficient atomic operations.
 */
#define MUTEX_UNLOCKED (0u)

/**
 * \brief Enable global exclusive monitor.
 * \note This is required for the atomic operations to work across multiple cores.
 *       It's already enabled during system_init.
 */
#define SCB_ACB_ENABLE_EXTEXCLALL    SCnSCB->ACTLR |= (1UL << 29)

/** \} group_mutex_macros */


/** \addtogroup group_mutex_data_structures
 * \{
 */

/**
 * \brief Mutex type definition. Defined as uint32_t for efficient atomic operations with LDREX/STREX.
 */
typedef uint32_t mutex_t;

/** \} group_mutex_data_structures */



/** \addtogroup group_mutex_functions
 * \{
 */

/*******************************************************************************
* Function Name: mutex_enable
****************************************************************************//**
*
* \brief Enables the mutex functionality.
*
* This function enables the mutex functionality based on the core type.
*
*******************************************************************************/
__STATIC_INLINE void mutex_enable(void)
{
    /* For Cortex-M33 Enable global exclusive monitor. */
    #if defined(CY_DEVICE_PSC3_P8)
    SCnSCB->ACTLR |= (1UL << 29);
    #endif
}


CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 5.3', 2, \
                             'Parameter "mutex" intentionally matches the type name for API clarity; no ambiguity with the external linkage array in context.')
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 5.8', 2, \
                             'Parameter "mutex" is a local identifier that does not conflict with the external linkage array of the same name.')

/*******************************************************************************
* Function Name: mutex_lock
****************************************************************************//**
*
* \brief Locks the mutex.
*
* This function attempts to lock the mutex. If the mutex is already locked,
* it will return false. If the mutex is successfully locked, it will return true.
*
* \param mutex Pointer to the mutex variable. For multi-core systems, the mutex
*             variable must be in shared memory.
*
* \return true if the mutex was successfully locked.
* \return false if the mutex was already locked.
*
* \note  assembly code
*           mutex_lock:
*               ldrex   r3, [r0]
*               cbnz    r3, .L2
*               movs    r3, #1
*               strex   r2, r3, [r0]
*               cbnz    r2, .L4
*               dmb 0xF
*               mov     r0, r3
*               bx  lr
*           .L2:
*               clrex
*               bx  lr
*           .L4:
*               bx  lr
*
*******************************************************************************/
__STATIC_INLINE bool mutex_lock(volatile mutex_t* mutex)
{
    bool res = false;
    if (__LDREXW(mutex) == MUTEX_UNLOCKED)
    {
        /* mutex unlocked */
        if (__STREXW(MUTEX_LOCKED, mutex) == 0u)
        {
            res = true;
            __DMB();
        }
    }
    else
    {
        /* mutex locked */
        __CLREX();
    }
    return res;
}


/*******************************************************************************
* Function Name: mutex_unlock
****************************************************************************//**
*
* \brief Unlocks the mutex.
*
* This function unlocks the mutex by setting it to the unlocked state.
*
* \param mutex Pointer to the mutex variable. For multi-core systems, the mutex
*             variable must be in shared memory.
* \note assembly code
*           mutex_unlock:
*               dmb 0xF
*               movs    r3, #0
*               str r3, [r0]
*               bx  lr
*
*******************************************************************************/
__STATIC_INLINE void mutex_unlock(volatile mutex_t* mutex)
{
    __DMB();
    *mutex = MUTEX_UNLOCKED;
}


CY_MISRA_BLOCK_END('MISRA C-2012 Rule 5.8')
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 5.3')

/** \} group_mutex_functions */

#endif /*MUTEX_H*/

/* [] END OF FILE */
