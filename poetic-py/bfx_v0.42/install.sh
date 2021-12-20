#!/bin/bash

# Make BFA
make

# Compile BF2C
echo "gcc -o bf2c bf2c.c"
gcc -o bf2c bf2c.c