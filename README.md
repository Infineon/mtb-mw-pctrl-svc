# ModusToolbox Power Control Service Library

## Overview

The power control service library layer provides a set of common services and utilities that enables fast integration with power control applications in multi-core systems such as Infineon PSOC™ C3M(P)8 MCU.

Key components and capabilities include:
- **Inter-Core Communication using Shared Memory**

    Enable fast and direct data exchange between cores using a configurable shared memory region. The library abstracts the complexity of memory management and provides APIs for initializing and accessing shared memory blocks.

- **Inter-Core Synchronization**

    Implements lightweight mutex objects based on atomic operations, allowing safe and efficient synchronization between cores without the overhead of traditional OS-based mutexes.

- **Profiling Support**

    Provides utilities for performance profiling using the CPU's DWT CYCCNT register, enabling developers to measure execution time and optimize inter-core interactions.

- **Watchdog-like Services**

    Includes a watchdog-like mechanism for PPCA (Programmable Peripheral Control Architecture) cores, helping to monitor core activity and ensure system reliability.

- **Basic Scheduling Services**

    Provides a lightweight cooperative task scheduler driven by a SysTick for both periodic and one-shot tasks.

- **Stack Monitoring Services**

    Provides utilities for stack memory usage, enabling developers to check the highest watermark.

While the primary purpose of the library is for middleware within ModusToolbox™, some of the features can also be reused by application code directly.


## Quick Start

The quick start guide provides steps to add this library into a multi-core template project.

1. Add `mtb-mw-pctrl-svc` to all projects by adding a `mtb-mw-pctrl-svc.mtb` file under `deps` folder.
    ```
    mtb:mtb-mw-pctrl-svc#release-v1.0.0#$$ASSET_REPO$$/mtb-mw-pctrl-svc/release-v1.0.0

    # replace the release-v1.0.0 with the latest version
    ```
    Make sure to run `make getlibs` to pull the library into local workspace

2. Created a `shared` folder under the application root directory.
   ```
    multi-core-template
    |-- bsp/
    |-- main_cm33_ns/
        |-- deps
        |-- main.c
        |-- Makefile    <== project Makefile
    |-- main_cm33_s/
        |-- deps
        |-- main.c
        |-- Makefile    <== project Makefile
    |-- ppca_cm33_0/
        |-- deps
        |-- main.c
        |-- Makefile    <== project Makefile
    |-- ppca_cm33_1/
        |-- deps
        |-- main.c
        |-- Makefile    <== project Makefile
    |-- shared/         <== shared folder
    |-- common_app.mk
    |-- common.mk       <== common Makefile for all projects
    |-- Makefile        <== application top level Makefile
   ```

3. Update `common.mk` to include the `shared` folder in the build
   ```Makefile
    # By default the build system automatically looks in the Makefile's directory
    # tree for source code and builds it. The SOURCES variable can be used to
    # manually add source code to the build process from a location not searched
    # by default, or otherwise not found by the build system.
    SEARCH += ./../shared
   ```

4. Copy *`cy_ppca_shm_config.h`* file from `mtb-mw-pctrl-svc/shared_memory/` folder to `shared`

5. Open the copied `cy_ppca_shm_config.h` file and remove `#warning This is a template...` line.

6. Add `shared_memory_vars.h/.c` in `shared` folder where it contains all variables to be used in shared memory regions
   ```c
   CY_SECTION_PPCA_SHM volatile uint32_t sm_var;
   ```
7.  Initialize the shared memory by calling `Cy_PPCA_SharedMem_Init()` in the main core.

Note that `Cy_PPCA_SharedMem_Init()` doesn't reset all variable values. Application will need to initialized all shared variables to default values.

## Design & Architecture

### Inter-Core Communication using Shared Memory
The shared memory feature in this library enables fast and direct data exchange between processor cores in a multi-core system by providing a configurable region of memory accessible to all participating cores. The library abstracts the complexity of shared memory management and offers APIs for initializing, reading, and writing shared memory variables and blocks.

