FROM ubuntu:jammy As builder

# RUN apk add openssl-dev musl-dev g++

RUN mkdir -p /bertv2userembedding && \
    apt update && \
    apt install curl -y && \
    curl https://sh.rustup.rs -sSf | bash -s -- -y && \
    # echo 'source $HOME/.cargo/env' >> $HOME/.bashrc && \
    apt install build-essential -y && \
    apt-get install libssl-dev -y && \
    apt-get install pkg-config -y 
    
ENV PATH="/root/.cargo/bin:${PATH}"
WORKDIR /bertv2userembedding
COPY user-embedding/src  /bertv2userembedding/src
COPY user-embedding/Cargo.toml /bertv2userembedding/Cargo.toml
RUN cargo build --release



FROM  ubuntu:jammy

# Import from builder.


WORKDIR /bertv2userembedding

# Copy our build
COPY --from=builder /bertv2userembedding/target/release/bertv2userembedding ./


CMD ["/bertv2userembedding/bertv2userembedding"]