#!/usr/bin/env bash

cd esp-idf
export IDF_PATH=$(pwd)
export ESPIDF=$(pwd)
source add_path.sh
cd -

if [ -f custom_env.sh ]; then
	. custom_env.sh
fi

# Anything added to the PATH in custom_env.sh will take precedence
# to the bundled toolchain:
export PATH=$PATH:$(pwd)/xtensa-esp32-elf/bin

# Some shells, like zsh, will cache path lookups, so after changing PATH we
# need to refresh the hash:
hash -r &>/dev/null || true
