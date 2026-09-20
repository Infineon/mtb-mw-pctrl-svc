/***************************************************************************//**
* \file cy_ppca_shm_tb.h
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

#ifndef CY_PPCA_SHM_TB_H
#define CY_PPCA_SHM_TB_H

#include "cy_ppca_shm_common.h"
#include "mutex.h"

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 5.3', 1, \
                             'Member "mutex" intentionally named to match the type for API consistency; no ambiguity with the external linkage array.')
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 5.8', 1, \
                             'Member "mutex" is a struct member that does not conflict with the external linkage array of the same name.')

#if CONFIG_USE_MEMORY_POOL

/**
 * \brief Triple buffer block structure.
 */
typedef struct
{
    uint32_t    new_data;           /**< Flag indicating which buffer is empty */
    uint32_t    read_index;         /**< Index of the read buffer */
    uint32_t    write_index;        /**< Index of the write buffer */
    uint32_t    switch_index;       /**< Index of the switch buffer */
    mutex_t     mutex;              /**< Mutex for buffer access */
    void*       buffer[3];          /**< Triple buffer */
    #if defined(CY_DEVICE_PSC3_P8)
    void*       buffer_ppca[3];     /**< Triple buffer for PPCA core */
    #endif
} cy_ppca_shm_tb_blk_t;

#else // if CONFIG_USE_MEMORY_POOL

/**
 * \brief Triple buffer block structure for non-memory pool configuration.
 */
typedef struct
{
    volatile uint32_t*   new_data;           /**< Flag indicating which buffer is empty */
    volatile mutex_t*    mutex;              /**< Mutex to synchronize shared memory access */
    volatile void*       read_buf;           /**< Pointer to the read buffer */
    volatile void*       write_buf;          /**< Pointer to the write buffer */
    volatile void*       switch_buf;         /**< Pointer to the switch buffer */
} cy_ppca_shm_tb_blk_t;

#endif // if CONFIG_USE_MEMORY_POOL
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 5.8')
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 5.3')


/** Triple buffer design**
 *   <br>
 *   Data structure
 *   - Three buffers are allocated for read, write and switch
 *       - Reader will read from read buffer
 *       - Writer will write to write buffer.
 *       - Switch buffer is used to switch read and write buffer
 *   - new_data flag is used to indicate new data is available
 *   - mutex is used to protect buffer switching
 *
 *   Operations - Writer
 *   - Get write buffer
 *   - Access write buffer as its own, exclusive access
 *   - Update write buffer by swapping write and switch buffer.
 *   - Writer also set new_data flag to indicate new data is available
 *
 *   Operations - Reader
 *   - Get read buffer.
 *       - If new_data flag is set, swap read and switch buffer.
 *       - If not, read existing read buffer.
 *   - Access read buffer as its own, exclusive access
 */

#if CONFIG_USE_MEMORY_POOL


/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_Init
 ****************************************************************************//**
 *
 * \brief Initializes the triple buffer block.
 *
 * \param tb_blk Pointer to the triple buffer block structure.
 *
 * \param size Size of the buffer.
 */
void Cy_PPCA_SharedMem_TB_Init(volatile cy_ppca_shm_tb_blk_t* tb_blk, uint32_t size);



/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_GetWriteBuf
 ****************************************************************************//**
 *
 * \brief Gets the write buffer.
 *
 * \param tb_blk Pointer to the triple buffer block structure.
 *
 * \return Pointer to the write buffer.
 */
__STATIC_INLINE void* Cy_PPCA_SharedMem_TB_GetWriteBuf(volatile cy_ppca_shm_tb_blk_t* tb_blk)
{
    #if defined(CY_DEVICE_PSC3_P8)

    #if CORE_NAME_CM33_0 == 1
    return tb_blk->buffer[tb_blk->write_index];
    #else
    return tb_blk->buffer_ppca[tb_blk->write_index];
    #endif

    #else
    return tb_blk->buffer[tb_blk->write_index];
    #endif /* CY_DEVICE_PSC3_P8 */
}


/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_UpdateWriteBuf
 ****************************************************************************//**
 *
 * \brief Updates the write buffer.
 *
 * \param tb_blk Pointer to the triple buffer block structure.
 *
 */
__STATIC_INLINE void Cy_PPCA_SharedMem_TB_UpdateWriteBuf(volatile cy_ppca_shm_tb_blk_t* tb_blk)
{
    /* Swap write & switch */
    while (mutex_lock(&tb_blk->mutex) != true)
    {
    }
    uint32_t temp;
    temp = tb_blk->write_index;
    tb_blk->write_index = tb_blk->switch_index;
    tb_blk->switch_index = temp;
    mutex_unlock(&tb_blk->mutex);    /* set flag */
    tb_blk->new_data = 1U;
}


/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_GetReadBuf
 ****************************************************************************//**
 *
 * \brief Gets the read buffer.
 *
 * \param tb_blk Pointer to the triple buffer block structure.
 *
 * \return Pointer to the read buffer.
 */
