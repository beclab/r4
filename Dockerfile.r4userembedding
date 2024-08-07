FROM ubuntu:jammy As builder

# RUN apk add openssl-dev musl-dev g++

RUN mkdir -p /userembedding && \
    apt update && \
    apt install curl -y && \
    curl https://sh.rustup.rs -sSf | bash -s -- -y && \
    # echo 'source $HOME/.cargo/env' >> $HOME/.bashrc && \
    apt install build-essential -y && \
    apt-get install libssl-dev -y && \
    apt-get install pkg-config -y 
    
ENV PATH="/root/.cargo/bin:${PATH}"
WORKDIR /userembedding
COPY user-embedding/src  /userembedding/src
COPY user-embedding/Cargo.toml /userembedding/Cargo.toml
RUN cargo build --release



FROM  ubuntu:jammy

# Import from builder.


WORKDIR /userembedding

# Copy our build
COPY --from=builder /userembedding/target/release/userembedding ./


CMD ["/userembedding/userembedding"]