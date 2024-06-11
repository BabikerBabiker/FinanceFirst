#!/bin/bash

cd ~/Documents/GitHub/FinanceFirst/build || { echo "Failed to navigate to build directory"; exit 1; }

cmake .. || { echo "CMake configuration failed"; exit 1; }

make || { echo "Build failed"; exit 1; }

./FinanceFirst || { echo "Failed to run FinanceFirst"; exit 1; }
