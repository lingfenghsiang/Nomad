#!/bin/bash

bash src/testing_scripts/redis/start_redis_server.sh &
sleep 15
bash src/testing_scripts/redis/run_redis_client.large.sh
sleep 60

bash src/testing_scripts/redis/start_redis_server.sh &
sleep 15
bash src/testing_scripts/redis/run_redis_client.small.sh
sleep 60

bash src/testing_scripts/redis/start_redis_server.sh &
sleep 15
bash src/testing_scripts/redis/run_redis_client.noevict.sh
sleep 60
