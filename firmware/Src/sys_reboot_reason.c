// Source: https://stackoverflow.com/a/54728664

#include "sys_reboot_reason.h"

// FIXME: make this work

#include "main.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"

/// @brief      Obtain the STM32 system reset cause
/// @param      None
/// @return     The system reset cause
reset_cause_t reset_cause_get(void)
{
    reset_cause_t reset_cause;

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
    {
        reset_cause = RESET_CAUSE_LOW_POWER_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
    {
        reset_cause = RESET_CAUSE_WINDOW_WATCHDOG_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
    {
        reset_cause = RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
    {
        // This reset is induced by calling the ARM CMSIS 
        // `NVIC_SystemReset()` function!
        reset_cause = RESET_CAUSE_SOFTWARE_RESET; 
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
    {
        reset_cause = RESET_CAUSE_POWER_ON_POWER_DOWN_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
    {
        
        reset_cause = RESET_CAUSE_EXTERNAL_RESET_PIN_RESET;
    }
    // Needs to come *after* checking the `RCC_FLAG_PORRST` flag in order to
    // ensure first that the reset cause is NOT a POR/PDR reset. See note
    // below. 
    /*
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))
    {
        reset_cause = RESET_CAUSE_BROWNOUT_RESET;
    }*/
    else
    {
        reset_cause = RESET_CAUSE_UNKNOWN;
    }

    // Clear all the reset flags or else they will remain set during future
    // resets until system power is fully removed.
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return reset_cause; 
}

// Note: any of the STM32 Hardware Abstraction Layer (HAL) Reset and Clock
// Controller (RCC) header files, such as 
// "STM32Cube_FW_F7_V1.12.0/Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_hal_rcc.h",
// "STM32Cube_FW_F2_V1.7.0/Drivers/STM32F2xx_HAL_Driver/Inc/stm32f2xx_hal_rcc.h",
// etc., indicate that the brownout flag, `RCC_FLAG_BORRST`, will be set in
// the event of a "POR/PDR or BOR reset". This means that a Power-On Reset
// (POR), Power-Down Reset (PDR), OR Brownout Reset (BOR) will trip this flag.
// See the doxygen just above their definition for the 
// `__HAL_RCC_GET_FLAG()` macro to see this:
//      "@arg RCC_FLAG_BORRST: POR/PDR or BOR reset." <== indicates the Brownout
//      Reset flag will *also* be set in the event of a POR/PDR. 
// Therefore, you must check the Brownout Reset flag, `RCC_FLAG_BORRST`, *after*
// first checking the `RCC_FLAG_PORRST` flag in order to ensure first that the
// reset cause is NOT a POR/PDR reset.


/// @brief      Obtain the system reset cause as an ASCII-printable name string 
///             from a reset cause type
/// @param[in]  reset_cause     The previously-obtained system reset cause
/// @return     A null-terminated ASCII name string describing the system 
///             reset cause
const char * reset_cause_get_name(reset_cause_t reset_cause)
{
    const char * reset_cause_name = "TBD";

    switch (reset_cause)
    {
        case RESET_CAUSE_UNKNOWN:
            reset_cause_name = "UNKNOWN";
            break;
        case RESET_CAUSE_LOW_POWER_RESET:
            reset_cause_name = "LOW_POWER_RESET";
            break;
        case RESET_CAUSE_WINDOW_WATCHDOG_RESET:
            reset_cause_name = "WINDOW_WATCHDOG_RESET";
            break;
        case RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET:
            reset_cause_name = "INDEPENDENT_WATCHDOG_RESET";
            break;
        case RESET_CAUSE_SOFTWARE_RESET:
            reset_cause_name = "SOFTWARE_RESET";
            break;
        case RESET_CAUSE_POWER_ON_POWER_DOWN_RESET:
            reset_cause_name = "POWER-ON_RESET (POR) / POWER-DOWN_RESET (PDR)";
            break;
        case RESET_CAUSE_EXTERNAL_RESET_PIN_RESET:
            reset_cause_name = "EXTERNAL_RESET_PIN_RESET";
            break;
        case RESET_CAUSE_BROWNOUT_RESET:
            reset_cause_name = "BROWNOUT_RESET (BOR)";
            break;
    }

    return reset_cause_name;
}




////////////////////////////////////////////////////////////////////////
#include <stdint.h>

// Function to get the last reset reason as a string
const char* get_last_reset_reason_str(void) {
    // Initialize the RCC peripheral
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable the PWR peripheral clock
    PWR->CR |= PWR_CR_CWUF;            // Clear the WUF flag
    PWR->CR |= PWR_CR_CSBF;            // Clear the SBF flag
    RCC->CSR |= RCC_CSR_RMVF;          // Clear the reset flags

    // Check the reset reason
    if (RCC->CSR & RCC_CSR_WWDGRSTF) {
        return "Watchdog Reset";
    }
    if (RCC->CSR & RCC_CSR_IWDGRSTF) {
        return "Independent Watchdog Reset";
    }
    if (RCC->CSR & RCC_CSR_SFTRSTF) {
        return "Software Reset";
    }
    if (RCC->CSR & RCC_CSR_PORRSTF) {
        return "Power-On Reset";
    }
    if (RCC->CSR & RCC_CSR_PINRSTF) {
        return "External Reset (Pin)";
    }/*
    if (RCC->CSR & RCC_CSR_BORRSTF) {
        return "Brownout Reset";
    }*/

    // Clear the reset flags
    RCC->CSR |= RCC_CSR_RMVF;

    // Default case if no reset reason is found
    return "Unknown Reset Reason";
}