> The library does not own or define data types for application data. It only provides the mechanism to define shared variables, interfaces to access 32-bit data or block (more than 32-bit) data.

Key aspects include:

- Configurable Memory Region: Shared variables are placed in a dedicated memory section (e.g., .cy_ppca_shm), ensuring all cores can access them.
- Simplified variables declarations on shared memory: Eliminating all manual address definitions, especially helpful for systems such as PSOC™ C3M(P)8 MCUs where different address mappings are applied for different cores.
- Memory Pool Management: Optionally uses a dynamic memory pool for allocating shared buffers at runtime.
- Synchronization: Integrates with lightweight mutexes or IPC locks to ensure safe concurrent access to block shared data (e.g more that 32-bit)

This feature is essential for inter-core communication, allowing cores to share data structures, signals, or messages efficiently and reliably in real-time embedded applications.

> Note on PSOC™ C3M(P)8 MCUs, for data transfer (including memcpy/memset) between main core and PPCA cores from shared memory regions must be done in full 32-bit words and data must be located on 4-byte aligned address. Unaligned data address or byte access will cause unpredictable results and may result in a fault.

#### Configurations
The shared memory module include the following configurations:

```c
/* For block memory access, configuration to use mutex or hardware IPC */
#define CONFIG_USE_IPC_LOCK                 0
#define CONFIG_USE_MUTEX_LOCK               1

/* Number of built-in lock channels */
#define CONFIG_USE_LOCK_MAX_CHAL_NUM        3

/* Optional memory pool for block data access */
#define CONFIG_USE_MEMORY_POOL              0
#define CONFIG_SHARED_MEM_POOL_SIZE         1024
```

#### API Interfaces
The library module provide the following interfaces:
For variables declarations:
```c
    /* Define variable to be allocated to shared memory region */
    CY_SECTION_PPCA_SHM volatile uint32_t sm_var;
```

For shared memory initialization:
```c
    /* This only needs to be called once from system main core */
    void Cy_PPCA_SharedMem_Init(void);
```

For 32-bit data access:
```c
    /* Single 32-bit data access */
    uint32_t Cy_PPCA_SharedMem_Read(volatile uint32_t* var_addr);
    void     Cy_PPCA_SharedMem_Write(volatile uint32_t* var_addr, uint32_t value)

    /* Single 32-bit data bit access with atomic operation */
    uint32_t Cy_PPCA_SharedMem_SetBits(volatile uint32_t* var_addr, uint32_t bitmask);
    uint32_t Cy_PPCA_SharedMem_ClrBits(volatile uint32_t* var_addr, uint32_t bitmask);
    bool     Cy_PPCA_SharedMem_CheckClrBits(volatile uint32_t* var_addr, uint32_t bitmask);
```

For block data (more than 32-bit) access without mempool:
```c
    /* Channel type definition */
    typedef uint32_t cy_ppca_shm_channel_t;
    /* Lock acquisition and release */
    void Cy_PPCA_SharedMem_Blk_LockAcquire(cy_ppca_shm_channel_t chnl);
    void Cy_PPCA_SharedMem_Blk_LockRelease(cy_ppca_shm_channel_t chnl);
    /* Block data access */
    void Cy_PPCA_SharedMem_Blk_Write(volatile void* sm_blk, const void* data_ptr, uint32_t size, cy_ppca_shm_channel_t chnl);
    void Cy_PPCA_SharedMem_Blk_Read(volatile void* sm_blk, void* data_ptr, uint32_t size, cy_ppca_shm_channel_t chnl);
```

