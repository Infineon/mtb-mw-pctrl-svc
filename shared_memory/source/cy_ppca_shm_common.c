/***************************************************************************//**
* \file cy_ppca_shm_common.c
* \brief
*   This file contains the common definition of the shared memory library
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

#include "cy_ppca_shm_common.h"

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 8.4', 7, \
                             'Shared memory variables are placed in .cy_ppca_shm section and accessed across cores')
#if CONFIG_USE_MEMORY_POOL
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile uint8_t shared_mem_pool[CONFIG_SHARED_MEM_POOL_SIZE];
#endif

#if CONFIG_USE_IPC_LOCK
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile IPC_STRUCT_Type* ipc_lock[CONFIG_USE_LOCK_MAX_CHAL_NUM];
#endif

#if CONFIG_USE_MUTEX_LOCK
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile mutex_t  mutex[CONFIG_USE_LOCK_MAX_CHAL_NUM];
#endif

/* Shared Variables for watchdog  */
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile uint32_t CORE0_WATCH;
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile uint32_t CORE1_WATCH;
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile uint32_t PREV_CORE0_WATCH;
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile uint32_t PREV_CORE1_WATCH;
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile uint32_t WDT_RESET_STATUS;
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile uint32_t CORE0_WDT_RESET_COUNT;
CY_SECTION_PPCA_SHM_SUB(SM_ICC_INT) volatile uint32_t CORE1_WDT_RESET_COUNT;
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 8.4')
