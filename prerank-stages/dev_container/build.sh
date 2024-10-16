dev_container_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd ) 
echo $dev_container_dir
root_dir=$(dirname -- "$dev_container_dir") 
echo $root_dir
DOCKER_FILE_PATH=$dev_container_dir/Dockerfile
PREFIX=beclab

docker  build    \
    -f ${DOCKER_FILE_PATH} \
    -t ${PREFIX}/prerank_stages_develop $root_dir