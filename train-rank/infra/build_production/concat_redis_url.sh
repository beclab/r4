#!/bin/bash

# Check if variables exist
if [[ -z "$REDIS_ADDR" || -z "$REDIS_PASSWORD" ]]; then
    echo "REDIS_ADDR or REDIS_PASSWORD is not set"
    exit 1
fi

# Concatenate Redis URL
REDIS_URL="redis://:$REDIS_PASSWORD@$REDIS_ADDR"

# Output the result
echo "$REDIS_URL"
# redis-cli -u redis://:terminusrecommendredis123@127.0.0.1:6381
