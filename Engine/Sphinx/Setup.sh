#!/bin/bash

# Separate source and build
sphinx-quickstart --sep --makefile ./Build \
    --project "Engine" \
    --author "yhyu13" \
    --release "0.2.0" \
    --language "zh"
