/***************************************************************************//**
* \file ppca_wdt.h
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
/**
 * \defgroup group_ppca_watchdog PPCA Watchdog
 * \{
 * The PPCA watchdog service provides a mechanism to monitor the activity and
 * health of PPCA cores in a multi-core system. It uses the MCU's multi-counter
 * watchdog timer (MCWDT) to detect if a core becomes unresponsive or fails to
 * periodically reset its watchdog counter. If a core does not reset its counter
 * within a configured protection time, the watchdog can trigger an interrupt or
 * system reset, helping to ensure system reliability and recover from software
 * faults or hangs. The service includes APIs for initialization, reconfiguration,
 * status checking, and periodic resetting, and is typically managed by the main core.
 *
 * \defgroup group_ppca_watchdog_macros Macros
 * Macros for configuring the PPCA watchdog, such as hardware selection and timing parameters.
 * \defgroup group_ppca_watchdog_data_structures Data Structures
 * Data structures for PPCA watchdog configuration and status.
 * \defgroup group_ppca_watchdog_functions Functions
 * Functions for initializing, configuring, and servicing the PPCA watchdog.
 * \} */

#ifndef PPCA_WDT_H
#define PPCA_WDT_H

#include <stdint.h>
#include "cy_pdl.h"
#include "cy_ppca_shm.h"

/** \addtogroup group_ppca_watchdog_macros
 * \{
 */

/**
 * \brief PPCA Watchdog Hardware Used. Default to MCWDT_STRUCT0.
 */
#ifndef CONFIG_PPCA_WDT_HW
    #define CONFIG_PPCA_WDT_HW MCWDT_STRUCT0
#endif

/**
 * \brief PPCA Watchdog Enable Lock. Default to enabled to avoid accidental misconfiguration.
 */
#ifndef CONFIG_PPCA_WDT_ENABLE_LOCK
    #define CONFIG_PPCA_WDT_ENABLE_LOCK 1
#endif

/**
 * \brief ISR Priority. Default to 0 (highest priority).
 */
#ifndef CONFIG_PPCA_WDT_ISR_PRIORITY
    #define CONFIG_PPCA_WDT_ISR_PRIORITY 0
#endif

/**
 * \brief PPCA watchdog minimum interval.
 *  \note About PPCA watchdog time
 *
 *  \note MCWDT counter has a resolution of ~31 us, so the input time will be rounded up to the nearest count.
 *        MCWDT also require 2 count delay at every reset. So the "safe" timeout (usually 2x the service interval)
 *        is about 125us (4 counts).
 *
 *  \note Also note that, since only main core is checking the PPCA progress by monitoring the shared
 *        memory progress variables, PPCA should update these variables at faster rate to avoid missed services.
 *        e.g if timeout is 125us (4 counts), main core service at 62us (2 counts), PPCA update at 31us (1 count)
 *
 *  \note If PPCA cores only updates at half of timeout, then main core needs to run at faster rate (2x)
 *        e.g if timeout is 250us (8 counts), main core service at 62us (2 counts), PPCA update at 125us (4 counts)
 */
#define PPCA_WDT_TIME_INTERVAL_US   (31U)

/**
 * \brief Minimum watchdog feed interval in microseconds. Default to 62us (2 counts).
 *  \note This is the minimum recommended interval between watchdog resets (feeds) to ensure reliable operation.
 */
#define PPCA_WDT_MIN_FEED_US        (62U)        // 2 counts

/**
 * \brief Minimum watchdog time in microseconds. Default to 125us (4 counts).
 *  \note This is the minimum recommended watchdog timeout to ensure reliable operation, accounting for the
 *        MCWDT's 2-count delay at every reset and the minimum feed interval.
 */
#define PPCA_WDT_TIME_MIN_US        (125U)       // 4 counts

/**
 * \brief Maximum watchdog time in microseconds. Default to 2 seconds (65535 counts).
 *  \note The maximum timeout is determined by the MCWDT's 16-bit counter
 */
#define PPCA_WDT_TIME_MAX_US        (2000000U)    // 65535 counts

/**
 * \brief Convert watchdog time in us to Match count. Each count is ~31 us.
 */
#define PPCA_WDT_US_TO_COUNT(x)     ((x) * CY_SYSCLK_ILO_FREQ / 1000000U )

/**
 * \brief Convert watchdog count to time in us. Each count is ~31 us.
 */
#define PPCA_WDT_COUNT_TO_US(x)     ((x) * 1000000U / CY_SYSCLK_ILO_FREQ)

/** \} group_ppca_watchdog_macros */


/* Shared Variables  */
extern CY_ALIGN(4) volatile uint32_t CORE0_WATCH;
extern CY_ALIGN(4) volatile uint32_t CORE1_WATCH;
extern CY_ALIGN(4) volatile uint32_t PREV_CORE0_WATCH;
extern CY_ALIGN(4) volatile uint32_t PREV_CORE1_WATCH;
extern CY_ALIGN(4) volatile uint32_t WDT_RESET_STATUS;
extern CY_ALIGN(4) volatile uint32_t CORE0_WDT_RESET_COUNT;
extern CY_ALIGN(4) volatile uint32_t CORE1_WDT_RESET_COUNT;


/** \addtogroup group_ppca_watchdog_data_structures
 * \{
 */

