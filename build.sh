#!/bin/bash

# param1:  build_type

current_dir=$(dirname $0)

build_type=Debug
[[ $1 =~ [rR]elease ]] && build_type=Release

[[ $OSTYPE =~ msys ]] && platform=Windows
[[ $OSTYPE =~ [lL]inux ]] && platform=Linux

tmp_dir=$current_dir/tmp

if [[ $platform = Windows ]]; then
    generate_options="-A x64"
    build_options="-- -v:n"
else
    build_options="-- -j $(nproc)"
fi

echo "building..."
echo "platform: $platform"
echo "build_type: $build_type"

eval "cmake -S $current_dir -B $tmp_dir -DCMAKE_BUILD_TYPE=$build_type $generate_options"
if [[ $? != 0 ]]; then
    exit 1
fi

eval "cmake --build $tmp_dir --config $build_type $build_options"
if [[ $? != 0 ]]; then
    exit 1
fi

echo "build completion."
