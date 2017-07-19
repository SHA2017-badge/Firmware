#!/bin/bash

set -x -e
ITERS=250

MCUFONTPATH=../../ugfx/tools/mcufontencoder/src/
export PATH=${MCUFONTPATH}:$PATH

for file in *.dat; do
	mcufont rlefont_optimize $file $ITERS
done
