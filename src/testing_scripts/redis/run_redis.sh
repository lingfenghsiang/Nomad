#!/bin/bash

# Define usage function
usage() {
    echo "Usage: $0 [-r ARG] [-b] [-c]"
    echo "Options:"
    echo "  -r ARG   Specify which case to run, if not specified, run all. 1 for case 1, 2 for case 2, 3 for case 3"
    exit 1
}

# Set default values
arg_a=0

# Parse arguments
while getopts ":r:" opt; do
    case $opt in
        r)
            arg_a="$OPTARG"
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            usage
            ;;
        :)
            echo "Option -$OPTARG requires an argument" >&2
            usage
            ;;
    esac
done

# Shift the parsed options, so $@ now contains the positional arguments
shift $((OPTIND - 1))

# Print parsed values
echo "Run test: $arg_a"


case1(){
    bash src/testing_scripts/redis/start_redis_server.sh &
    sleep 5
    bash src/testing_scripts/redis/run_redis_client.small.sh
    sleep 60

    wait
}

case2(){
    bash src/testing_scripts/redis/start_redis_server.sh &
    sleep 5
    bash src/testing_scripts/redis/run_redis_client.large.sh
    sleep 60

    wait
}

case3(){
    bash src/testing_scripts/redis/start_redis_server.sh &
    sleep 5
    bash src/testing_scripts/redis/run_redis_client.noevict.sh
    sleep 60

    wait
}



if [ "$arg_a" -eq 0 ]; then
    case1
    case2
    case3
elif [ "$arg_a" -eq 1 ]; then
    case1
elif [ "$arg_a" -eq 2 ]; then
    case2
elif [ "$arg_a" -eq 3 ]; then
    case3
else
echo unavailable option
fi
