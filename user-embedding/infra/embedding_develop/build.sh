embedding_develop_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd ) 
echo $embedding_develop_dir
infra_dir=$(dirname -- "$embedding_develop_dir") 
echo $infra_dir
user_embedding_dir=$(dirname -- "$infra_dir") 
echo $user_embedding_dir
root_dir=$(dirname -- "$user_embedding_dir") 
echo $root_dir



DOCKER_FILE_PATH=$embedding_develop_dir/Dockerfile
echo $DOCKER_FILE_PATH
PREFIX=beclab

docker  build    \
    -f ${DOCKER_FILE_PATH} \
    -t ${PREFIX}/userembedding_develop $root_dir