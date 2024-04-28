FROM ubuntu:jammy

# RUN apk add openssl-dev musl-dev g++

RUN mkdir -p /opt/rss-termius-v2-userembedding && \
    apt update && \
    apt install curl -y && \
    curl https://sh.rustup.rs -sSf | bash -s -- -y && \
    # echo 'source $HOME/.cargo/env' >> $HOME/.bashrc && \
    apt install build-essential -y && \
    apt-get install libssl-dev -y && \
    apt-get install pkg-config -y 

ENV PATH="/root/.cargo/bin:${PATH}"
# RUN rm /bin/sh && ln -s /bin/bash /bin/sh
WORKDIR  /opt/rss-termius-v2-userembedding
ENTRYPOINT ["tail", "-f", "/dev/null"]