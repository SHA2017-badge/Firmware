ESP32 AMEPD test for SHA2017Badge
=================================

Read `project 
documentation <https://orga.sha2017.org/index.php/Projects:Badge>`__ and
get involved.

This firmware is theoretic
--------------------------

Please test ASAP on real hardware :grimacing:

This should theoretically display some images on the e-ink screen.

Based on template application for `Espressif IoT Development Framework`_ (ESP-IDF). 
Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0 as described in the file LICENSE.

.. _Espressif IoT Development Framework: https://github.com/espressif/esp-idf

::

        cd ..
        git clone https://github.com/espressif/esp-idf.git
        wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-59.tar.gz
        tar xzvf xtensa-esp32-elf-linux64-1.22.0-59.tar.gz
        cd -
        make menuconfig
        make
        make flash

TODO: More documentation

.. image:: https://travis-ci.org/SHA2017-badge/Firmware.png?branch=master
    :target: https://travis-ci.org/SHA2017-badge/Firmware