/**
 * \brief Watchdog interrupt callback type
 *  \param reset_count: Consecutive WDT interrupt count for the core in INT_RESET mode.
 *  \note For other modes, this value is 0.
 */
typedef void (* ppca_wdt_cb_t)(uint32_t reset_count);


/**
 * \brief Reset core selector
 *  \note Matches MCWDT interrupt status bits (CTR0/CTR1).
 */
typedef enum
{
    PPCA_WDT_RESET_NONE      = 0x0, /**< No reset */
    PPCA_WDT_RESET_CORE_0    = 0x1, /**< Reset core 0 */
    PPCA_WDT_RESET_CORE_1    = 0x2, /**< Reset core 1 */
    PPCA_WDT_RESET_CORE_BOTH = 0x3  /**< Reset both cores */
} ppca_wdt_reset_t;

/**
 * \brief PPCA watchdog modes
 * \note Direct mapping to MCWDT modes with PPCA-specific naming.
 */
typedef enum
{
    PPCA_WDT_MODE_NONE,      /**< No action (disabled). */
    PPCA_WDT_MODE_INT,       /**< Interrupt mode. */
    PPCA_WDT_MODE_RESET,     /**< Reset mode. */
    PPCA_WDT_MODE_INT_RESET  /**< Interrupt for first 2 matches, reset on 3rd if not cleared. */
} ppca_wdt_mode_t;

/** \} group_ppca_watchdog_data_structures */


#if !defined(COMPONENT_PPCA_DEVICE)

extern ppca_wdt_mode_t  ppca_wdt_core0_mode;
extern ppca_wdt_mode_t  ppca_wdt_core1_mode;
extern ppca_wdt_cb_t    ppca_wdt_core0_cb;
extern ppca_wdt_cb_t    ppca_wdt_core1_cb;


/** \addtogroup group_ppca_watchdog_functions
 * \{
 */

/*******************************************************************************
* Function Name: ppca_wdt_init
****************************************************************************//**
*
* \brief Initialize ppca watchdog timer.
*
* \param core0_protection_time: Protection time for core0 in us. Set to 0 to disable.
* \param core1_protection_time: Protection time for core1 in us. Set to 0 to disable.
* \param core0_wdt_mode: Watchdog mode for core0.
* \param core1_wdt_mode: Watchdog mode for core1.
* \param core0_cb: Callback executed on Core0 WDT interrupt. Can be NULL.
* \param core1_cb: Callback executed on Core1 WDT interrupt. Can be NULL.
*
* \note This function should be called from main core only.
* \note In INT_RESET mode, the handler clears the interrupt for the first two
*       occurrences and allows reset on the third.
*
*******************************************************************************/
void ppca_wdt_init(uint32_t core0_protection_time, ppca_wdt_mode_t core0_wdt_mode, ppca_wdt_cb_t core0_cb,
                   uint32_t core1_protection_time, ppca_wdt_mode_t core1_wdt_mode, ppca_wdt_cb_t core1_cb);


/*******************************************************************************
* Function Name: ppca_wdt_enable
****************************************************************************//**
*
* \brief Enable ppca watchdog.
*
* \note This function should be called from main core only.
*
*******************************************************************************/
void ppca_wdt_enable(void);


/*******************************************************************************
* Function Name: ppca_wdt_reconfigure
****************************************************************************//**
*
* \brief Reconfigure ppca watchdog timer
*
* \param ppca_core: PPCA core to be reconfigured
* \param protection_time: Protection time in us. Set to 0 to disable.
*
* \note This function should be called from main core only.
*
*******************************************************************************/
void ppca_wdt_reconfigure(cy_en_ppca_core_t ppca_core, uint32_t protection_time);


/*******************************************************************************
* Function Name: ppca_wdt_check_restart_reason
****************************************************************************//**
*
* \brief Check the reason for device restart
*
* \return Reset reason derived from WDT interrupt status.
*
* \note This function should be called from main core only.
*
*******************************************************************************/
ppca_wdt_reset_t ppca_wdt_check_restart_reason(void);


/*******************************************************************************
* Function Name: ppca_wdt_clear_restart_reason
****************************************************************************//**
*
* \brief Clear the reason for device restart
*
* \note This function should be called from main core only.
*
*******************************************************************************/
void ppca_wdt_clear_restart_reason(void);


/*******************************************************************************
* Function Name: ppca_wdt_handler
****************************************************************************//**
*
* \brief PPCA watchdog timer interrupt handler
*
* \note In INT_RESET mode, the interrupt is cleared for the first two
*       occurrences only. The third occurrence is left uncleared so the
*       hardware can reset the device.
*
* \note This function should be called from main core only.
*
*******************************************************************************/
void ppca_wdt_handler(void);

/** \} group_ppca_watchdog_functions */

#endif // if !defined(COMPONENT_PPCA_DEVICE)


/** \addtogroup group_ppca_watchdog_functions
 * \{
 */

/*******************************************************************************
* Function Name: ppca_wdt_reset
****************************************************************************//**
*
* \brief Reset watchdog timer. This requires periodic execution.

* \note This function should be called from all cores
*
*******************************************************************************/
void ppca_wdt_reset(void);

/** \} group_ppca_watchdog_functions */

#endif /* PPCA_WDT_H */
