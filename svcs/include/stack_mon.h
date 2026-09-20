/***************************************************************************//**
* \file stack_mon.h
* \brief
*   This file contains the common definition of the stack monitor functions.
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
 * \defgroup group_stack_monitor Stack Monitor
 * \{
 * The monitor places a guard word at the stack limit and fills the remaining
 * stack region with \c STACK_FILL_WORD. The update routine scans the region to
 * estimate the maximum stack usage (high-water mark).
 *
 * This method measures the deepest stack region that has been *written*.
 * If the compiler reserves stack space but does not write to it, the scan may
 * under-report usage.
 *
 * \defgroup group_stack_monitor_macros Macros
 * Macros for configuring the stack monitor, such as fill patterns and scan options.
 * \defgroup group_stack_monitor_functions Functions
 * Functions for initializing the stack monitor and updating/reading the high-water mark.
 * \} */

#ifndef STACK_MON_H
#define STACK_MON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cmsis_compiler.h"
#include "cy_utils.h"

/** Stack symbols from the linker script.
 *  Declared as arrays so the compiler treats them as address labels
 *  rather than singleton objects this avoids Coverity ARRAY_VS_SINGLETON
 *  and MISRA 18.1/18.3 false positives when doing pointer arithmetic within
 *  the linker-defined stack region. */
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 5.8', 2, \
                             'Linker-provided symbols __INITIAL_SP and __STACK_LIMIT must be redeclared in C to reference them; the duplicate external linkage is inherent and intentional.')
extern uint32_t __INITIAL_SP[];
extern uint32_t __STACK_LIMIT[];
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 5.8')

/** \addtogroup group_stack_monitor_macros
 * \{
 */

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 11.3', 2, \
                             'Linker symbols casting is safe on this 32-bit ARM target where both types are identical')
/**
 * \brief Stack top address (highest address of the stack region).
 *        Defined as the initial stack pointer value from the linker script.
 */
#define STACK_TOP           ((uint32_t*)&__INITIAL_SP)

/**
 * \brief Stack limit address (lowest address of the stack region).
 *        Defined as the stack limit value from the linker script.
 */
#define STACK_LIMIT         ((uint32_t*)&__STACK_LIMIT)

CY_MISRA_BLOCK_END('MISRA C-2012 Rule 11.3')

/**
 * \brief 32-bit fill pattern written to the stack region.
 *        Used to initialize the stack and detect high-water mark.
 */
#define STACK_FILL_WORD     0xA5A5A5A5U

/**
 * \brief Guard word placed at \c STACK_LIMIT to detect overflow.
 *        This value is checked to detect stack overflows.
 */
#define STACK_GUARD_WORD    0xDEADBEEFU

/**
 * \brief Enable saving a scan pointer between updates.
 *
 * When enabled, \c svc_stack_mon_update uses the previous scan position
 * (with an optional back-step via \c STACK_SCAN_SKIP ) to reduce scan time.
 *
 * \note
 * The scan still assumes that used stack forms a contiguous region near
 * \c STACK_TOP.
 */
#define STACK_MONITORING_SAVE_SCAN_PTR  0
#if STACK_MONITORING_SAVE_SCAN_PTR
/** Number of 32-bit words to back up before scanning again. */
    #define STACK_SCAN_SKIP     256
#endif

/** \} group_stack_monitor_macros */



/** \cond INTERNAL */
/* Stack monitor internal state. */
typedef struct
{
    /** Stack limit address (guard word location). */
    volatile uint32_t* limit;
    /** Stack top address. */
    volatile uint32_t* top;
    #if STACK_MONITORING_SAVE_SCAN_PTR
    /** Scan start pointer (used only if \c STACK_MONITORING_SAVE_SCAN_PTR is enabled). */
    volatile uint32_t* scan_ptr;
    #endif
    /** Highest observed stack usage in bytes. */
    size_t highwater;
} cy_stc_stack_mon_t;
/** \endcond */


/** \addtogroup group_stack_monitor_functions
 * \{
 */

/*******************************************************************************
 * Function Name: svc_stack_mon_init
 ***************************************************************************//**
 * \brief Initialize the stack monitor.
 *
 * \details
 * Writes \c STACK_GUARD_WORD at \c STACK_LIMIT and fills the range
 * `[STACK_LIMIT + 1, STACK_TOP)` with \c STACK_FILL_WORD. This should be
 * called once early at boot.
 *
 * \warning
 * Calling this after the stack has already been used will overwrite stack
 * memory.
 */
void svc_stack_mon_init(void);


/*******************************************************************************
 * Function Name: svc_stack_mon_get_total
 ***************************************************************************//**
 * \brief Get the total stack size monitored.
 *
 * \return Total monitored stack size in bytes.
 *
 * \note
 * The returned value is based on the linker symbols \c __StackTop and
 * \c __StackLimit.
 */
size_t svc_stack_mon_get_total(void);


/*******************************************************************************
 * Function Name: svc_stack_mon_update
 ***************************************************************************//**
 * \brief Update and return the stack high-water mark.
 *
 * \details
 * Scans the stack region for the first word that differs from
 * \c STACK_FILL_WORD and updates the internally stored high-water mark.
 *
 * \return Highest observed stack usage (high-water) in bytes.
 *
 * \note
 * This estimate may under-report stack usage if reserved stack space is never
 * written.
 */
size_t svc_stack_mon_update(void);

/** \} group_stack_monitor_functions */

#endif /* STACK_MON_H */
