#!/bin/bash

if [[ "$OSTYPE" == "msys"* ]]; then
    ./build/Debug/inOneWeekend.exe
elif [[ "$OSTYPE" == "darwin"* ]]; then
    ./build/inOneWeekend
fi