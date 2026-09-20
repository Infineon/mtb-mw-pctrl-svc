/***************************************************************************//**
* \file stack_mon.c
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

#include "stack_mon.h"

// stack monitor instance
static cy_stc_stack_mon_t stack_monitor;


// initialize stack_mon
void svc_stack_mon_init(void)
{
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.3', \
                          'Linker symbol cast: unsigned int* to uint32_t* is safe on 32-bit ARM.');
    volatile uint32_t* limit = STACK_LIMIT;
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.3', \
                          'Linker symbol cast: unsigned int* to uint32_t* is safe on 32-bit ARM.');
    volatile uint32_t* top = STACK_TOP;

    /* Place guard word at the bottom of the stack */
    *limit = STACK_GUARD_WORD;

    /* Fill the rest of the stack with canary */
    CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 18.1', 2, \
                                 '__StackLimit is a linker symbol marking the start of a contiguous stack region [__StackLimit, __INITIAL_SP), not a singleton object.');
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 18.3', \
                          'limit and top both point into the same linker-defined stack region [__StackLimit, __INITIAL_SP).');
    for (volatile uint32_t* ptr = limit + 1; ptr < top; ptr++)
    {
        *ptr = STACK_FILL_WORD;
    }
    CY_MISRA_BLOCK_END('MISRA C-2012 Rule 18.1')

    /* Initialize stack monitor structure */
    stack_monitor.limit = limit;
    stack_monitor.top = top;
    #if STACK_MONITORING_SAVE_SCAN_PTR
    stack_monitor.scan_ptr = limit + 1;
    #endif
    stack_monitor.highwater = 0;
}


size_t svc_stack_mon_get_total(void)
{
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 10.8', \
                          'Intentional cast of signed ptr difference composite to size_t for stack size calculation.');
    return (size_t)(stack_monitor.top - stack_monitor.limit) * sizeof(uint32_t);
}


// update stack_mon and return highwater mark size (not address)
size_t svc_stack_mon_update(void)
{
    #if STACK_MONITORING_SAVE_SCAN_PTR
    volatile uint32_t* ptr = stack_monitor.scan_ptr;
    #else
    volatile uint32_t* ptr = stack_monitor.limit + 1;
    #endif
    volatile uint32_t* top = stack_monitor.top;

    /* Scan the stack for used space */
    while (ptr < top)
    {
        if (*ptr != STACK_FILL_WORD)
        {
            break;
        }
        ptr++;
    }

    /* Do a skip check in case some stack space is untouched */
    #if STACK_MONITORING_SAVE_SCAN_PTR
    ptr -= STACK_SCAN_SKIP;
    if (ptr < stack_monitor.limit + 1)
    {
        ptr = stack_monitor.limit + 1;
    }

    /* Update scan pointer for next call */
    stack_monitor.scan_ptr = ptr;
    #endif
    /* Update used space  */
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 10.8', \
                          'Intentional cast of signed ptr difference composite to size_t for stack usage calculation.');
    size_t used_space = (size_t)(top - ptr) * sizeof(uint32_t);

    /* Update highwater mark if needed */
    if (used_space > stack_monitor.highwater)
    {
        stack_monitor.highwater = used_space;
    }

    return stack_monitor.highwater;
}


// stack smashing protection

// for ARM Compiler
// if stack smashing protection is enabled in build, these MACROs are defined:
//  -fstack-protector ==> __SSP__
//  -fstack-protector-all ==> __SSP_ALL__
//  -fstack-protector-strong ==> __SSP_STRONG__
// Provide stack smashing protection related functions here
#if defined (__ARMCC_VERSION)

#if defined(__SSP__) || defined(__SSP_ALL__) || defined(__SSP_STRONG__)

__attribute__((weak))
uintptr_t __stack_chk_guard = STACK_GUARD_WORD;

__attribute__((weak, noreturn))
void __stack_chk_fail(void)
{
    /* Stop interrupts to prevent further system instability */
    __asm volatile ("cpsid i");

    /* If a debugger is attached, trigger a breakpoint */
    __asm volatile ("bkpt #0");

    /* Infinite loop to prevent returning to corrupted code */
    while (true)
    {
        /* User code */
    }
}


#endif // if defined(__SSP__) || defined(__SSP_ALL__) || defined(__SSP_STRONG__)


// for GCC
// if stack smashing protection is enabled in build, these MACROs are defined:
//  -fstack-protector ==> __SSP__
//  -fstack-protector-all ==> __SSP_ALL__
//  -fstack-protector-strong ==> __SSP_STRONG__
//  Already supported by nanolib (default)
#elif defined (__GNUC__)


// for IAR compiler (supported from version 8.10.0),
// Provide stack smashing protection related functions
#elif defined (__ICCARM__) && (__VER__ >= 8010000)

/* Use __weak keyword for IAR instead of __attribute__((weak)) */
__weak uint32_t __stack_chk_guard = STACK_GUARD_WORD;

/* IAR uses __noreturn or __stack_chk_fail depending on version */
__weak void __stack_chk_fail(void)
{
    /* Stop interrupts (IAR intrinsic) */
    __disable_irq();

    /* Optional: Breakpoint for IAR C-SPY debugger */
    __asm("BKPT #0");

    while (true)
    {
    }
}


// unsupported compiler
#else // if defined (__ARMCC_VERSION)
  #error Unsupported compiler.
#endif // if defined (__ARMCC_VERSION)
