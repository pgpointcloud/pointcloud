-- See https://github.com/pgpointcloud/pointcloud/issues/71
select '#71', PC_SchemaIsValid('<xml/>'::xml::text);
