/***************************************************************************//**
* \file ppca_wdt.c
* \brief
*   This file contains the inline function, macros and constants for ppca
*   watchdog services using mcwdt
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


#include "ppca_wdt.h"
#include "cmsis_compiler.h"


/* The function Cy_MCWDT_Enable() waits for some delay in microseconds before
 * returning */
#define MCWDT_0_ENABLE_DELAY                (93u)


#if !defined(COMPONENT_PPCA_DEVICE)

ppca_wdt_mode_t  ppca_wdt_core0_mode = PPCA_WDT_MODE_NONE;
ppca_wdt_mode_t  ppca_wdt_core1_mode = PPCA_WDT_MODE_NONE;
ppca_wdt_cb_t    ppca_wdt_core0_cb = NULL;
ppca_wdt_cb_t    ppca_wdt_core1_cb = NULL;

/*******************************************************************************
* Function Name: ppca_wdt_init
*******************************************************************************/
void ppca_wdt_init(uint32_t core0_protection_time, ppca_wdt_mode_t core0_wdt_mode, ppca_wdt_cb_t core0_cb,
                   uint32_t core1_protection_time, ppca_wdt_mode_t core1_wdt_mode, ppca_wdt_cb_t core1_cb)
{
    if (!((core0_protection_time == 0U) ||
          ((core0_protection_time < PPCA_WDT_TIME_MAX_US) && (core0_protection_time >= PPCA_WDT_TIME_MIN_US))))
    {
        CY_ASSERT_HANDLER();
    }
    if (!((core1_protection_time == 0U) ||
          ((core1_protection_time < PPCA_WDT_TIME_MAX_US) && (core1_protection_time >= PPCA_WDT_TIME_MIN_US))))
    {
        CY_ASSERT_HANDLER();
    }

    // 0 protection time will default to MODE_NONE
    ppca_wdt_core0_mode = (core0_protection_time == 0U) ? PPCA_WDT_MODE_NONE : core0_wdt_mode;
    ppca_wdt_core1_mode = (core1_protection_time == 0U) ? PPCA_WDT_MODE_NONE : core1_wdt_mode;
    ppca_wdt_core0_cb = core0_cb;
    ppca_wdt_core1_cb = core1_cb;

    // Initialize MCWDT interrupt
    cy_stc_sysint_t svc_mcwdt_sysint =
    {
        .intrSrc      = srss_interrupt_mcwdt_0_IRQn,
        .intrPriority = CONFIG_PPCA_WDT_ISR_PRIORITY
    };
    (void)Cy_SysInt_Init(&svc_mcwdt_sysint, &ppca_wdt_handler);
    NVIC_EnableIRQ(srss_interrupt_mcwdt_0_IRQn);


    // This structure initializes mcwdt
    const cy_stc_mcwdt_config_t svc_mcwdt_config =
    {
        .c0Match          = (uint16_t)PPCA_WDT_US_TO_COUNT(core0_protection_time),
        .c1Match          = (uint16_t)PPCA_WDT_US_TO_COUNT(core1_protection_time),
        .c0Mode           = (uint8_t)(cy_en_mcwdtmode_t)ppca_wdt_core0_mode,
        .c1Mode           = (uint8_t)(cy_en_mcwdtmode_t)ppca_wdt_core1_mode,
        .c2ToggleBit      = 16U,
        .c2Mode           = (uint8_t)CY_MCWDT_MODE_NONE,
        .c0ClearOnMatch   = false,
        .c1ClearOnMatch   = false,
        .c0c1Cascade      = false,
        .c1c2Cascade      = false,
        .c0LowerLimitMode = (uint16_t)CY_MCWDT_LOWER_LIMIT_MODE_NOTHING,
        .c0LowerLimit     = 0U,
        .c1LowerLimitMode = (uint16_t)CY_MCWDT_LOWER_LIMIT_MODE_NOTHING,
        .c1LowerLimit     = 0U,
    };
    #if CONFIG_PPCA_WDT_ENABLE_LOCK
    Cy_MCWDT_Unlock(CONFIG_PPCA_WDT_HW);
    #endif
    (void)Cy_MCWDT_Init(CONFIG_PPCA_WDT_HW, &svc_mcwdt_config);
    Cy_MCWDT_ClearInterrupt(CONFIG_PPCA_WDT_HW, CY_MCWDT_CTR0|CY_MCWDT_CTR1);
    Cy_MCWDT_SetInterruptMask(CONFIG_PPCA_WDT_HW, CY_MCWDT_CTR0|CY_MCWDT_CTR1);
    #if CONFIG_PPCA_WDT_ENABLE_LOCK
    Cy_MCWDT_Lock(CONFIG_PPCA_WDT_HW);
    #endif /* Clear internal variable */
    CORE0_WATCH = 0U;
    CORE1_WATCH = 0U;
    PREV_CORE0_WATCH = CORE0_WATCH;
    PREV_CORE1_WATCH = CORE1_WATCH;
    WDT_RESET_STATUS = 0U;
    CORE0_WDT_RESET_COUNT = 0U;
    CORE1_WDT_RESET_COUNT = 0U;
}


