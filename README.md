# Pointcloud

[![Release][release-image]][releases] [![Dockerhub][dockerhub-image]][dockerhub]

[release-image]: https://img.shields.io/badge/release-1.2.5-green.svg?style=plastic
[releases]: https://github.com/pgpointcloud/pointcloud/releases

[dockerhub-image]: https://img.shields.io/docker/pulls/pgpointcloud/pointcloud?logo=docker&label=pulls
[dockerhub]: https://hub.docker.com/r/pgpointcloud/pointcloud

A PostgreSQL extension for storing point cloud (LIDAR) data. See
https://pgpointcloud.github.io/pointcloud/ for more information.

## Continuous integration

|                    | w/o PostGIS | PostGIS 3.3 |
| ------------------ |:-----------:|:-----------:|
| PostgreSQL 13      | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres13_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres13_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres13_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres13_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) |
| PostgreSQL 14      | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres14_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres14_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres14_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres14_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) |
| PostgreSQL 15      | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres15_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres15_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres15_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres15_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) |
| PostgreSQL 16      | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres16_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres16_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres16_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres16_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) |
| PostgreSQL 17      | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres17_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres17_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) | ![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/jammy_postgres17_postgis33.yml?branch=master&label=22.04&logo=ubuntu&style=plastic)<br />![](https://img.shields.io/github/actions/workflow/status/pgpointcloud/pointcloud/noble_postgres17_postgis33.yml?branch=master&label=24.04&logo=ubuntu&style=plastic) |
