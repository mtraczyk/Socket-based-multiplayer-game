#!/bin/bash

rm -rf ./screen*
cd ../
make clean
make
mv ./screen* ./tests
cd tests