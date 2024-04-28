bertv2rank_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd ) 
echo $bertv2rank_dir
infra_dir=$(dirname -- "$bertv2rank_dir") 
echo $infra_dir
train_rank_dir=$(dirname -- "$infra_dir") 
echo $train_rank_dir
root_dir=$(dirname -- "$train_rank_dir")
echo $root_dir
DOCKER_FILE_PATH=$bertv2rank_dir/Dockerfile
PREFIX=beclab


docker  build --progress auto   \
    -f ${DOCKER_FILE_PATH} \
    -t ${PREFIX}/rank_develop $root_dir