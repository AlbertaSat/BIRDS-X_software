# BIRDS-X_Firmware
Firmware for the JASPER team's BIRDS-X PCB submission.

## Acknowledgement
This firmware is based heavily off the [vp-digi firmware](https://github.com/sq8vps/vp-digi), with modifications. It was forked at v1.2.6, from the [nousb branch](https://github.com/sq8vps/vp-digi/tree/nousb)

## Sources
* [vp-digi v1 docs, at fork point](https://web.archive.org/web/20210411062942/https://sq8l.pzk.pl/index.php/vp-digi-cheap-and-functional-aprs-digipeater-controller-with-kiss-modem/)

## Flashing and Setup
1. Connect ST-Link.
2. Compile and flash.
3. For the first time, set the configuration contained in `vp-digi-config.md`. These configurations are saved in the flash memory.


## UART Channels

Each UART port can be in 1 of 5 modes:
1. KISS
2. TERMINAL
3. MONITOR
4. DRA (for DRA818V transceiver) [Default for UART2]
5. BOSS (for Mission Boss comms) [Default for UART1]

Both ports are at 9600 baud, by default.

### Port Assignments
* In deployment, UART1 is to the Mission Boss, and UART2 is to the DRA818V transceiver

### Mission Boss Commands
#### Examples
* Echo: `0xE0 0x10 0 0 0 0 0 0 0xED`
