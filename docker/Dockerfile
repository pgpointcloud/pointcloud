FROM postgres:12
MAINTAINER Paul Blottiere <blottiere.paul@gmail.com>

ENV POSTGRES_VERSION 12
ENV POSTGIS_VERSION 3

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        postgis \
        postgresql-${POSTGRES_VERSION}-postgis-${POSTGIS_VERSION} \
        postgresql-${POSTGRES_VERSION}-postgis-${POSTGIS_VERSION}-scripts

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        git \
        ca-certificates \
        build-essential \
        autoconf \
        automake \
        zlib1g-dev \
        postgresql-server-dev-all \
        libxml2-dev \
    && rm -rf /var/lib/apt/lists/* \
    && git clone https://github.com/pgpointcloud/pointcloud \
    && cd pointcloud \
    && ./autogen.sh \
    && ./configure --with-pgconfig=/usr/lib/postgresql/${POSTGRES_VERSION}/bin/pg_config CFLAGS="-Wall -Werror -O2 -g" \
    && make \
    && make install \
    && apt-get purge -y --auto-remove \
        git \
        ca-certificates \
        build-essential \
        autoconf \
        automake \
        zlib1g-dev \
        postgresql-server-dev-all \
        libxml2-dev
