ESP32 AMEPD test for SHA2017Badge
=================================

Read `project documentation <https://orga.sha2017.org/index.php/Projects:Badge>`__ and get involved.

This firmware is tentative
--------------------------

Unfortunately on SHA Rev0.0.1 Dev pin 20 has to be connected to 23 (solder bridge).
The screen reset pin is currently attached to IO20 (in practice NC?)

Currently debugging options:

- Is pin 20 pulled down/up ?
- Check voltages . .
- Are all pins firing as they should?
- Can we get the "old" screen breakout to work with this firmware?
- Can we get the screen to work by feeding it data from Arduino?

This might display some images on the e-ink screen of the prototype . .

Based on template application for `Espressif IoT Development Framework`_ (ESP-IDF).
Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0 as described in the file LICENSE.

.. _Espressif IoT Development Framework: https://github.com/espressif/esp-idf

.. image:: https://travis-ci.org/SHA2017-badge/Firmware.png?branch=master
    :target: https://travis-ci.org/SHA2017-badge/Firmware
