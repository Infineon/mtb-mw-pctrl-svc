/***************************************************************************//**
* \file cy_ppca_shm_pool.h
* \brief
*   This file contains the function, macros and constants for shared memory pool
*  management.
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

#ifndef CY_PPCA_SHM_POOL_H
#define CY_PPCA_SHM_POOL_H

#include "cy_ppca_shm_common.h"

#if CONFIG_USE_MEMORY_POOL

/* definition of mem block */
typedef struct mem_block
{
    size_t size;             // Size of the block (excluding header)
    struct mem_block* next;  // Pointer to the next free block
} mem_block_t;

/* shared memory pool */
extern volatile uint8_t shared_mem_pool[CONFIG_SHARED_MEM_POOL_SIZE];

/*******************************************************************************
* Function Name: shm_pool_init
****************************************************************************//**
*
* \brief Initialize the shared memory region for dynamic allocation
*
*******************************************************************************/
void shm_pool_init(void);


/*******************************************************************************
* Function Name: shm_pool_malloc
****************************************************************************//**
*
* \brief Simple "malloc". A lot of TODOs
*
*******************************************************************************/
void* shm_pool_malloc(size_t size);


/*******************************************************************************
* Function Name: shm_pool_free
****************************************************************************//**
*
* \brief Simple "free", we dont need this as we are not "dynamic"
*
*******************************************************************************/
void shm_pool_free(void* ptr);

#endif /* CONFIG_USE_MEMORY_POOL */

#endif /* CY_PPCA_SHM_POOL_H */
