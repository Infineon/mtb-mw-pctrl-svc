/***************************************************************************//**
* \file cy_ppca_shm_tb.c
* \brief
*   This file contains the inline function, macros and constants for inter-core
*   communication based on shared memory implementation, using triple buffer
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

#include "cy_ppca_shm_tb.h"

#if CONFIG_USE_MEMORY_POOL
#include "cy_ppca_shm_pool.h"
#endif


#if CONFIG_USE_MEMORY_POOL

/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_Init
 ******************************************************************************/
void Cy_PPCA_SharedMem_TB_Init(volatile cy_ppca_shm_tb_blk_t* tb_blk, uint32_t size)
{
    tb_blk->new_data = 0U;
    tb_blk->read_index = 0U;
    tb_blk->write_index = 1U;
    tb_blk->switch_index = 2U;
    tb_blk->mutex = MUTEX_UNLOCKED;
    void* buf[3];
    for (uint32_t i = 0U; i < 3U; i++)
    {
        buf[i] = shm_pool_malloc(size);
    }
    #if defined(CY_DEVICE_PSC3_P8)
    for (uint32_t i = 0U; i < 3U; i++)
    {
        tb_blk->buffer[i] = MAIN_CPU_ADDR(buf[i]);
        tb_blk->buffer_ppca[i] = PPCA_CPU_ADDR(buf[i]);
    }
    #else
    for (uint32_t i = 0U; i < 3U; i++)
    {
        tb_blk->buffer[i] = buf[i];
    }
    #endif /* CY_DEVICE_PSC3_P8 */
}


#endif // if CONFIG_USE_MEMORY_POOL