For block data (more than 32-bit) access with mempool:
```c
    /* Channel type definition */
    typedef uint32_t cy_ppca_shm_channel_t;

    /* Block type definition */
    typedef struct
    {
        cy_ppca_shm_channel_t        chnl;
        void*                        data;
    } cy_ppca_shm_blk_t;

    /* Block data access */
    void Cy_PPCA_SharedMem_Blk_Init(volatile cy_ppca_shm_blk_t* blk_ptr, uint32_t size, cy_ppca_shm_channel_t chnl);
    void Cy_PPCA_SharedMem_Blk_Write(volatile cy_ppca_shm_blk_t* sm_blk, const void* data_ptr, uint32_t size);
    void Cy_PPCA_SharedMem_Blk_Read(volatile cy_ppca_shm_blk_t* sm_blk, void* data_ptr, uint32_t size);
```

#### Example Code Snippet
- In the shared variable definition file (e.g shared/sm_var.c)
```c
    CY_SECTION_PPCA_SHM volatile uint32_t sm_var;
```
- One the system main CPU:
```c
    /* Initialize shared memory */
    Cy_System_PPCA_RAM_Enable();
    Cy_PPCA_SharedMem_Init();
    sm_var = INITIAL_VALUE;

    /* Perform read/write */
    Cy_PPCA_SharedMem_Write(&sm_var, NEW_VALUE);
```
- On other cores:
```c
    /* Perform read/write */
    value = Cy_PPCA_SharedMem_Read(&sm_var);
```


### Inter-Core Synchronization using Mutex
On multi-core system, inter-core synchronization ensures that different processor cores can safely coordinate access to shared resources, such as memory or peripherals, without conflicts or data corruption. Besides the IPC hardware block available in MCU, lightweight mutexes based on atomic operations are provided from this library to provide efficient and reliable synchronization between cores.

An atomic operation is a type of operation that completes in a single step relative to other threads or cores. This means it cannot be interrupted or observed in an incomplete state, ensuring data consistency when multiple cores access shared resources.

On ARM Cortex-M processors, atomic operations for inter-core synchronization are typically implemented using exclusive load/store instructions (LDREX/STREX). For atomic operations to work on multi-core systems, these instructions work together with the "Global Exclusive Monitor" which is a hardware feature that tracks exclusive accesses across all cores in a multi-core system.
- When a core executes LDREX, it marks the address as "exclusively accessed."
- If no other core writes to that address, a subsequent STREX by the same core will succeed, atomically updating the value.
- If another core writes to the address in between, the STREX will fail, and the operation must be retried.

> Note that the address must be accessible by all cores, e.g located in a shared memory region

By enabling the Global Exclusive Monitor, the system ensures that atomic operations using LDREX/STREX are reliable across all cores, making them suitable for implementing mutexes and other synchronization primitives in shared memory.

