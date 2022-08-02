#!/usr/bin/env bash

os=$(uname)
find_pattern='.*\.(cxx|c|h)$'

if [ "$os" == 'Darwin' ]; then
  find_command="find -E src -type f -regex '$find_pattern' -print0"
else
  find_command="find src -type f -regextype egrep -regex '$find_pattern' -print0"
fi

eval "$find_command" | xargs -0 -n1 clang-format -i --style=file --sort-includes

