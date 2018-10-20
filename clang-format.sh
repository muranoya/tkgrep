#!/bin/bash

find . -type f -iregex '.+\.\(cpp\|hpp\)$' | xargs clang-format -i
