/***************************************************************************//**
* \file dwt_profiling.h
* \brief
*   This file contains the inline function, macros and constants for profiling
*   implementation using DWT block in Cortex-M cores.
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
 * \defgroup group_profiling Profiling
 * \{
 * The profiling interfaces leverages the built-in DWT (Data Watchpoint and Trace)
 * unit in ARM Cortex-M processor cores to measure code execution time with high
 * precision. By enabling the DWT cycle counter, developers can record the number
 * of CPU cycles elapsed between specific code sections, allowing accurate
 * performance analysis. This method is lightweight and does not require external
 * tools or significant software overhead.
 *
 * \defgroup group_profiling_functions Functions
 * Functions for initializing the DWT, starting and stopping profiling, and reading results.
 * \} */

#ifndef DWT_PROFILING_H
#define DWT_PROFILING_H

#include <stdint.h>

#include "cmsis_compiler.h"
#include "cy_device.h"


/** Number of clock cycles overhead introduced by the profiling wrapper functions. */
#define PPCA_CORE_OFFSET 6U


/** Captured DWT cycle count at profiling start. Set by \ref svc_dwt_profile_start. */
extern uint32_t dwt_start_count;
/** Captured DWT cycle count at profiling stop. Set by \ref svc_dwt_profile_stop. */
extern uint32_t dwt_stop_count;


/** \addtogroup group_profiling_functions
 * \{
 */

/*******************************************************************************
* Function Name: svc_dwt_profile_init
****************************************************************************//**
*
* \brief Initialize the profiling using DWT.
*
* \details
* Enables the DWT cycle counter (CYCCNT) and resets it to zero.
*
*******************************************************************************/
__STATIC_INLINE void svc_dwt_profile_init(void)
{
    /* Enable DWT */
    DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* Main Core */
    #if defined(CY_DEVICE_PSC3_P8) && !defined(COMPONENT_PPCA_DEVICE)
    PPCA->CPUSS_CNFG.AP_CTL |=
        PPCA_CPUSS_CNFG_AP_CTL_CM33_0_NID_ENABLE_Msk |
        PPCA_CPUSS_CNFG_AP_CTL_CM33_1_NID_ENABLE_Msk;
    #endif // if !defined(COMPONENT_PPCA_DEVICE)
}


/*******************************************************************************
* Function Name: svc_dwt_profile_start
****************************************************************************//**
*
* \brief Start the profiling.
*
* \note assembly code
*   svc_dwt_profile_start:
*       dsb 0xF
*       isb 0xF
*       ldr r3, .L5
*       ldr r2, [r3, #4]
*       ldr r3, .L5+4
*       str r2, [r3]
*       bx  lr
*   .L5:
*       .word   -536866816
*       .word   dwt_start_count
*
*******************************************************************************/
__STATIC_INLINE void svc_dwt_profile_start(void)
{
    __DSB();
    __ISB();
    dwt_start_count = DWT->CYCCNT;
}


/*******************************************************************************
* Function Name: svc_dwt_profile_stop
****************************************************************************//**
*
* \brief Stop the profiling.
*
* \note assembly code
*   svc_dwt_profile_stop:
*       ldr r3, .L8
*       ldr r2, [r3, #4]
*       ldr r3, .L8+4
*       str r2, [r3]
*       dsb 0xF
*       isb 0xF
*       bx  lr
*   .L8:
*       .word   -536866816
*       .word   dwt_stop_count
*
*******************************************************************************/
__STATIC_INLINE void svc_dwt_profile_stop(void)
{
    dwt_stop_count = DWT->CYCCNT;
    __DSB();
    __ISB();
}


/*******************************************************************************
* Function Name: svc_dwt_profile_reset_cyccnt
****************************************************************************//**
*
* \brief Reset the cycle count. Clear CYCCNT to zero without disabling the counter.
*
*******************************************************************************/
__STATIC_INLINE void svc_dwt_profile_reset_cyccnt(void)
{
    DWT->CYCCNT = 0U;
}


/*******************************************************************************
* Function Name: svc_dwt_profile_read
****************************************************************************//**
*
* \brief Read the cycle count.
*
* \return The cycle count in core clock cycles
*
*******************************************************************************/
__STATIC_INLINE uint32_t svc_dwt_profile_read(void)
{
    /* Read the tick count and convert to us */
    uint32_t tick = dwt_stop_count - dwt_start_count - PPCA_CORE_OFFSET;

    return tick;
}


/** \} group_profiling_functions */

#endif /* DWT_PROFILING_H */
