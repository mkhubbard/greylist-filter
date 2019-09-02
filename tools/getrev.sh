#!/bin/sh
#
# Utility to return the current revision 
#
# this snippet was originally take from:
#     http://wiki.gnuarch.org/moin.cgi/ArchUtilityCode
#
# It has been modified to meet my own needs - MKH
#
 
PATH=${PATH}:/c/program\ files/tla
TREE_VERSION_STRING=`tla tree-version`
TREE_BRANCH=`tla parse-package-name -b ${TREE_VERSION_STRING}`
TREE_VERSION=`tla parse-package-name -v ${TREE_VERSION_STRING}`
TREE_REVISION=`tla logs -r | head -n 1`
VERSION_STRING="${TREE_VERSION}"
REVISION_TAG=`echo ${TREE_REVISION} | gawk -F\- '{print $1}'`
REVISION_NUM=`echo ${TREE_REVISION} | gawk -F\- '{print $2}'`

if [ `echo ${TREE_VERSION_STRING} | grep -c "devel"` != "0" ]; then
	SUFFIX="-devel"
else
	SUFFIX=""
fi

case "${REVISION_TAG}" in
	patch) VERSION_STRING="${VERSION_STRING}.${REVISION_NUM}" ;;
 	version) ;;
  	versionfix) VERSION_STRING="${VERSION_STRING}.${REVISION_NUM}" ;;
esac
echo ${VERSION_STRING}${SUFFIX}