__STATIC_INLINE void* Cy_PPCA_SharedMem_TB_GetReadBuf(volatile cy_ppca_shm_tb_blk_t* tb_blk)
{
    /* New data */
    if (tb_blk->new_data == 1U)
    {
        /* Swap read & switch */
        while (mutex_lock(&tb_blk->mutex) != true)
        {
        }
        uint32_t temp;
        temp = tb_blk->read_index;
        tb_blk->read_index = tb_blk->switch_index;
        tb_blk->switch_index = temp;
        mutex_unlock(&tb_blk->mutex);        /* clear flag */
        tb_blk->new_data = 0U;
    }

    #if defined(CY_DEVICE_PSC3_P8)
    #if CORE_NAME_CM33_0 == 1
    return tb_blk->buffer[tb_blk->read_index];
    #else
    return tb_blk->buffer_ppca[tb_blk->read_index];
    #endif
    #else
    return tb_blk->buffer[tb_blk->read_index];
    #endif /* CY_DEVICE_PSC3_P8 */
}


#else /* CONFIG_USE_MEMORY_POOL */

/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_Init
 ****************************************************************************//**
 * \brief Initializes the triple buffer block.
 *
 * \param tb_blk Pointer to the triple buffer block structure.
 *
 * \param new_data Pointer to the new data flag.
 *
 * \param mutex Pointer to the mutex.
 *
 * \param read_buf Pointer to the read buffer.
 *
 * \param write_buf Pointer to the write buffer.
 *
 * \param switch_buf Pointer to the switch buffer.
 */
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 5.3', 1, \
                             'Parameter "mutex" intentionally named to match the type for API clarity; no ambiguity with the external linkage array.')
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 5.8', 1, \
                             'Parameter "mutex" is a local identifier that does not conflict with the external linkage array of the same name.')
__STATIC_INLINE void Cy_PPCA_SharedMem_TB_Init(volatile cy_ppca_shm_tb_blk_t* tb_blk,
                                               volatile uint32_t* new_data,
                                               volatile mutex_t* mutex,
                                               volatile void* read_buf,
                                               volatile void* write_buf,
                                               volatile void* switch_buf)
{
    tb_blk->new_data    = new_data;
    tb_blk->mutex       = mutex;
    tb_blk->read_buf    = read_buf;
    tb_blk->write_buf   = write_buf;
    tb_blk->switch_buf  = switch_buf;
}


CY_MISRA_BLOCK_END('MISRA C-2012 Rule 5.8')
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 5.3')


/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_GetWriteBuf
 ****************************************************************************//**
 *
 * \brief Gets the write buffer.
 *
 * \param tb_blk Pointer to the triple buffer block structure.
 *
 * \return Pointer to the write buffer.
 */
__STATIC_INLINE volatile void* Cy_PPCA_SharedMem_TB_GetWriteBuf(volatile cy_ppca_shm_tb_blk_t* tb_blk)
{
    return tb_blk->write_buf;
}


/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_UpdateWriteBuf
 ****************************************************************************//**
 *
 * \brief Updates the write buffer.
 *
 * \param tb_blk Pointer to the triple buffer block structure.
 */
__STATIC_INLINE void Cy_PPCA_SharedMem_TB_UpdateWriteBuf(volatile cy_ppca_shm_tb_blk_t* tb_blk)
{
    /* Swap write & switch */
    while (mutex_lock(tb_blk->mutex) != true)
    {
    }
    volatile void* temp;
    temp = tb_blk->write_buf;
    tb_blk->write_buf = tb_blk->switch_buf;
    tb_blk->switch_buf = temp;
    mutex_unlock(tb_blk->mutex);

    /* set flag */
    *(tb_blk->new_data) = 1U;
}


/*******************************************************************************
 * Function Name: Cy_PPCA_SharedMem_TB_GetReadBuf
 ****************************************************************************//**
 *
 * \brief Gets the read buffer.
 *
 * \param tb_blk Pointer to the triple buffer block structure.
 *
 * \return Pointer to the read buffer.
 */
__STATIC_INLINE volatile void* Cy_PPCA_SharedMem_TB_GetReadBuf(volatile cy_ppca_shm_tb_blk_t* tb_blk)
{
    /* New data */
    if (*(tb_blk->new_data) == 1U)
    {
        /* Swap read & switch */
        while (mutex_lock(tb_blk->mutex) != true)
        {
        }
        volatile void* temp;
        temp = tb_blk->read_buf;
        tb_blk->read_buf = tb_blk->switch_buf;
        tb_blk->switch_buf = temp;
        mutex_unlock(tb_blk->mutex);
        /* clear flag */
        *(tb_blk->new_data) = 0U;
    }

    return tb_blk->read_buf;
}


#endif /* CONFIG_USE_MEMORY_POOL */


#endif /* CY_PPCA_SHM_TB_H */
