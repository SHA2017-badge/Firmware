# Change Log

## [Unreleased](https://github.com/SHA2017-badge/Firmware/tree/HEAD)

**Implemented enhancements:**

- Merge tsd's changes . . [\#2](https://github.com/SHA2017-badge/Firmware/pull/2) ([annejan](https://github.com/annejan))

**Closed issues:**

- uGFX 'toggle' input driver [\#15](https://github.com/SHA2017-badge/Firmware/issues/15)
- Intermittent build failures: no rule to make target .../bootloader.bin [\#13](https://github.com/SHA2017-badge/Firmware/issues/13)
- Add Roboto as ugfx external font [\#11](https://github.com/SHA2017-badge/Firmware/issues/11)
- Question: New distribution technique [\#8](https://github.com/SHA2017-badge/Firmware/issues/8)

**Merged pull requests:**

- Add Plain ESP32 option. [\#47](https://github.com/SHA2017-badge/Firmware/pull/47) ([annejan](https://github.com/annejan))
- added fonts for use in the weather app [\#46](https://github.com/SHA2017-badge/Firmware/pull/46) ([doebi](https://github.com/doebi))
- added missing fonts [\#45](https://github.com/SHA2017-badge/Firmware/pull/45) ([doebi](https://github.com/doebi))
- add badge deep-sleep support. [\#44](https://github.com/SHA2017-badge/Firmware/pull/44) ([basvs](https://github.com/basvs))
- remove all busy\_wait calls. it's now automatically done on next command. [\#43](https://github.com/SHA2017-badge/Firmware/pull/43) ([basvs](https://github.com/basvs))
- rename all gde\* methods to badge\_eink\_dev\_\* methods. [\#42](https://github.com/SHA2017-badge/Firmware/pull/42) ([basvs](https://github.com/basvs))
- Sneaking in some updates [\#41](https://github.com/SHA2017-badge/Firmware/pull/41) ([annejan](https://github.com/annejan))
- Support both pressed and released button events in uGFX [\#40](https://github.com/SHA2017-badge/Firmware/pull/40) ([basvs](https://github.com/basvs))
- Only have travis be a bad boy and use local mbedtls [\#39](https://github.com/SHA2017-badge/Firmware/pull/39) ([annejan](https://github.com/annejan))
- Emulator on mbedtls instead of axtls [\#38](https://github.com/SHA2017-badge/Firmware/pull/38) ([annejan](https://github.com/annejan))
- Latest micropython and micropython-lib updates [\#37](https://github.com/SHA2017-badge/Firmware/pull/37) ([annejan](https://github.com/annejan))
- Updated micropython to latest upstream \(ahead of micropython-esp32\) [\#36](https://github.com/SHA2017-badge/Firmware/pull/36) ([annejan](https://github.com/annejan))
- Simplified make steps  [\#35](https://github.com/SHA2017-badge/Firmware/pull/35) ([aczid](https://github.com/aczid))
- cleanup of badge\_button.h [\#34](https://github.com/SHA2017-badge/Firmware/pull/34) ([basvs](https://github.com/basvs))
- Travis build emulator, README updated on emulator compilation [\#33](https://github.com/SHA2017-badge/Firmware/pull/33) ([annejan](https://github.com/annejan))
- Missing badge\_button.h includes, latest micropython updates . . [\#31](https://github.com/SHA2017-badge/Firmware/pull/31) ([annejan](https://github.com/annejan))
- Moved button definitions to a header that doesn't depend on FreeRTOS queues [\#30](https://github.com/SHA2017-badge/Firmware/pull/30) ([aczid](https://github.com/aczid))
- added sdcard and vibrator modules; improved greyscale on depg display. [\#29](https://github.com/SHA2017-badge/Firmware/pull/29) ([basvs](https://github.com/basvs))
- Fixes in low-level drivers [\#28](https://github.com/SHA2017-badge/Firmware/pull/28) ([basvs](https://github.com/basvs))
- splitting high-level ugfx driver from low-level input drivers [\#27](https://github.com/SHA2017-badge/Firmware/pull/27) ([basvs](https://github.com/basvs))
- Demo listening for keypresses through GEVENT [\#26](https://github.com/SHA2017-badge/Firmware/pull/26) ([raboof](https://github.com/raboof))
- Allow overriding stuff in custom\_env.sh while still using defaults [\#25](https://github.com/SHA2017-badge/Firmware/pull/25) ([raboof](https://github.com/raboof))
- Use ugfx branch with freertos timer fix [\#24](https://github.com/SHA2017-badge/Firmware/pull/24) ([raboof](https://github.com/raboof))
- PermanentMarker36 [\#23](https://github.com/SHA2017-badge/Firmware/pull/23) ([Peetz0r](https://github.com/Peetz0r))
- ugfx touch input \(and some more goodies\) [\#22](https://github.com/SHA2017-badge/Firmware/pull/22) ([raboof](https://github.com/raboof))
- Enabled LEDs demo [\#21](https://github.com/SHA2017-badge/Firmware/pull/21) ([raboof](https://github.com/raboof))
- cleanups in components/epd and components/badge [\#20](https://github.com/SHA2017-badge/Firmware/pull/20) ([basvs](https://github.com/basvs))
- Many fixes and simplifications [\#19](https://github.com/SHA2017-badge/Firmware/pull/19) ([basvs](https://github.com/basvs))
- Bugfix: add \#defines to demo\_power.c [\#18](https://github.com/SHA2017-badge/Firmware/pull/18) ([basvs](https://github.com/basvs))
- Basvs charging status [\#17](https://github.com/SHA2017-badge/Firmware/pull/17) ([basvs](https://github.com/basvs))
- Attempt to stabalize build \(fixes \#13\) [\#16](https://github.com/SHA2017-badge/Firmware/pull/16) ([annejan](https://github.com/annejan))
- Log Make version [\#14](https://github.com/SHA2017-badge/Firmware/pull/14) ([raboof](https://github.com/raboof))
- Add support for SHA-style fonts \(fixes \#11\) [\#12](https://github.com/SHA2017-badge/Firmware/pull/12) ([raboof](https://github.com/raboof))
- ugfx integration [\#10](https://github.com/SHA2017-badge/Firmware/pull/10) ([raboof](https://github.com/raboof))
- Micropython [\#9](https://github.com/SHA2017-badge/Firmware/pull/9) ([annejan](https://github.com/annejan))
- Basvs update because it looks nicer :D [\#7](https://github.com/SHA2017-badge/Firmware/pull/7) ([annejan](https://github.com/annejan))
- greyscale image code improvement; add demo 3 [\#6](https://github.com/SHA2017-badge/Firmware/pull/6) ([annejan](https://github.com/annejan))
- Greyscale awesomeness!! [\#5](https://github.com/SHA2017-badge/Firmware/pull/5) ([annejan](https://github.com/annejan))
- Menu and demo's [\#4](https://github.com/SHA2017-badge/Firmware/pull/4) ([annejan](https://github.com/annejan))
- Compatibility and awesomes [\#3](https://github.com/SHA2017-badge/Firmware/pull/3) ([annejan](https://github.com/annejan))
- Update structure and add submodules. [\#1](https://github.com/SHA2017-badge/Firmware/pull/1) ([Petraea](https://github.com/Petraea))



\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*