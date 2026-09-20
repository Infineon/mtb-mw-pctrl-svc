/***************************************************************************//**
* \file basic_sched.c
* \brief
*   This file contains the basic scheduler implementation.
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

#include "basic_sched.h"

#include "cy_device.h"
#include "cy_sysint.h"
#include "cmsis_compiler.h"

/* SystemCoreClock is defined in system_<device>.c */
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 8.5', 1, 'Re-declaration here avoids pulling in full system header.')
extern uint32_t SystemCoreClock;
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 8.5')

/* Task list */
static svc_task_t g_tasks[SVC_SCHED_MAX_TASKS];

/* Scheduler time base */
static volatile uint32_t g_svc_ticks;
static volatile uint8_t  g_svc_tick_pend;
static uint32_t          g_tick_hz;

/* ---------- Internal helpers ---------- */
__STATIC_INLINE void sched_request_dispatch(void)
{
    g_svc_tick_pend = 1u;
}


/* ---------- Time base ---------- */
/* Get current tick count helper (for internal use) */
__STATIC_INLINE uint32_t sched_now(void)
{
    /* Simple, non-atomic read is usually OK; if you care, wrap in critical section */
    return g_svc_ticks;
}


uint32_t svc_sched_now(void)
{
    /* Simple, non-atomic read is usually OK; if you care, wrap in critical section */
    return g_svc_ticks;
}


uint32_t svc_sched_ms_to_ticks(uint32_t ms)
{
    if ((ms == 0u) || (g_tick_hz == 0u))
    {
        return 0u;
    }

    uint64_t ticks = ((uint64_t)ms * (uint64_t)g_tick_hz + 999u) / 1000u;
    return (ticks == 0u) ? 1u : (uint32_t)ticks;
}


uint32_t svc_sched_us_to_ticks(uint32_t us)
{
    if ((us == 0u) || (g_tick_hz == 0u))
    {
        return 0u;
    }

    uint64_t ticks = ((uint64_t)us * (uint64_t)g_tick_hz + 999999u) / 1000000u;
    return (ticks == 0u) ? 1u : (uint32_t)ticks;
}


// ---------- Init ----------

bool svc_sched_init(uint32_t tick_hz)
{
    uint32_t core_clk_hz = SystemCoreClock;

    if (tick_hz == 0u)
    {
        return false;
    }

    uint32_t ticks_per_interrupt = core_clk_hz / tick_hz;
    if ((ticks_per_interrupt == 0u) || (ticks_per_interrupt > (SysTick_LOAD_RELOAD_Msk + 1u)))
    {
        return false;
    }

    g_tick_hz = tick_hz;
    g_svc_ticks = 0u;
    g_svc_tick_pend = 0u;

    for (uint32_t i = 0U; i < (uint32_t)SVC_SCHED_MAX_TASKS; i++)
    {
        g_tasks[i].fn = NULL;
        g_tasks[i].arg = NULL;
        g_tasks[i].period_ticks = 0u;
        g_tasks[i].next_due = 0u;
        g_tasks[i].type = SVC_TASK_UNUSED;
        g_tasks[i].enabled = 0u;
        g_tasks[i].priority = 0u;
    }

    if (SysTick_Config(ticks_per_interrupt) != 0u)
    {
        return false;
    }

    // SysTick->LOAD = ticks_per_interrupt - 1u;
    // SysTick->VAL = 0u;
    // SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
    //                 SysTick_CTRL_TICKINT_Msk |
    //                 SysTick_CTRL_ENABLE_Msk;

    /* Install svc_sched_tick_isr directly as the SysTick handler via the PDL vector API.
     * This avoids defining a SysTick_Handler symbol in the library, preventing duplicate
     * symbol conflicts in applications that own their own SysTick_Handler. */
    (void)Cy_SysInt_SetVector(SysTick_IRQn, &svc_sched_tick_isr);

    return true;
}


/* ---------- Task creation helpers ---------- */
/* Allocate a free task slot */
__STATIC_INLINE svc_task_id_t sched_alloc_task_slot(void)
{
    svc_task_id_t result = -1;
    for (uint32_t i = 0U; i < (uint32_t)SVC_SCHED_MAX_TASKS; i++)
    {
        if (g_tasks[i].type == SVC_TASK_UNUSED)
        {
            result = (svc_task_id_t)i;
            break;
        }
    }
    return result;
}


svc_task_id_t svc_sched_add_periodic_ticks(svc_task_fn_t fn,
                                           void* arg,
                                           uint32_t period_ticks,
                                           uint8_t priority)
{
    svc_task_id_t idx = sched_alloc_task_slot();
    if ((idx < 0) || (fn == NULL) || (period_ticks == 0u) || (period_ticks > SVC_SCHED_MAX_PERIOD_TICKS))
    {
        return -1;
    }

    svc_task_t* t = &g_tasks[idx];
    t->fn           = fn;
    t->arg          = arg;
    t->period_ticks = period_ticks;
    t->next_due     = sched_now() + period_ticks;
    t->type         = SVC_TASK_PERIODIC;
    t->enabled      = 1u;
    t->priority     = priority;

    return idx;
}


svc_task_id_t svc_sched_add_oneshot_ticks(svc_task_fn_t fn,
                                          void* arg,
                                          uint32_t delay_ticks,
                                          uint8_t priority)
{
    svc_task_id_t idx = sched_alloc_task_slot();
    if ((idx < 0) || (fn == NULL) || (delay_ticks == 0u) || (delay_ticks > SVC_SCHED_MAX_PERIOD_TICKS))
    {
        return -1;
    }

    svc_task_t* t = &g_tasks[idx];
    t->fn           = fn;
    t->arg          = arg;
    t->period_ticks = 0u;
    t->next_due     = sched_now() + delay_ticks;
    t->type         = SVC_TASK_ONESHOT;
    t->enabled      = 1u;
    t->priority     = priority;

    return idx;
}


