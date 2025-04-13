build_production_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd ) 
echo $build_production_dir
infra_dir=$(dirname -- "$build_production_dir") 
echo $infra_dir
user_embedding_dir=$(dirname -- "$infra_dir") 
echo $user_embedding_dir
r4_dir=$(dirname -- "$user_embedding_dir")
echo $r4_dir



DOCKER_FILE_PATH=$r4_dir/Dockerfile.r4userembedding
PREFIX=beclab

docker  build    \
    -f ${DOCKER_FILE_PATH} \
    -t ${PREFIX}/r4userembedding $r4_dir