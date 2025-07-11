FROM mcr.microsoft.com/devcontainers/base:ubuntu

RUN export DEBIAN_FRONTEND=noninteractive

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    UV_CACHE_DIR=/tmp/uv_cache

RUN apt-get update \
 && apt-get -y install --no-install-recommends build-essential gdb python3 python3-pip python3-dev \
 && apt-get -y autoremove \
 && apt-get -y clean \
 && rm -rf /var/lib/apt/lists/*

RUN pip3 install uv
