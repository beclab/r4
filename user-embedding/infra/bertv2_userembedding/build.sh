bertv2_userembedding_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd ) 
echo $bertv2_userembedding_dir
infra_dir=$(dirname -- "$bertv2_userembedding_dir") 
echo $infra_dir
user_embedding_dir=$(dirname -- "$infra_dir") 
echo $user_embedding_dir
root_dir=$(dirname -- "$user_embedding_dir") 
echo $root_dir



DOCKER_FILE_PATH=$bertv2_userembedding_dir/Dockerfile
PREFIX=beclab

docker  build    \
    -f ${DOCKER_FILE_PATH} \
    -t ${PREFIX}/bertv2userembedding $root_dir