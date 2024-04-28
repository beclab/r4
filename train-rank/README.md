
# Local Build Image

## Build beclab/rank_develop Image
```bash
cd train-rank/infra/ubuntu_develop
bash build.sh
```

## Build beclab/bertv2train
```bash
cd train-rank/infra/bertv2train_deploy
bash build.sh
```

## Build beclab/bertv2rank
```bash
cd train-rank/infra/bertv2rank_deploy
bash build.sh
```

# Start Develop Environment
C++ development environment is complex, so we develop in a container.
```bash
host_code_directory="/home/ubuntu/recommend-bytetrade-algorithm/train-rank"
docker run --name temp_rank_develop -v $host_code_directory:/opt/rss-termius-v2-rank --net=host -d beclab/rank_develop
```

# Deploy Local

## Prepare Other Dependent Container
```bash
cd train-rank/infra/rss_wise_environment
bash deploy.sh
```

## Local Run
```bash
export TERMINUS_RECOMMEND_MONGODB_URI="mongodb://root:example@localhost:27017/?authSource=admin&readPreference=primary&ssl=false&directConnection=true"
export TERMINUS_RECOMMEND_MONGODB_NAME="document"
export HOST_MODEL_DIRECTORY="/home/ubuntu/rank_model"
export TERMINUS_RECOMMEND_SOURCE_NAME="bert_v2"
export KNOWLEDGE_BASE_API_URL="127.0.0.1:3010"

docker run --name temp_bertv2train -v $HOST_MODEL_DIRECTORY:/opt/rank_model --net=host -e KNOWLEDGE_BASE_API_URL=$KNOWLEDGE_BASE_API_URL -e TERMINUS_RECOMMEND_SOURCE_NAME=$TERMINUS_RECOMMEND_SOURCE_NAME -d beclab/bertv2train

docker run --name temp_bertv2rank -v $HOST_MODEL_DIRECTORY:/opt/rank_model --net=host -e KNOWLEDGE_BASE_API_URL=$KNOWLEDGE_BASE_API_URL -e TERMINUS_RECOMMEND_SOURCE_NAME=$TERMINUS_RECOMMEND_SOURCE_NAME -d beclab/bertv2rank
```
