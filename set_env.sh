cd esp-idf
export IDF_PATH=`pwd`
source add_path.sh
cd -

cd nodemcu-prebuilt-toolchains/esp32/xtensa-esp32-elf/bin/
export PATH=$PATH:`pwd`
cd -
