#!/usr/bin/bash

cd ~/c/toby/cproj && cmake -DCMAKE_BUILD_TYPE=Release -B build -S . && cmake --build build -- -j8 && ./build/tests/test_hashtable && ./build/bin/topwords ./data/shakespeare.txt 10