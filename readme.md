# Crazyflie 2.0 nRF51 Master Boot Switch [![CI](https://github.com/bitcraze/crazyflie2-nrf-mbs/workflows/CI/badge.svg)](https://github.com/bitcraze/crazyflie2-nrf-mbs/actions?query=workflow%3ACI)

This program is part of Crazyflie 2.0. It runs at nRF51 early startup and
measure the time the ON/OFF button is pressed. This program is also in
charge of reprogramming the bootloader and softdevice.

See readme.md of the crazyflie-nrf-firmware project for information about
the nrf flash architecture.

Compiling requires the nRF51_SDK and S110 packages.

        ./tools/build/download_deps.sh

will download the zip and unpack is.
If you want to download manually from the Nordic semiconductor website, you
will find the details in s110/readme.

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