/*******************************************************************************
* Function Name: ppca_wdt_enable
*******************************************************************************/
void ppca_wdt_enable(void)
{
    #if CONFIG_PPCA_WDT_ENABLE_LOCK
    Cy_MCWDT_Unlock(CONFIG_PPCA_WDT_HW);
    #endif
    Cy_MCWDT_Enable(CONFIG_PPCA_WDT_HW, CY_MCWDT_CTR0|CY_MCWDT_CTR1, MCWDT_0_ENABLE_DELAY);
    #if CONFIG_PPCA_WDT_ENABLE_LOCK
    Cy_MCWDT_Lock(CONFIG_PPCA_WDT_HW);
    #endif
}


/*******************************************************************************
* Function Name: ppca_wdt_reconfigure
*******************************************************************************/
void ppca_wdt_reconfigure(cy_en_ppca_core_t ppca_core, uint32_t protection_time)
{
    if (ppca_core > CY_PPCA_CORE_1)
    {
        CY_ASSERT_HANDLER();
    }
    if (!((protection_time == 0U) ||
          ((protection_time < PPCA_WDT_TIME_MAX_US) && (protection_time >= PPCA_WDT_TIME_MIN_US))))
    {
        CY_ASSERT_HANDLER();
    }

    #if CONFIG_PPCA_WDT_ENABLE_LOCK
    Cy_MCWDT_Unlock(CONFIG_PPCA_WDT_HW);
    #endif
    if (ppca_core == CY_PPCA_CORE_0)
    {
        ppca_wdt_core0_mode = (protection_time == 0U) ? PPCA_WDT_MODE_NONE : ppca_wdt_core0_mode;
        Cy_MCWDT_SetMode(CONFIG_PPCA_WDT_HW, CY_MCWDT_COUNTER0, (cy_en_mcwdtmode_t)ppca_wdt_core0_mode);
        Cy_MCWDT_SetMatch(CONFIG_PPCA_WDT_HW, CY_MCWDT_COUNTER0, PPCA_WDT_US_TO_COUNT(protection_time),
                          MCWDT_0_ENABLE_DELAY);
    }
    else if (ppca_core == CY_PPCA_CORE_1)
    {
        ppca_wdt_core1_mode = (protection_time == 0U) ? PPCA_WDT_MODE_NONE : ppca_wdt_core1_mode;
        Cy_MCWDT_SetMode(CONFIG_PPCA_WDT_HW, CY_MCWDT_COUNTER1, (cy_en_mcwdtmode_t)ppca_wdt_core1_mode);
        Cy_MCWDT_SetMatch(CONFIG_PPCA_WDT_HW, CY_MCWDT_COUNTER1, PPCA_WDT_US_TO_COUNT(protection_time),
                          MCWDT_0_ENABLE_DELAY);
    }
    else
    {
        /* Do nothing */
    }
    #if CONFIG_PPCA_WDT_ENABLE_LOCK
    Cy_MCWDT_Lock(CONFIG_PPCA_WDT_HW);
    #endif
}


/*******************************************************************************
* Function Name: ppca_wdt_handler
*******************************************************************************/
__WEAK void ppca_wdt_handler(void)
{
    WDT_RESET_STATUS = Cy_MCWDT_GetInterruptStatus(CONFIG_PPCA_WDT_HW);

    /* PPCA 0 */
    if ((WDT_RESET_STATUS & CY_MCWDT_CTR0) != 0U)
    {
        uint32_t core0_count = CORE0_WDT_RESET_COUNT + 1U;
        CORE0_WDT_RESET_COUNT = core0_count;
        if (ppca_wdt_core0_mode == PPCA_WDT_MODE_INT_RESET)
        {
            if (core0_count < 3U)
            {
                /* Clear interrupt before RESET */
                Cy_MCWDT_ClearInterrupt(CONFIG_PPCA_WDT_HW, CY_MCWDT_CTR0);
                if (ppca_wdt_core0_cb != NULL)
                {
                    ppca_wdt_core0_cb(core0_count);
                }
            }
            else
            {
                /* Avoid interrupt storm */
                uint32_t mask = Cy_MCWDT_GetInterruptMask(CONFIG_PPCA_WDT_HW);
                mask &= (uint32_t)(~CY_MCWDT_CTR0);
                Cy_MCWDT_SetInterruptMask(CONFIG_PPCA_WDT_HW, mask);
            }
        }
        else if (ppca_wdt_core0_mode == PPCA_WDT_MODE_INT)
        {
            if (ppca_wdt_core0_cb != NULL)
            {
                ppca_wdt_core0_cb(core0_count);
            }
            Cy_MCWDT_ClearInterrupt(CONFIG_PPCA_WDT_HW, CY_MCWDT_CTR0);
        }
        else
        {
            /* Other modes (NONE, RESET): no interrupt handling required */
        }
    }

    /* PPCA 1 */
    if ((WDT_RESET_STATUS & CY_MCWDT_CTR1) != 0U)
    {
        uint32_t core1_count = CORE1_WDT_RESET_COUNT + 1U;
        CORE1_WDT_RESET_COUNT = core1_count;
        if (ppca_wdt_core1_mode == PPCA_WDT_MODE_INT_RESET)
        {
            if (core1_count < 3U)
            {
                /* Clear interrupt before RESET */
                Cy_MCWDT_ClearInterrupt(CONFIG_PPCA_WDT_HW, CY_MCWDT_CTR1);
                if (ppca_wdt_core1_cb != NULL)
                {
                    ppca_wdt_core1_cb(core1_count);
                }
            }
            else
            {
                /* Avoid interrupt storm */
                uint32_t mask = Cy_MCWDT_GetInterruptMask(CONFIG_PPCA_WDT_HW);
                mask &= (uint32_t)(~CY_MCWDT_CTR1);
                Cy_MCWDT_SetInterruptMask(CONFIG_PPCA_WDT_HW, mask);
            }
        }
        else if (ppca_wdt_core1_mode == PPCA_WDT_MODE_INT)
        {
            if (ppca_wdt_core1_cb != NULL)
            {
                ppca_wdt_core1_cb(core1_count);
            }
            Cy_MCWDT_ClearInterrupt(CONFIG_PPCA_WDT_HW, CY_MCWDT_CTR1);
        }
        else
        {
            /* Other modes (NONE, RESET): no interrupt handling required */
        }
    }
}


