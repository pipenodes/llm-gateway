FROM debian:12-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    libjsoncpp-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libyaml-cpp-dev \
    wget \
    make \
    && rm -rf /var/lib/apt/lists/*

COPY . /build
WORKDIR /build

# Mesmas fontes e flags que `make all` (plugins em src/plugins/core e src/plugins/enterprise).
# make clean: evita reutilizar .o do contexto se algum escapar ao .dockerignore (LTO host ≠ container).
RUN make clean 2>/dev/null || true; make -j"$(nproc)" all

FROM debian:12-slim

RUN apt-get update && apt-get install -y \
    libjsoncpp25 \
    libssl3 \
    libcurl4 \
    libyaml-cpp0.7 \
    curl \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/hermes /app/hermes
COPY config.json /app/data/config.json

RUN mkdir -p /app/data

WORKDIR /app/data
EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=5s --retries=3 \
    CMD curl -sf http://localhost:8080/health || exit 1

VOLUME /app/data
CMD ["/app/hermes"]