svc_task_id_t svc_sched_add_periodic_ms(svc_task_fn_t fn,
                                        void* arg,
                                        uint32_t period_ms,
                                        uint8_t priority)
{
    return svc_sched_add_periodic_ticks(fn, arg,
                                        svc_sched_ms_to_ticks(period_ms),
                                        priority);
}


svc_task_id_t svc_sched_add_oneshot_ms(svc_task_fn_t fn,
                                       void* arg,
                                       uint32_t delay_ms,
                                       uint8_t priority)
{
    return svc_sched_add_oneshot_ticks(fn, arg,
                                       svc_sched_ms_to_ticks(delay_ms),
                                       priority);
}


void svc_sched_enable(svc_task_id_t task_id)
{
    if ((task_id < 0) || (task_id >= SVC_SCHED_MAX_TASKS))
    {
        return;
    }

    if (g_tasks[task_id].type == SVC_TASK_UNUSED)
    {
        return;
    }

    g_tasks[task_id].enabled = 1u;

    /* Optionally re-arm from "now" */
    if ((g_tasks[task_id].type == SVC_TASK_PERIODIC) && (g_tasks[task_id].period_ticks != 0u))
    {
        g_tasks[task_id].next_due = sched_now() + g_tasks[task_id].period_ticks;
    }
}


void svc_sched_disable(svc_task_id_t task_id)
{
    if ((task_id < 0) || (task_id >= SVC_SCHED_MAX_TASKS))
    {
        return;
    }

    g_tasks[task_id].enabled = 0u;

    if (g_tasks[task_id].type == SVC_TASK_ONESHOT)
    {
        g_tasks[task_id].type = SVC_TASK_UNUSED;
        g_tasks[task_id].fn = NULL;
    }
}


void svc_sched_kick(svc_task_id_t task_id)
{
    if ((task_id < 0) || (task_id >= SVC_SCHED_MAX_TASKS))
    {
        return;
    }

    if (g_tasks[task_id].enabled == 0u)
    {
        return;
    }

    g_tasks[task_id].next_due = sched_now();
    sched_request_dispatch();
}


/* ---------- Dispatcher ---------- */
/* Check if task is due */
__STATIC_INLINE bool sched_task_is_due(const svc_task_t* t, uint32_t now)
{
    if ((t->enabled == 0u) || (t->type == SVC_TASK_UNUSED))
    {
        return false;
    }
    uint32_t diff = now - t->next_due;
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 10.5', \
                          'Cast of uint32_t to int32_t for wrap-safe tick comparison; value wraps correctly for due-time detection.');
    return ((int32_t)diff >= 0);
}


/* Best task is highest priority (lowest number), then earliest due time */
__STATIC_INLINE svc_task_id_t sched_pick_best_task(uint32_t now)
{
    svc_task_id_t best_idx = -1;
    uint8_t best_prio = 0xFFu;
    uint32_t best_due = 0u;

    for (uint32_t i = 0U; i < (uint32_t)SVC_SCHED_MAX_TASKS; i++)
    {
        svc_task_t* t = &g_tasks[i];
        if (!sched_task_is_due(t, now))
        {
            continue;
        }


        if ((best_idx < 0) || (t->priority < best_prio))
        {
            best_idx = (svc_task_id_t)i;
            best_prio = t->priority;
            best_due = t->next_due;
            continue;
        }
        uint32_t due_diff = t->next_due - best_due;
        CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 10.5', \
                              'Cast of uint32_t to int32_t for wrap-safe tick comparison; value wraps correctly for earliest-due detection.');
        if ((t->priority == best_prio) && ((int32_t)due_diff < 0))
        {
            best_idx = (svc_task_id_t)i;
            best_due = t->next_due;
        }
    }

    return best_idx;
}


void svc_sched_dispatch(void)
{
    /* Check if a tick has occurred */
    if (g_svc_tick_pend == 0u)
    {
        return;
    }
    g_svc_tick_pend = 0u;

    /* Run all due tasks in priority order */
    while (true)
    {
        uint32_t now = sched_now();
        svc_task_id_t task_idx = sched_pick_best_task(now);
        if (task_idx < 0)
        {
            break;
        }

        svc_task_t* task = &g_tasks[task_idx];
        svc_task_fn_t fn = task->fn;
        void* arg = task->arg;

        svc_task_type_t type = task->type;
        uint32_t period_ticks = task->period_ticks;

        /* For one-shot tasks, free the slot before running */
        if (type == SVC_TASK_ONESHOT)
        {
            task->enabled = 0u;
            task->type = SVC_TASK_UNUSED;
            task->fn = NULL;
        }

        /* Run the task */
        if (fn != NULL)
        {
            fn(arg);
        }

        /* Periodic reschedule: fixed-delay from completion time.
         * This avoids pathological "catch-up" loops if a task overruns its period.
         */
        if ((type == SVC_TASK_PERIODIC) && (period_ticks != 0u) &&
            (task->enabled != 0u) && (task->type == SVC_TASK_PERIODIC))
        {
            task->next_due = sched_now() + period_ticks;
        }
    }
}


/* ---------- SysTick ISR hook ---------- */
void svc_sched_tick_isr(void)
{
    g_svc_ticks++;
    sched_request_dispatch();
}
