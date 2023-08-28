# BIRDS-X_Firmware
Firmware for the JASPER team's BIRDS-X PCB submission.

## UART Channels

Each UART port can be in 1 of _5_ modes:
1. KISS
2. TERMINAL
3. MONITOR
4. DRA (for DRA818V transceiver)
5. BOSS (for Mission Boss comms)

Both ports are at 9600 baud, by default.

### Port Assignments
* In deployment, UART1 is to the Mission Boss, and UART2 is to the DRA818V transceiver

