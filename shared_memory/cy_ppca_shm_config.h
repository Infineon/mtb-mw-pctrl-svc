/***************************************************************************//**
* \file cy_ppca_shm_config.h
* \brief
*   This is a template configuration file for the shared memory library.
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

#ifndef CY_PPCA_SHM_CONFIG_H
#define CY_PPCA_SHM_CONFIG_H

#warning This is a template. Copy this file to your project and remove this line.

// Define the total size of the shared memory pool
#define CONFIG_USE_MEMORY_POOL              0
#define CONFIG_SHARED_MEM_POOL_SIZE         1024

// Use double buffer mechanism
#define CONFIG_USE_DOUBLE_BUFFER            0

// Use locking mechanism
#define CONFIG_USE_IPC_LOCK                 0
#define CONFIG_USE_MUTEX_LOCK               1

// use triple buffer mechanism
#define CONFIG_USE_TRIPLE_BUFFER            0

#endif /* CY_PPCA_SHM_CONFIG_H */
