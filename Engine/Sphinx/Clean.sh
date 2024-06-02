#!/bin/bash

ROOT=$(pwd)
cd ./Build || exit 1
make clean
cd $ROOT || exit 1