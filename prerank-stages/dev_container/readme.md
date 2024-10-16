export host_code_directory="/home/ubuntu/r4/prerank-stages"
docker run --name prerank_stage_develop  -v $host_code_directory:/opt/prerank-stages --net=host -d beclab/prerank_stages_develop