
## set necessary directory
```

export TERMINUS_RECOMMEND_MONGODB_URI="mongodb://localhost:27017"

docker run  --name  extractor  -e TERMINUS_RECOMMEND_MONGODB_URI=$TERMINUS_RECOMMEND_MONGODB_URI --net=host -d  aboveos/rss-extractor

```