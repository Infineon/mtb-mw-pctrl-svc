/***************************************************************************//**
* \file cy_ppca_shm_pool.c
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


#include "cy_ppca_shm_pool.h"


#if CONFIG_USE_MEMORY_POOL


/* shm_pool_free_list contains the remaining block, located in the pool */
mem_block_t* shm_pool_free_list = (mem_block_t*)(void*)shared_mem_pool;

/* Initialize the shared memory region for dynamic allocation. */
void shm_pool_init(void)
{
    shm_pool_free_list->size = CONFIG_SHARED_MEM_POOL_SIZE - sizeof(mem_block_t);
    shm_pool_free_list->next = NULL;
}


/* Simple "malloc". a lot of TODOs */
void* shm_pool_malloc(size_t size)
{
    /* Align size to 4 bytes for proper alignment. */
    /* size = (size + 3) & ~3; */

    mem_block_t* prev = NULL;
    mem_block_t* curr = shm_pool_free_list;

    /* Traverse the free list looking for a block that is big enough. */
    while (curr != NULL)
    {
        if (curr->size >= size)
        {
            /* Check if we can split the block. */
            if (curr->size >= (size + sizeof(mem_block_t) + 4U))
            {
                /* Create a new block header after the allocated memory. */
                mem_block_t* new_block = (mem_block_t*)(void*)((uint8_t*)curr + sizeof(mem_block_t) + size);
                new_block->size = curr->size - size - sizeof(mem_block_t);
                new_block->next = curr->next;
                /* Adjust the current block's size. */
                curr->size = size;
                /* Update the free list link. */
                if (prev != NULL)
                {
                    prev->next = new_block;
                }
                else
                {
                    shm_pool_free_list = new_block;
                }
            }
            else
            {
                /* Block is too small to split; remove it from the free list. */
                if (prev != NULL)
                {
                    prev->next = curr->next;
                }
                else
                {
                    shm_pool_free_list = curr->next;
                }
            }
            /* Return a pointer to the memory after the block header. */
            return (void*)((uint8_t*)curr + sizeof(mem_block_t));
        }
        prev = curr;
        curr = curr->next;
    }
    /* Out of memory. */
    return NULL;
}


/* Simple "free", we dont need this as we are not "dynamic" */
void shm_pool_free(void* ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    /* Retrieve the block header. */
    mem_block_t* block = (mem_block_t*)(void*)((uint8_t*)ptr - sizeof(mem_block_t));

    /* For simplicity, insert the freed block at the beginning of the free list. */
    block->next = shm_pool_free_list;
    shm_pool_free_list = block;
}


#endif /* CONFIG_USE_MEMORY_POOL */
