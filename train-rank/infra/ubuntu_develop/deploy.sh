export host_code_directory="/home/ubuntu/r4/train-rank"
export host_data_directory="/home/ubuntu/data"
docker run --name temp_rank_develop -v $host_code_directory:/opt/rss-termius-v2-rank -v $host_data_directory:/opt/data   --net=host -d beclab/rank_develop