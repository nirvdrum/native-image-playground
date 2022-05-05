#!/bin/bash

find src -type f -regex '.*\.\(cxx\|c\|h\)$' -print0 | xargs -0 -n1 clang-format -i --style=file --sort-includes
