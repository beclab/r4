FROM ubuntu:jammy

RUN apt-get update && \
    apt-get install build-essential -y && \
    apt-get install cmake -y && \
    apt install git -y && \
    apt install curl -y && \
    apt install pkg-config -y && \
    apt-get install libssl-dev -y && \
    apt-get install unzip -y && \
    apt-get install protobuf-compiler -y

RUN  cd /opt && \
     curl -LO https://go.dev/dl/go1.20.11.linux-amd64.tar.gz && \
     tar -xvzf go1.20.11.linux-amd64.tar.gz && \
     mv go /usr/local 
ENV PATH $PATH:/usr/local/go/bin
RUN go env -w GO111MODULE=on

RUN mkdir -p /opt/prerank-stages && \
    go install google.golang.org/protobuf/cmd/protoc-gen-go@latest  




ENTRYPOINT ["tail", "-f", "/dev/null"]