/*******************************************************************************
* Function Name: ppca_wdt_check_restart_reason
*******************************************************************************/
ppca_wdt_reset_t ppca_wdt_check_restart_reason(void)
{
    /* Check the reason for device restart */
    if (CY_SYSLIB_RESET_SWWDT0 == Cy_SysLib_GetResetReason())
    {
        return (ppca_wdt_reset_t)WDT_RESET_STATUS;
    }
    return PPCA_WDT_RESET_NONE;
}


/*******************************************************************************
* Function Name: ppca_wdt_clear_restart_reason
*******************************************************************************/
void ppca_wdt_clear_restart_reason(void)
{
    /* Clears the reset cause register */
    Cy_SysLib_ClearResetReason();
}


#endif /* COMPONENT_PPCA_DEVICE */


/*******************************************************************************
* Function Name: ppca_wdt_reset
*******************************************************************************/
void ppca_wdt_reset(void)
{
    /* PPCA 0 core */
    #if defined (COMPONENT_PPCA_DEVICE) && defined (CY_PDL_PPCA_CORE0)
    uint32_t local_core0_watch = CORE0_WATCH;
    CORE0_WATCH = local_core0_watch + 1U;
    return;
    #endif

    /* PPCA 1 core */
    #if defined (COMPONENT_PPCA_DEVICE) && defined (CY_PDL_PPCA_CORE1)
    uint32_t local_core1_watch = CORE1_WATCH;
    CORE1_WATCH = local_core1_watch + 1U;
    return;
    #endif

    /* Main Core */
    #if !defined(COMPONENT_PPCA_DEVICE)
    uint32_t counters = 0U;
    uint32_t resetCounters = 0U;

    uint32_t local_cur0 = CORE0_WATCH;
    uint32_t local_prev0 = PREV_CORE0_WATCH;
    if (local_cur0 != local_prev0)
    {
        PREV_CORE0_WATCH = local_cur0;
        counters |= CY_MCWDT_CTR0;
        CORE0_WDT_RESET_COUNT = 0U;
    }

    uint32_t local_cur1 = CORE1_WATCH;
    uint32_t local_prev1 = PREV_CORE1_WATCH;
    if (local_cur1 != local_prev1)
    {
        PREV_CORE1_WATCH = local_cur1;
        counters |= CY_MCWDT_CTR1;
        CORE1_WDT_RESET_COUNT = 0U;
    }

    // Cy_MCWDT_ResetCounters(CONFIG_PPCA_WDT_HW, counters, MCWDT_0_ENABLE_DELAY);
    resetCounters = ((0UL != (counters & CY_MCWDT_CTR0)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_RESET0_Msk : 0UL) |
                    ((0UL != (counters & CY_MCWDT_CTR1)) ? MCWDT_STRUCT_MCWDT_CTL_WDT_RESET1_Msk : 0UL);
    #if CONFIG_PPCA_WDT_ENABLE_LOCK
    Cy_MCWDT_Unlock(CONFIG_PPCA_WDT_HW);
    #endif
    /* Reset occurs after one lf_clk cycle passes. The recommended wait time is 62 us. */
    MCWDT_CTL(CONFIG_PPCA_WDT_HW) |= resetCounters;
    #if CONFIG_PPCA_WDT_ENABLE_LOCK
    Cy_MCWDT_Lock(CONFIG_PPCA_WDT_HW);
    #endif

    #endif // if !defined(COMPONENT_PPCA_DEVICE)
}
