Firmware SHA2017Badge
=====================

Read [project documentation](https://wiki.sha2017.org/w/Projects:Badge)
and get involved.

Project updates:

* [Wiki](https://wiki.sha2017.org/w/Projects:Badge)
* [Documentation](https://wiki.sha2017.org/w/Projects:Badge/Documentation)
* [Firmware](https://github.com/SHA2017-badge/Firmware)
* [PCB](https://github.com/SHA2017-badge/PCB)
* [Pad](https://pad.sha2017.org/p/badge)
* [Changelog](CHANGELOG.md)

Copyright (C) 2017 SHA2017 Badge team.
Based on template application for [Espressif IoT Development Framework (ESP-IDF)](https://github.com/espressif/esp-idf).
Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0
as described in the file LICENSE.

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d73b02ee522942b8ae1244aa2ee62f43)](https://www.codacy.com/app/annejan/Firmware?utm_source=github.com&utm_medium=referral&utm_content=SHA2017-badge/Firmware&utm_campaign=badger)
[![Build Status](https://travis-ci.org/SHA2017-badge/Firmware.svg?branch=master)](https://travis-ci.org/SHA2017-badge/Firmware)

Debian prerequisites
--------------------

```
sudo apt-get install libncurses5-dev flex bison gperf python-serial libffi-dev libsdl2-dev libmbedtls-dev perl
```

Compiling and flashing
----------------------

```
git submodule update --init --recursive
source set_env.sh
make defconfig
make -j5
```

Optional configuration
-------------
```
make menuconfig
```

Compiling MicroPython
----------------------------------

```
make -j5 -C micropython/mpy-cross
make -j5 -C micropython/esp32/
```

Flashing MicroPython
----------------------------------

```
cd auto-flasher
cp ../micropython/esp32/build/application.bin firmware/sha2017-badge.bin
./auto_flash.pl /dev/ttyUSB0
```

Interacting via serial
----------------------
```
make monitor
```

MicroPython
-----------
```
import badge
badge.eink_init()
badge.display_picture(0,-1)
import ugfx
ugfx.init()
ugfx.demo("HACKING")
ugfx.clear(badge.BLACK)
ugfx.thickline(1,1,100,100,badge.WHITE,10,5)
ugfx.box(30,30,50,50,badge.WHITE)
ugfx.string(150,25,"STILL","Roboto_BlackItalic24",badge.WHITE)
ugfx.string(130,50,"Hacking","PermanentMarker22",badge.WHITE)
len = ugfx.get_string_width("Hacking","PermanentMarker22")
ugfx.line(130, 72, 144 + len, 72, badge.WHITE)
ugfx.line(140 + len, 52, 140 + len, 70, badge.WHITE)
ugfx.string(140,75,"Anyway","Roboto_BlackItalic24",badge.WHITE)
ugfx.flush()
```
More info on the [MicroPython badge features](https://wiki.sha2017.org/w/Projects:Badge/MicroPython)

Badge Emulator
--------------
```
make -C micropython/unix
cd micropython/unix
./micropython ../../examples/Game\ of\ Life/game_of_life.py
```
