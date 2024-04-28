export GOPATH=$HOME/go
export PATH=$PATH:$GOPATH/bin
PROTOBUF_ENTITY_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd ) 
DUMP_PACKAGE_DIR=$(dirname -- "$PROTOBUF_ENTITY_DIR") 
echo $PROTOBUF_ENTITY_DIR
echo $DUMP_PACKAGE_DIR
protoc --go_out=$DUMP_PACKAGE_DIR  --proto_path=$DUMP_PACKAGE_DIR   $PROTOBUF_ENTITY_DIR/feed.proto
protoc --go_out=$DUMP_PACKAGE_DIR  --proto_path=$DUMP_PACKAGE_DIR   $PROTOBUF_ENTITY_DIR/entry.proto
