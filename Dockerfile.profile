FROM debian:12-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    libjsoncpp-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

COPY . /build
WORKDIR /build

RUN wget -q https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.34.0/httplib.h -O src/httplib.h

RUN g++ -o hermes src/main.cpp src/gateway.cpp src/ollama_client.cpp src/api_keys.cpp \
    -std=c++23 -g -O1 \
    -I/usr/include/jsoncpp -Isrc \
    -ljsoncpp -lpthread \
    -fsanitize=address,leak \
    -fno-omit-frame-pointer

FROM debian:12-slim

RUN apt-get update && apt-get install -y \
    libjsoncpp25 \
    libasan8 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/hermes /app/hermes
COPY config.json /app/config.json

WORKDIR /app
EXPOSE 8080

ENV ASAN_OPTIONS=detect_leaks=1:log_path=/app/asan.log
CMD ["/app/hermes"]
