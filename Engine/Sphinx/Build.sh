#!/bin/bash

ROOT=$(pwd)
cd ./Build || exit 1
make html
cd $ROOT || exit 1