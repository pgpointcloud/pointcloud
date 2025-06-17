FROM postgres:14
MAINTAINER Paul Blottiere <blottiere.paul@gmail.com>

ENV POSTGRES_VERSION 14
ENV POSTGIS_VERSION 3
ENV LAZPERF_VERSION 1.5.0

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
        cmake \
        zlib1g-dev \
        postgresql-server-dev-all \
        libxml2-dev \
    && rm -rf /var/lib/apt/lists/* \
    && git clone https://github.com/hobuinc/laz-perf.git \
    && cd laz-perf \
    && git checkout ${LAZPERF_VERSION} \
    && cmake -DWITH_TESTS=FALSE . \
    && make \
    && make install \
    && cd .. \
    && rm -r laz-perf \
    && git clone https://github.com/pgpointcloud/pointcloud \
    && cd pointcloud \
    && ./autogen.sh \
    && ./configure --with-lazperf=/usr/local --with-pgconfig=/usr/lib/postgresql/${POSTGRES_VERSION}/bin/pg_config CFLAGS="-Wall -Werror -O2 -g" \
    && make \
    && make install \
    && apt-get purge -y --auto-remove \
        git \
        ca-certificates \
        build-essential \
        autoconf \
        automake \
        cmake \
        zlib1g-dev \
        postgresql-server-dev-all \
        libxml2-dev

RUN mkdir -p /docker-entrypoint-initdb.d
COPY ./initdb-pgpointcloud.sh /docker-entrypoint-initdb.d/10_pgpointcloud.sh
