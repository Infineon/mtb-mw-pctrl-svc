/***************************************************************************//**
* \file cy_ppca_shm.c
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

#include "cy_ppca_shm.h"



/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Init
*******************************************************************************/
void Cy_PPCA_SharedMem_Init(void)
{
    /* Common Mutex Enable */
    #if CONFIG_USE_MUTEX_LOCK
    mutex_enable();
    #endif

    /* For PPCA devices, initialization should is done only in main core */
    #if !defined (COMPONENT_PPCA_DEVICE)

    #if CONFIG_USE_IPC_LOCK
    /* Initialize IP locks */
    for (uint32_t i = 0U; i < (uint32_t)CONFIG_USE_LOCK_MAX_CHAL_NUM; i++)
    {
        ipc_lock[i] = SM_IPC_STRUCT_PTR((CONFIG_USE_LOCK_BASE_CHNL + i));
    }
    #endif

    #if CONFIG_USE_MUTEX_LOCK
    /* Initialize mutx locks */
    for (uint32_t i = 0U; i < (uint32_t)CONFIG_USE_LOCK_MAX_CHAL_NUM; i++)
    {
        mutex[i] = MUTEX_UNLOCKED;
    }
    #endif

    #if CONFIG_USE_MEMORY_POOL
    /* Initialize memory pool */
    shm_pool_init();
    #endif

    #endif /* ! COMPONENT_PPCA_DEVICE */
}


#if CONFIG_USE_MEMORY_POOL
/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_Blk_Init
*******************************************************************************/
void Cy_PPCA_SharedMem_Blk_Init(volatile cy_ppca_shm_blk_t* blk_ptr, uint32_t size, cy_ppca_shm_channel_t chnl)
{
    blk_ptr->chnl = chnl;
    void* buf     = shm_pool_malloc(size);
    #if defined(CY_DEVICE_PSC3_P8)
    blk_ptr->data       = MAIN_CPU_ADDR(buf);
    blk_ptr->data_ppca  = PPCA_CPU_ADDR(buf);
    #else
    blk_ptr->data = buf;
    #endif /* CY_DEVICE_PSC3_P8 */
}


#endif /* CONFIG_USE_MEMORY_POOL */
