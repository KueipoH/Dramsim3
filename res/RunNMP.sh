#!/bin/bash

cd ..

cd build

./dramsim3main ../configs/DDR4_8Gb_x8_3200.ini -s nmp -c 10 -o ../res
