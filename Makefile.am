SUBDIRS = libfileaccess libapoc apocd src 

apocddir=$(APOCDIR)
apocd_DATA=build/class/apocd.jar

configdir=$(sysconfdir)
config_DATA= \
	src/com/sun/apoc/daemon/properties/apocd.properties \
	src/com/sun/apoc/daemon/properties/os.properties \
	src/com/sun/apoc/daemon/properties/policymgr.properties


if SOLARIS
smfdir=$(localstatedir)/svc/manifest/network
smf_DATA=smf/apocd.xml
endif
