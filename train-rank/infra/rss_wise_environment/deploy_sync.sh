export TERMINUS_RECOMMEND_REDIS_ADDR="127.0.0.1:6381"
export TERMINUS_RECOMMEND_REDIS_PASSOWRD="terminusrecommendredis123"
export NFS_ROOT_DIRECTORY="/home/ubuntu/terminus_data/nfs"
export JUICEFS_ROOT_DIRECTORY="/home/ubuntu/terminus_data/juicefs"

export FEED_MONGO_API_URL="http://localhost:3010/knowledge/feed/algorithm/"
docker run  --name  wise-sync  -v $NFS_ROOT_DIRECTORY:/nfs -v $JUICEFS_ROOT_DIRECTORY:/juicefs -e TERMINUS_RECOMMEND_REDIS_ADDR=$TERMINUS_RECOMMEND_REDIS_ADDR  -e TERMINUS_RECOMMEND_REDIS_PASSOWRD=$TERMINUS_RECOMMEND_REDIS_PASSOWRD -e NFS_ROOT_DIRECTORY="/nfs" -e JUICEFS_ROOT_DIRECTORY="/juicefs" -e FEED_MONGO_API_URL=$FEED_MONGO_API_URL -e ALGORITHM_FILE_CONFIG_PATH=/usr/config/ --net=host -d  aboveos/rss-sync
