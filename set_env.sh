cd esp-idf
export IDF_PATH=`pwd`
export ESPIDF=`pwd`
source add_path.sh
cd -

cd nodemcu-prebuilt-toolchains/esp32/bin/
export PATH=$PATH:`pwd`
cd -
