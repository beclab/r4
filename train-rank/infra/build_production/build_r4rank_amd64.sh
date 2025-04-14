build_production_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd ) 
echo $build_production_dir
infra_dir=$(dirname -- "$build_production_dir") 
echo $infra_dir
train_rank_dir=$(dirname -- "$infra_dir") 
echo $train_rank_dir
r4_dir=$(dirname -- "$train_rank_dir")
echo $r4_dir



DOCKER_FILE_PATH=$r4_dir/Dockerfile.r4rank.amd64
PREFIX=beclab

docker  build    \
    -f ${DOCKER_FILE_PATH} \
    -t ${PREFIX}/r4rank $r4_dir