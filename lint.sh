#!/usr/bin/env bash

find src -regex '.*\.\(cxx\|c\|h\)$' -print0 | xargs -0 -n1 clang-format --dry-run -W error --style=google --sort-includes
