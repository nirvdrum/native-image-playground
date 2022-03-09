#!/usr/bin/env bash

export CXX=clang++

if [[ ! -d benchmark ]]; then
  git clone --depth 1 https://github.com/google/benchmark.git

  pushd benchmark

  cmake -E make_directory "build"
  cmake -E chdir "build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../
  cmake --build "build" --config Release

  popd
fi
