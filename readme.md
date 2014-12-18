Crazyflie 2.0 nRF51 Master Boot Switch
======================================

This program is part of Crazyflie 2.0. It runs at nRF51 early startup and
measure the time the ON/OFF button is pressed. This program is also in
charge of reprogramming the bootloader and softdevice.

See readme.md of the crazyflie-nrf-firmware project for information about
the nrf flash architecture.

Compiling this project require to download and extract the Nordic Semiconductor
S110 BLE start package in the s110 folder. See s110/readme for exact version.

License
-------

See files header for license. The main file is licensed under MIT license.

Downloading the nordic S110 softdevice require to aquire one of the Nordic
Semiconductor development kit. Discussion is in progress with Nordic to solve
this situation.

Compiling
---------

To compile arm-none-eabi- tools from https://launchpad.net/gcc-arm-embedded
should be in the path.
