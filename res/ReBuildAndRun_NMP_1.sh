#!/bin/bash
cd ..
# Remove the build directory if it exists
rm -rf build

# Create a new build directory
mkdir build
cd build

# Run cmake to set up the build system
cmake ..

# Build the dramsim3 library and executables using 4 parallel jobs
make -j4

# Run the dramsim3main executable with the specified configuration and options
./dramsim3main ../configs/NMP_DDR4_8Gb_x16_3200.ini -s nmp -c 1000 -o ../res

# Print completion message
echo "***Rebuild and Run Completed!***"
