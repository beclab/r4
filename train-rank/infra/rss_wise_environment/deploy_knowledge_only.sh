SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
export CREATE_USER_JS=$SCRIPT_DIR/001_users.js
export MONGO_URL=mongodb://root:example@127.0.0.1:27017/knowledge?authSource=admin
                 # mongodb://root:example@localhost:27017/?authSource=admin
export BACKEND_URL=http://127.0.0.1:8080 
export MONGODB_NAME="knowledge" 
export MONGODB_FEED_COLL="feeds" 
export MONGODB_ENTRY_COLL="entries" 
export LISTEN_ADDR="127.0.0.1:8080"
export REDIS_HOST_DATA_DIRECTORY="/home/ubuntu/terminus_redis"
export REDIS_ADDR="127.0.0.1:6381"
export REDIS_PASSWORD="terminusrecommendredis123"
docker-compose -f docker-compose-only-knowledge-base-api.yaml up -d