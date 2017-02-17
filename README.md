ESP32 AMEPD test for SHA2017Badge
=================================

Read [project documentation](https://orga.sha2017.org/index.php/Projects:Badge) and get involved.

Project updates:

* [Firmware](https://github.com/SHA2017-badge/Firmware)
* [PCB](https://github.com/SHA2017-badge/PCB)

Based on template application for [Espressif IoT Development Framework (ESP-IDF)](https://github.com/espressif/esp-idf
). Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0 as described in the file LICENSE.

[![Build Status](https://travis-ci.org/SHA2017-badge/Firmware.svg?branch=master)](https://travis-ci.org/SHA2017-badge/Firmware)

Debian prerequisites
--------------------

```
sudo apt-get install libncurses5-dev flex bison gperf
```

Compiling and flashing
----------------------

```
git submodule update --init --recursive
make defconfig
make flash
```
