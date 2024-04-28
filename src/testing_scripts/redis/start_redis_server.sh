#!/bin/bash

source global_dirs.sh
redis_dir=third_party/tmp/redis-6.2.13

rm *.rdb

${redis_dir}/src/redis-server src/testing_scripts/redis/redis.conf
