/***************************************************************************//**
* \file basic_sched.h
* \brief
*   This file contains the common definition of the basic scheduler functions.
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
 * \defgroup group_basic_sched Basic Scheduler
 * \{
 * A lightweight cooperative task scheduler driven by a periodic tick (typically
 * SysTick). Tasks run in the main loop via \c svc_sched_dispatch.
 *
 * Features:
 * - Periodic and one-shot tasks
 * - Simple priority (lower value = higher priority)
 * - Small fixed-size task table (\c SVC_SCHED_MAX_TASKS)
 *
 * \note
 * The scheduler is cooperative: task functions must return quickly and must not
 * block for long periods.
 *
 * \defgroup group_basic_sched_macros Macros
 * Macros for configuring the scheduler, such as maximum tasks and tick conversion.
 * \defgroup group_basic_sched_data_structures Data Structures
 * Data structures for task control blocks and related types.
 * \defgroup group_basic_sched_functions Functions
 * Functions for initializing the scheduler, adding tasks, and dispatching.
 * \}
 */


#ifndef BASIC_SCHED_H
#define BASIC_SCHED_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup group_basic_sched_macros
 * \{
 */

/**
 * \brief Maximum number of tasks that can be registered in the scheduler. Default 16.
 */
#ifndef SVC_SCHED_MAX_TASKS
#define SVC_SCHED_MAX_TASKS 16
#endif

/**
 * \brief Maximum supported delay/period in ticks.
 *
 * \details
 * The scheduler uses wrap-safe time comparisons based on signed 32-bit deltas
 * (see implementation of due checks). For that technique to be unambiguous
 * across tick wrap, delays must be strictly less than 2^31 ticks.
 */
#ifndef SVC_SCHED_MAX_PERIOD_TICKS
#define SVC_SCHED_MAX_PERIOD_TICKS (0x7FFFFFFFu)
#endif

/** \} group_basic_sched_macros */


/** \addtogroup group_basic_sched_data_structures
 * \{
 */

/**
 * \brief Task function prototype. Task functions must match this signature.
 */
typedef void (* svc_task_fn_t)(void* arg);


/**
 * \brief Task type.
 */
typedef enum
{
    SVC_TASK_UNUSED   = 0,  /**< Unused */
    SVC_TASK_PERIODIC = 1,  /**< Periodic task */
    SVC_TASK_ONESHOT  = 2   /**< One-shot task */
} svc_task_type_t;


/**
 * \brief Task control block.
 */
typedef struct
{
    svc_task_fn_t   fn;         /**< Task function */
    void*           arg;        /**< Task argument */
    uint32_t        period_ticks; /**< Task period in ticks */
    uint32_t        next_due;   /**< Next due time in ticks */
    svc_task_type_t type;       /**< Task type */
    uint8_t         enabled;    /**< Task enabled flag */
    uint8_t         priority;   /**< Task priority */
} svc_task_t;

/**
 * \brief Task ID. Returned by add functions and used for enable/disable/kick.
 */
typedef int svc_task_id_t;

/** \} group_basic_sched_data_structures */


/** \addtogroup group_basic_sched_functions
 * \{
 */

/*******************************************************************************
 * Function Name: svc_sched_init
 ***************************************************************************//**
 * \brief Initialize the scheduler.
 *
 * \details
 * Configures a periodic tick source (SysTick when available) and initializes
 * internal scheduler state.
 *
 * \param tick_hz Desired tick rate in Hz.
 *
 * \return True if the timer and scheduler were configured successfully.
 */
bool svc_sched_init(uint32_t tick_hz);


/*******************************************************************************
 * Function Name: svc_sched_dispatch
 ***************************************************************************//**
 * \brief Run due tasks cooperatively.
 *
 * \details
 * Call this from the main while(1) loop. It runs tasks whose due time has
 * elapsed. Tasks run to completion; there is no preemption.
 */
void svc_sched_dispatch(void);


/*******************************************************************************
 * Function Name: svc_sched_now
 ***************************************************************************//**
 * \brief Get current scheduler time.
 *
 * \return Current tick counter value.
 *
 * \note
 * The returned value is a simple read of an incrementing counter. If a fully
 * consistent read is required in a particular system, wrap the call in a
 * critical section.
 */
uint32_t svc_sched_now(void);       // returns current tick


/*******************************************************************************
 * Function Name: svc_sched_ms_to_ticks
 ***************************************************************************//**
 * \brief Convert milliseconds to scheduler ticks.
 *
 * \param ms Milliseconds.
 *
 * \return Equivalent number of ticks (rounded up). Returns 0 if \p ms is 0 or
 * if the scheduler has not been initialized.
 */
uint32_t svc_sched_ms_to_ticks(uint32_t ms);


/*******************************************************************************
 * Function Name: svc_sched_us_to_ticks
 ***************************************************************************//**
 * \brief Convert microseconds to scheduler ticks.
 *
 * \param us Microseconds.
 *
 * \return Equivalent number of ticks (rounded up). Returns 0 if \p us is 0 or
 * if the scheduler has not been initialized.
 */
uint32_t svc_sched_us_to_ticks(uint32_t us);


/*******************************************************************************
 * Function Name: svc_sched_tick_isr
 ***************************************************************************//**
 * \brief Tick ISR hook.
 *
 * \details
 * This function increments the tick counter and requests a dispatch in the
 * main loop.
 *
 * \c svc_sched_init() installs this function directly into the Cortex-M vector
 * table (VTOR[15]) so that no \c SysTick_Handler symbol is emitted by the
 * library. Applications that need to share SysTick with other uses must chain
 * a call to this function from their own handler after restoring or managing
 * the VTOR entry themselves.
 */
void svc_sched_tick_isr(void);



/*******************************************************************************
 * Function Name: svc_sched_add_periodic_ms
 ***************************************************************************//**
 * \brief Add a periodic task (milliseconds).
 *
 * \param fn Task function.
 * \param arg Task argument passed to \p fn.
 * \param period_ms Period in milliseconds.
 * \param priority Task priority (lower value = higher priority).
 *
 * \return Task ID on success, or -1 on failure.
 */
svc_task_id_t svc_sched_add_periodic_ms(svc_task_fn_t fn,
                                        void* arg,
                                        uint32_t period_ms,
                                        uint8_t priority);


/*******************************************************************************
 * Function Name: svc_sched_add_oneshot_ms
 ***************************************************************************//**
 * \brief Add a one-shot task (milliseconds).
 *
 * \param fn Task function.
 * \param arg Task argument passed to \p fn.
 * \param delay_ms Delay from now in milliseconds.
 * \param priority Task priority (lower value = higher priority).
 *
 * \return Task ID on success, or -1 on failure.
 */
svc_task_id_t svc_sched_add_oneshot_ms(svc_task_fn_t fn,
                                       void* arg,
                                       uint32_t delay_ms,
                                       uint8_t priority);


/*******************************************************************************
 * Function Name: svc_sched_enable
 ***************************************************************************//**
 * \brief Enable a task.
 * \param task_id Task ID returned from an add function.
 */
void svc_sched_enable(svc_task_id_t task_id);


/*******************************************************************************
 * Function Name: svc_sched_disable
 ***************************************************************************//**
 * \brief Disable a task.
 * \param task_id Task ID returned from an add function.
 * \note
 * Disabling a one-shot task also releases its slot for reuse.
 */
void svc_sched_disable(svc_task_id_t task_id);


/*******************************************************************************
 * Function Name: svc_sched_kick
 ***************************************************************************//**
 * \brief Force a task to become due immediately.
 *
 * \details
 * Useful when an external event (e.g. IPC message) should trigger immediate
 * execution of a task that is otherwise periodic.
 *
 * \param task_id Task ID returned from an add function.
 */
void svc_sched_kick(svc_task_id_t task_id);


/*******************************************************************************
 * Function Name: svc_sched_add_periodic_ticks
 ***************************************************************************//**
 * \brief Add a periodic task (ticks).
 *
 * \param fn Task function.
 * \param arg Task argument passed to \p fn.
 * \param period_ticks Period in scheduler ticks.
 * \param priority Task priority (lower value = higher priority).
 *
 * \return Task ID on success, or -1 on failure.
 */
svc_task_id_t svc_sched_add_periodic_ticks(svc_task_fn_t fn,
                                           void* arg,
                                           uint32_t period_ticks,
                                           uint8_t priority);


/*******************************************************************************
 * Function Name: svc_sched_add_oneshot_ticks
 ***************************************************************************//**
 * \brief Add a one-shot task (ticks).
 *
 * \param fn Task function.
 * \param arg Task argument passed to \p fn.
 * \param delay_ticks Delay from now in scheduler ticks.
 * \param priority Task priority (lower value = higher priority).
 *
 * \return Task ID on success, or -1 on failure.
 */
svc_task_id_t svc_sched_add_oneshot_ticks(svc_task_fn_t fn,
                                          void* arg,
                                          uint32_t delay_ticks,
                                          uint8_t priority);

/** \} group_basic_sched_functions */

#ifdef __cplusplus
}
#endif


#endif /* BASIC_SCHED_H */
