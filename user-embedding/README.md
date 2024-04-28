
# Build Local Image

## Build beclab/userembedding_develop
```bash
cd user-embedding/infra/embedding_develop
bash build.sh
```

## Build beclab/bertv2userembedding
```bash
cd user-embedding/infra/bertv2_userembedding
bash build.sh
```

# Develop Environment
```bash
host_code_directory="/home/ubuntu/recommend-bytetrade-algorithm/user-embedding"
host_hugging_face_directory="/home/ubuntu/huggingface"
docker run --name temp_userembedding_develop -v $host_code_directory:/opt/rss-termius-v2-userembedding -v $host_hugging_face_directory:/root/.cache/huggingface --net=host -d beclab/userembedding_develop
```
Use VSCode to open the user-embedding directory, then attach to the `temp_userembedding_develop` container.

# Deploy Local

## Prepare Other Dependent Container
```bash
cd train-rank/infra/rss_wise_environment
bash deploy.sh
```

## Local Run
```bash
export host_hugging_face_directory="/home/ubuntu/huggingface"
export TERMINUS_RECOMMEND_MONGODB_URI="mongodb://root:example@localhost:27017/?authSource=admin&readPreference=primary&ssl=false&directConnection=true"
export TERMINUS_RECOMMEND_SOURCE_NAME="worlnews"
export TERMINUS_RECOMMEND_MONGODB_NAME="document"
export TERMINUS_RECOMMEND_REDIS_ADDR="127.0.0.1:6381"
export TERMINUS_RECOMMEND_REDIS_PASSOWRD="terminusrecommendredis123"
export KNOWLEDGE_BASE_API_URL="http://127.0.0.1:3010"

docker run --name temp_userembedding -v $host_hugging_face_directory:/root/.cache/huggingface --net=host -e TERMINUS_RECOMMEND_REDIS_ADDR=$TERMINUS_RECOMMEND_REDIS_ADDR -e TERMINUS_RECOMMEND_REDIS_PASSOWRD=$TERMINUS_RECOMMEND_REDIS_PASSOWRD -e TERMINUS_RECOMMEND_SOURCE_NAME=$TERMINUS_RECOMMEND_SOURCE_NAME -e TERMINUS_RECOMMEND_MONGODB_NAME=$TERMINUS_RECOMMEND_MONGODB_NAME -e TERMINUS_RECOMMEND_REDIS_URI=$TERMINUS_RECOMMEND_REDIS_URI -d bytetrade/bertv2userembedding
```
