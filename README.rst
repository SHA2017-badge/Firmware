ESP32 AMEPD test for SHA2017Badge
=================================

Read `project documentation <https://orga.sha2017.org/index.php/Projects:Badge>`__ and get involved.

Project updates: https://github.com/SHA2017-badge/Firmware

Hardware patch: PIN 20 to PIN 23

Based on template application for `Espressif IoT Development Framework`_ (ESP-IDF).
Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0 as described in the file LICENSE.

.. _Espressif IoT Development Framework: https://github.com/espressif/esp-idf

.. image:: https://travis-ci.org/SHA2017-badge/Firmware.png?branch=master
    :target: https://travis-ci.org/SHA2017-badge/Firmware

Ubuntu Prerequisites
--------------------

prequisites::
    sudo apt-get install libncurses5-dev flex bison gperf

Compiling and flashing
----------------------

compiling::
    git submodule update -i -r
    make defconfig
    make
flashing::
    make flash
