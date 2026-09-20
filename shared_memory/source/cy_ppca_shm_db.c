/***************************************************************************//**
* \file cy_ppca_shm_db.c
* \brief
*   This file contains the inline function, macros and constants for inter-core
*   communication based on shared memory implementation, using double buffer
*   design.
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

#include "cy_ppca_shm_db.h"

#if CONFIG_USE_MEMORY_POOL
#include "cy_ppca_shm_pool.h"
#endif

#if CONFIG_USE_MEMORY_POOL
/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_DB_Init
*******************************************************************************/
void Cy_PPCA_SharedMem_DB_Init(volatile cy_ppca_shm_db_blk_t* db_blk, uint32_t size)
{
    db_blk->flag = 0U;
    void* buf_0 = shm_pool_malloc(size);
    void* buf_1 = shm_pool_malloc(size);
    #if defined(CY_DEVICE_PSC3_P8)
    db_blk->buffer_0       = MAIN_CPU_ADDR(buf_0);
    db_blk->buffer_1       = MAIN_CPU_ADDR(buf_1);
    db_blk->buffer_0_ppca  = PPCA_CPU_ADDR(buf_0);
    db_blk->buffer_1_ppca  = PPCA_CPU_ADDR(buf_1);
    #else
    db_blk->buffer_0       = buf_0;
    db_blk->buffer_1       = buf_1;
    #endif /* CY_DEVICE_PSC3_P8 */
}


#endif /* CONFIG_USE_MEMORY_POOL */
