#!/bin/bash

cd ..

cd build

./dramsim3main ../configs/DDR4_8Gb_x8_3200.ini --stream stream -c 100000 -o ../res

