/***************************************************************************//**
* \file cy_ppca_shm_db.h
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

#ifndef CY_PPCA_SHM_DB_H
#define CY_PPCA_SHM_DB_H

#include "cy_ppca_shm_common.h"

/*******************************************************************************
 * Double buffer design
 ****************************************************************************//**
 *
 * \note Double buffer design
 *       - Two buffers for reading and writing at the same time
 *       - Flag to indicate which buffer is used for reading and writing
 *       - Writer will flip the flag after writing
 *
 *       In PSOC C3P8, because of additional delay introduced when main core accessing
 *       PPCA, this design is considered NOT safe.
 *
 */


#if CONFIG_USE_MEMORY_POOL
/**
 * \brief Double buffer block structure.
 */
typedef struct
{
    uint32_t            flag;               // flag to which buffer is empty
    void*               buffer_0;           // double buffer
    void*               buffer_1;           // double buffer
    #if defined(CY_DEVICE_PSC3_P8)
    void*               buffer_0_ppca;      // double buffer
    void*               buffer_1_ppca;      // double buffer
    #endif
} cy_ppca_shm_db_blk_t;



/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_DB_Init
****************************************************************************//**
*
* \brief Initialize a shared memory variable.
* The shared memory block should be declared in shared memory region.
*
* \param db_blk
* The pointer to the shared memory variable to be initialized.
*
* \param size
* The size of the buffer. This must be in multiples of 4 bytes.
*
*******************************************************************************/
void Cy_PPCA_SharedMem_DB_Init(volatile cy_ppca_shm_db_blk_t* db_blk, uint32_t size);



/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_DB_Write
****************************************************************************//**
*
* \brief Write a block of data to a shared memory variable.
* The block data should be in multiples of 32-bit (4 bytes).
*
* \param db_blk
* The pointer to the shared memory block structure.
*
* \param data_ptr
* The pointer to the block data to be written.
*
* \param size
* The size of the data block. This must be in multiples of 4 bytes.
*
* \note The shared memory variable must be declared at a 32-bit aligned address,
*       e.g., __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_DB_Write(volatile cy_ppca_shm_db_blk_t* db_blk,
                                                const void* data_ptr,
                                                uint32_t size)
{
    /* Get which buffer to use */
    uint32_t flag = db_blk->flag;
    uint32_t* dest;

    #if defined(CY_DEVICE_PSC3_P8)
    #if CORE_NAME_CM33_0 == 1
    if ((flag & 0x1U) != 0U)
    {
        dest = db_blk->buffer_0;
    }
    else
    {
        dest = db_blk->buffer_1;
    }
    #else
    if ((flag & 0x1U) != 0U)
    {
        dest = db_blk->buffer_0_ppca;
    }
    else
    {
        dest = db_blk->buffer_1_ppca;
    }
    #endif /* CORE_NAME_CM33_0 == 1 */
    #else /* CY_DEVICE_PSC3_P8 */
    if ((flag & 0x1U) != 0U)
    {
        dest = db_blk->buffer_0;
    }
    else
    {
        dest = db_blk->buffer_1;
    }
    #endif /* CY_DEVICE_PSC3_P8 */

    /* Write */
    Cy_PPCA_SharedMem_MemCpy32(dest, data_ptr, size);

    /* Update flag */
    db_blk->flag = flag ^ 0x1U;
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_DB_Read
****************************************************************************//**
*
* \brief Read a block of data from a shared memory variable.
* The block data should be in multiples of 32-bit (4 bytes).
*
* \param db_blk
* The pointer to the shared memory block structure.
*
* \param data_ptr
* The pointer to the buffer where the read data will be stored.
*
* \param size
* The size of the data block. This must be in multiples of 4 bytes.
*
* \note The shared memory variable must be declared at a 32-bit aligned address,
*       e.g., __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_DB_Read(volatile cy_ppca_shm_db_blk_t* db_blk,
                                               void* data_ptr,
                                               uint32_t size)
{
    /* Get which buffer to use */
    uint32_t flag = db_blk->flag;
    uint32_t* source;
    #if defined(CY_DEVICE_PSC3_P8)
    #if CORE_NAME_CM33_0 == 1
    if ((flag & 0x1U) != 0U)
    {
        source = db_blk->buffer_1;
    }
    else
    {
        source = db_blk->buffer_0;
    }
    #else
    if ((flag & 0x1U) != 0U)
    {
        source = db_blk->buffer_1_ppca;
    }
    else
    {
        source = db_blk->buffer_0_ppca;
    }
    #endif /* CORE_NAME_CM33_0 == 1 */
    #else /* CY_DEVICE_PSC3_P8 */
    if ((flag & 0x1U) != 0U)
    {
        source = db_blk->buffer_1;
    }
    else
    {
        source = db_blk->buffer_0;
    }
    #endif /* CY_DEVICE_PSC3_P8 */

    /* Read */
    Cy_PPCA_SharedMem_MemCpy32(data_ptr, source, size);
}


#else /* CONFIG_USE_MEMORY_POOL */

/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_DB_Write
****************************************************************************//**
*
* \brief Write a block of data to a shared memory variable.
* The block data should be in multiples of 32-bit (4 bytes).
*
* \param buf_flag
* The pointer to the flag indicating which buffer to use.
*
* \param buf_0
* The pointer to the first buffer.
*
* \param buf_1
* The pointer to the second buffer.
*
* \param data_ptr
* The pointer to the block data to be written.
*
* \param size
* The size of the data block. This must be in multiples of 4 bytes.
*
* \note The shared memory variable must be declared at a 32-bit aligned address,
*       e.g., __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_DB_Write(uint32_t* buf_flag,
                                                void* buf_0,
                                                void* buf_1,
                                                void* data_ptr,
                                                uint32_t size)
{
    /* Get which buffer to use */
    uint32_t flag = *(buf_flag);
    uint32_t* dest;

    if ((flag & 0x1U) != 0U)
    {
        dest = buf_0;
    }
    else
    {
        dest = buf_1;
    }

    /* Write */
    Cy_PPCA_SharedMem_MemCpy32(dest, data_ptr, size);

    /* Update flag */
    *(buf_flag) = flag ^ 0x1U;
}


/*******************************************************************************
* Function Name: Cy_PPCA_SharedMem_DB_Read
****************************************************************************//**
*
* \brief Read a block of data from a shared memory variable.
* The block data should be in multiples of 32-bit (4 bytes).
*
* \param buf_flag
* The pointer to the flag indicating which buffer to use.
*
* \param buf_0
* The pointer to the first buffer.
*
* \param buf_1
* The pointer to the second buffer.
*
* \param data_ptr
* The pointer to the buffer where the read data will be stored.
*
* \param size
* The size of the data block. This must be in multiples of 4 bytes.
*
* \note The shared memory variable must be declared at a 32-bit aligned address,
*       e.g., __attribute__((aligned(4))).
*
*******************************************************************************/
__STATIC_INLINE void Cy_PPCA_SharedMem_DB_Read(uint32_t* buf_flag,
                                               void* buf_0,
                                               void* buf_1,
                                               void* data_ptr,
                                               uint32_t size)
{
    /* Get which buffer to use */
    uint32_t flag = *(buf_flag);
    uint32_t* source;
    if ((flag & 0x1U) != 0U)
    {
        source = buf_0;
    }
    else
    {
        source = buf_1;
    }

    /* Read */
    Cy_PPCA_SharedMem_MemCpy32(data_ptr, source, size);
}


#endif /* CONFIG_USE_MEMORY_POOL */

#endif /*  CY_PPCA_SHM_DB_H */
