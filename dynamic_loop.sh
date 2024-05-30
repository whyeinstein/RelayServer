#!/bin/bash

while true; do
  # 生成一个在1000内的偶数
  param1=$(( 600 + 2 * ($RANDOM % 201) ))

  # 生成一个在50-100范围内的数
  param3=$(( 50 + ($RANDOM % 11) ))

  ./build/LoadGenerator/src/dynamic_loadgenerator $param1 3 $param3 127.0.0.1 8888

  sleep 10  
done