#### PSOC™ C3M(P)8 MCUs
On PSOC™ C3M(P)8 MCUs, the main core and PPCA cores are all Cortex-M33. Bit 29 of [Auxiliary Control Register of System Control](https://developer.arm.com/documentation/100230/0100/System-Control/Auxiliary-Control-Register) is used to enable "Global Exclusive Monitor" and this bit is enabled during at Rest Handler.

#### API Interfaces
The library module provide the following interfaces:
```c
    /* Mutex definition */
    typedef uint32_t mutex_t;

    /* Mutex lock */
    bool mutex_lock(volatile mutex_t* mutex);

    /* Mutex unlock */
    void mutex_unlock(volatile mutex_t* mutex);
```
#### Example Code Snippet
```c
    /* Mutex declaration */
    CY_SECTION_PPCA_SHM volatile mutex_t uart_mutex;

    /* Mutex Initialization */
    uart_mutex = MUTEX_UNLOCKED;

    /* Locking */
    while(mutex_lock(&uart_mutex) == false) {}

    /* Exclusive Access */
    // do something

    /* Unlocking */
    mutex_unlock(&uart_mutex);
```

### Profiling Support
The profiling interfaces leverages the built-in DWT (Data Watchpoint and Trace) unit in ARM Cortex-M processor cores to measure code execution time with high precision. By enabling the DWT cycle counter, developers can record the number of CPU cycles elapsed between specific code sections, allowing accurate performance analysis. This method is lightweight and does not require external tools or significant software overhead.

#### API Interface
The library module provide the following interfaces:
```c
    /* Profiling initialization */
    void svc_dwt_profile_init(void);

    /* Start profiling */
    void svc_dwt_profile_start(void);

    /* Stop profiling */
    void svc_dwt_profile_stop(void);

    /* Reset the cycle counter */
    void svc_dwt_profile_reset_cyccnt(void);

    /* Read profiling results */
    uint32_t svc_dwt_profile_read(void);
```
#### Example Code Snippet
On every core of the system,
```c
    uint32_t count = 0;

    /* dwt init */
    svc_dwt_profile_init();

    /* start clock counting */
    svc_dwt_profile_start();

    /* execution */
    // do something

    /* stop clock counting */
    svc_dwt_profile_stop();

    /* Read the count */
    count = svc_dwt_profile_read();
```

### Watchdog-like Services
This feature is created for PSOC™ C3M(P)8 MCUs ONLY.

#### PSOC™ C3M(P)8 MCUs
The PPCA watchdog service provides a mechanism to monitor the activity and health of PPCA cores in a multi-core system. It uses the MCU's multi-counter watchdog timer (MCWDT) to detect if a core becomes unresponsive or fails to periodically reset its watchdog counter. If a core does not reset its counter within a configured protection time, the watchdog can trigger an interrupt or system reset, helping to ensure system reliability and recover from software faults or hangs. The service includes APIs for initialization, reconfiguration, status checking, and periodic resetting, and is typically managed by the main core.

#### API Interface
```c
    /* Initialize ppca watchdog services.
     *
     * This should be done on the system main core only
     */
    void ppca_wdt_init(uint32_t core0_protection_time, ppca_wdt_mode_t core0_wdt_mode, ppca_wdt_cb_t core0_cb,
                       uint32_t core1_protection_time, ppca_wdt_mode_t core1_wdt_mode, ppca_wdt_cb_t core1_cb);

    /* Start ppca watchdog services.
     *
     * This should be done on the system main core only
     */
    void ppca_wdt_enable(void);

    /* Reconfigure ppca watchdog timer for a specific core.
     *
     * This should be done on the system main core only
     */
    void ppca_wdt_reconfigure(cy_en_ppca_core_t ppca_core, uint32_t protection_time);

    /* Check the reason for device restart.
     *
     * This should be done on the system main core only
     */
    ppca_wdt_reset_t ppca_wdt_check_restart_reason(void);

    /* Clear the reason for device restart.
     *
     * This should be done on the system main core only
     */
    void ppca_wdt_clear_restart_reason(void);

    /* PPCA watchdog interrupt handler.
     *
     * This should be done on the system main core only
     */
    void ppca_wdt_handler(void);

    /* Reset watchdog timer. This requires periodic execution.
     *
     * This should be done on all cores.
     */
    void ppca_wdt_reset(void);
```
#### Example code snippet
- On system main core
```c
    /* Check restart reason on boot */
    ppca_wdt_reset_t reason = ppca_wdt_check_restart_reason();

    /* Clear reset reason */
    ppca_wdt_clear_restart_reason();

    /* Initialize ppca wdt services */
    ppca_wdt_init(CORE0_TIMEOUT_US, PPCA_WDT_MODE_INT_RESET, NULL,
                  CORE1_TIMEOUT_US, PPCA_WDT_MODE_INT_RESET, NULL);

    /* Enable */
    ppca_wdt_enable();

    /* In the program main loop */
    ppca_wdt_reset();
```

- On PPCA cores
```c
    /* In the program main loop */
    ppca_wdt_reset();
```



### Basic Scheduling Services
This feature is created for PSOC™ C3M(P)8 MCUs ONLY.

A lightweight cooperative task scheduler driven by a periodic tick (typically SysTick). Tasks are registered with a period or one-shot delay and dispatched from the main loop via `svc_sched_dispatch`. The scheduler supports periodic and one-shot tasks with simple priority ordering (lower value = higher priority).

> Note: The scheduler is cooperative — task functions must return quickly and must not block.

#### API Interface
```c
    /* Initialize the scheduler with a tick rate in Hz */
    bool svc_sched_init(uint32_t tick_hz);

    /* Add a periodic task (runs every period_ms milliseconds) */
    svc_task_id_t svc_sched_add_periodic_ms(svc_task_fn_t fn, void* arg, uint32_t period_ms, uint8_t priority);

    /* Add a one-shot task (runs once after delay_ms milliseconds) */
    svc_task_id_t svc_sched_add_oneshot_ms(svc_task_fn_t fn, void* arg, uint32_t delay_ms, uint8_t priority);

    /* Enable / disable a registered task */
    void svc_sched_enable(svc_task_id_t task_id);
    void svc_sched_disable(svc_task_id_t task_id);

    /* Call from the main loop to run due tasks */
    void svc_sched_dispatch(void);
```

#### Example Code Snippet
```c
    /* Define a task function */
    void my_periodic_task(void* arg)
    {
        // do something periodically
    }

    /* Initialize the scheduler at 1000 Hz (1 ms tick) */
    svc_sched_init(1000);

    /* Register a periodic task running every 100 ms at priority 0 */
    svc_task_id_t task_id = svc_sched_add_periodic_ms(my_periodic_task, NULL, 100, 0);

    /* Main loop */
    while (1)
    {
        svc_sched_dispatch();
    }
```


### Stack Monitoring Services
This feature is created for PSOC™ C3M(P)8 MCUs ONLY.

The stack monitor places a guard word at the stack limit and fills the remaining stack region with a known fill pattern at initialization. Periodically calling the update function scans the stack region to report the highest observed stack usage (high-water mark), allowing developers to detect potential stack overflows and optimize stack allocation.

#### API Interface
```c
    /* Initialize the stack monitor — fill stack region with known pattern.
     * Call once early at boot, before significant stack usage has occurred. */
    void svc_stack_mon_init(void);

    /* Get total monitored stack size in bytes */
    size_t svc_stack_mon_get_total(void);

    /* Scan stack and return the high-water mark (peak usage) in bytes */
    size_t svc_stack_mon_update(void);
```

#### Example Code Snippet
```c
    /* Initialize early in main, before the stack is heavily used */
    svc_stack_mon_init();

    /* Periodically check high-water mark */
    size_t peak_usage = svc_stack_mon_update();
    size_t total      = svc_stack_mon_get_total();
```



## More information

For more information, refer to the following documents:

* [ModusToolbox Power Control Service Library API Reference Guide](https://infineon.github.io/mtb-mw-pctrl-svc/html/index.html)
* [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software)
* [Infineon Technologies AG](https://www.infineon.com)


---
© 2025-2026, Infineon Technologies AG, or an affiliate of Infineon Technologies AG. All rights reserved.
This software, associated documentation and materials ("Software") is owned by Infineon Technologies AG or one of its affiliates ("Infineon") and is protected by and subject to worldwide patent protection, worldwide copyright laws, and international treaty provisions. Therefore, you may use this Software only as provided in the license agreement accompanying the software package from which you obtained this Software. If no license agreement applies, then any use, reproduction, modification, translation, or compilation of this Software is prohibited without the express written permission of Infineon.
Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A SPECIFIC USE/PURPOSE OR MERCHANTABILITY. Infineon reserves the right to make changes to the Software without notice. You are responsible for properly designing, programming, and testing the functionality and safety of your intended application of the Software, as well as complying with any legal requirements related to its use. Infineon does not guarantee that the Software will be free from intrusion, data theft or loss, or other breaches ("Security Breaches"), and Infineon shall have no liability arising out of any Security Breaches. Unless otherwise explicitly approved by Infineon, the Software may not be used in any application where a failure of the Product or any consequences of the use thereof can reasonably be expected to result in personal injury.
