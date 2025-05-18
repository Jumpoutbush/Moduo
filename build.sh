#!/bin/bash

set -e


# check build dir exists
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*    

cd `pwd`/build &&
    cmake .. && 
    make clean &&
    make -j

cd ..

# copy .h to /usr/include/Moduo, copy .so to /usr/lib
if [ ! -d /usr/include/Moduo ]; then
    mkdir /usr/include/Moduo
fi

for header in `pwd`/src/*.h
do
    cp $header /usr/include/Moduo/
done

cp `pwd`/lib/libModuo.so /usr/lib/

ldconfig