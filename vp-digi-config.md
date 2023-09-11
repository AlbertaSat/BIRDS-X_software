# vp-digi-config.md
The vp-digi section of the project must be configured using its interface.

## Important Notes
* Run `save` after you set the config. Very important.

## `print` Output from VP-Digi
```
Callsign: VA6XXX-0
Destination: APNV01
TXDelay (ms): 1000
TXTail (ms): 30
Quiet time (ms): 300
UART1 baudrate: 9600
UART2 baudrate: 9600
DAC type: PWM
Flat audio input: yes
Beacon 0: On, Iv: 100, Dl: 1, WIDE1-1, >Hello from JASPER satellite
Beacon 1: Off, Iv: 0, Dl: 0, no path, 
Beacon 2: Off, Iv: 0, Dl: 0, no path, 
Beacon 3: Off, Iv: 0, Dl: 0, no path, 
Beacon 4: Off, Iv: 0, Dl: 0, no path, 
Beacon 5: Off, Iv: 0, Dl: 0, no path, 
Beacon 6: Off, Iv: 0, Dl: 0, no path, 
Beacon 7: Off, Iv: 0, Dl: 0, no path, 
Digipeater: On
Alias WIDE: On, max: 2, rep: 3, untraced, unfiltered
Alias : Off, max: 0, rep: 0, untraced, unfiltered
Alias : Off, max: 0, rep: 0, untraced, unfiltered
Alias : Off, max: 0, rep: 0, untraced, unfiltered
Alias -0: Off, untraced, unfiltered
Alias -0: Off, untraced, unfiltered
Alias -0: Off, untraced, unfiltered
Alias -0: Off, untraced, unfiltered
Anti-duplicate buffer hold time (s): 30
Callsign filter type: blacklist
Callsign filter list: 0 entries
Auto-reset every (h): disabled
KISS monitor: Off
Allow non-APRS frames: Off
```

* Note that the `beacon 0` text is overwritten with telemetry in the firmware.

## Configuration Script
Run this config script in RealTerm or similar to set the configuration above.

```
config
call VA6XXX
dest APNV01
quiet 300
txdelay 1000
beacon 0 on
beacon 0 iv 1
beacon 0 dl 15
beacon 0 path WIDE1-1
beacon 0 data >HIJASPER
pwm on
flat on
save
monitor
beacon 0
```
