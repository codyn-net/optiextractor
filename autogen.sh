#!/bin/sh
# Run this to generate all the initial makefiles, etc.

ccbuild-autotools --version 0.1 generate
autoreconf --install
