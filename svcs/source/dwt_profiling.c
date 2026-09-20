/***************************************************************************//**
* \file dwt_profiling.c
* \brief
*   This file contains the inline function, macros and constants for profiling
*   implementation using DWT block in Cortex-M cores.
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

#include <stdint.h>
#include "dwt_profiling.h"

/* Arrays to keep start and stop tick count */
uint32_t dwt_start_count = 0U;
uint32_t dwt_stop_count = 0U;
