
## set necessary directory
```
export TERMINUS_RECOMMEND_REDIS_ADDR="127.0.0.1:6379"
export TERMINUS_RECOMMEND_REDIS_PASSOWRD="123456"
export NFS_ROOT_DIRECTORY="/nfs"
export JUICEFS_ROOT_DIRECTORY="/juicefs"

docker run  --name  recall  -v /tmp/data/nfs:/nfs -v /tmp/data/juicefs:/juicefs -e TERMINUS_RECOMMEND_REDIS_ADDR=$TERMINUS_RECOMMEND_REDIS_ADDR  -e TERMINUS_RECOMMEND_REDIS_PASSOWRD=$TERMINUS_RECOMMEND_REDIS_PASSOWRD -e NFS_ROOT_DIRECTORY=$NFS_ROOT_DIRECTORY -e JUICEFS_ROOT_DIRECTORY=$JUICEFS_ROOT_DIRECTORY  --net=host -d  aboveos/rss-recall

```