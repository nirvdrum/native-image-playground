#!/usr/bin/env bash

export CXX=clang++

if [[ ! -d benchmark ]]; then
  git clone https://github.com/google/benchmark.git

  pushd benchmark || { echo "'benchmark' library source location does not exist!"; exit 1; }
  git checkout 8d86026c67e41b1f74e67c1b20cc8f73871bc76e

  cmake -E make_directory "build"
  cmake -E chdir "build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../
  cmake --build "build" --config Release

  popd || { echo "Unable to return to parent directory!"; exit 1; }
fi
