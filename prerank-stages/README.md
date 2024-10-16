# system workflow

Provider recall, prerank,extractor task.


## recall
### environment
```
export NFS_ROOT_DIRECTORY="/nfs"
export JUICEFS_ROOT_DIRECTORY="/juicefs"
export ALGORITHM_FILE_CONFIG_PATH="/usr/config/"
export TERMINUS_RECOMMEND_SOURCE_NAME="worldnews"
export KNOWLEDGE_BASE_API_URL="http://localhost:3010/knowledge/feed/algorithm/"
export SYNC_PROVIDER="bytetrade"
export SYNC_FEED_NAME="worldnews"
export SYNC_MODEL_NAME="bert_v2"
export SUPPORT_LANGUAGE="en"
export SUPPORT_TIMELINESS="0"
```

### local run
```
docker run  --name  recall  -v /tmp/data/nfs:/nfs -v /tmp/data/juicefs:/juicefs -e ALGORITHM_FILE_CONFIG_PATH=$ALGORITHM_FILE_CONFIG_PATH  -e TERMINUS_RECOMMEND_SOURCE_NAME=$TERMINUS_RECOMMEND_SOURCE_NAME -e NFS_ROOT_DIRECTORY=$NFS_ROOT_DIRECTORY -e JUICEFS_ROOT_DIRECTORY=$JUICEFS_ROOT_DIRECTORY -e SYNC_PROVIDER=$SYNC_PROVIDER  -e SYNC_FEED_NAME=$SYNC_FEED_NAME -e SYNC_MODEL_NAME=$SYNC_MODEL_NAME -e SUPPORT_LANGUAGE=$SUPPORT_LANGUAGE -e SUPPORT_TIMELINESS=$SUPPORT_TIMELINESS  -e KNOWLEDGE_BASE_API_URL=$KNOWLEDGE_BASE_API_URL  --net=host -d  beclab/r4recall
```
## prerank
### environment
```
export NFS_ROOT_DIRECTORY="/nfs"
export JUICEFS_ROOT_DIRECTORY="/juicefs"
export ALGORITHM_FILE_CONFIG_PATH="/usr/config/"
export TERMINUS_RECOMMEND_SOURCE_NAME="worldnews"
export KNOWLEDGE_BASE_API_URL="http://localhost:3010/knowledge/feed/algorithm/"
export SUPPORT_LANGUAGE="en"
export SUPPORT_TIMELINESS="0"
```

### local run
```
docker run  --name  prerank  -v /tmp/data/nfs:/nfs -v /tmp/data/juicefs:/juicefs -e ALGORITHM_FILE_CONFIG_PATH=$ALGORITHM_FILE_CONFIG_PATH  -e TERMINUS_RECOMMEND_SOURCE_NAME=$TERMINUS_RECOMMEND_SOURCE_NAME -e NFS_ROOT_DIRECTORY=$NFS_ROOT_DIRECTORY -e JUICEFS_ROOT_DIRECTORY=$JUICEFS_ROOT_DIRECTORY -e SUPPORT_LANGUAGE=$SUPPORT_LANGUAGE -e SUPPORT_TIMELINESS=$SUPPORT_TIMELINESS  -e KNOWLEDGE_BASE_API_URL=$KNOWLEDGE_BASE_API_URL --net=host -d  beclab/prerank
```
## extractor
### environment
```
export TERMINUS_RECOMMEND_SOURCE_NAME="worldnews"
export KNOWLEDGE_BASE_API_URL="http://localhost:3010/knowledge/feed/algorithm/"
```

### local run
```
docker run  --name  extractor   -e TERMINUS_RECOMMEND_SOURCE_NAME=$TERMINUS_RECOMMEND_SOURCE_NAME -e KNOWLEDGE_BASE_API_URL=$KNOWLEDGE_BASE_API_URL  --net=host -d  beclab/extractor
```
