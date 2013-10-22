
set ( PCPG_INSTALL_EXENSIONS
  pointcloud_postgis--1.0.sql
  pointcloud_postgis.control
  )
        
install (
  FILES ${PCPG_INSTALL_EXENSIONS} 
  DESTINATION "${PGSQL_SHAREDIR}/extension"
  )

