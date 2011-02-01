#!/bin/bash

set -xe

# Enviroment parameter
MAJORVERSION="mcbsystools-2011.01"
MINORVERSION="1"
TARBALL_NAME="$MAJORVERSION.$MINORVERSION.tar.gz"
RSYNC_DEPLOY="root@kplanas01:/home/ftp/mcb-2/mcbsystools/"

# Subversion parameter
PROJECT_URL="http://kplanas01/svn/mcbsystools/branches/prod-v2011.01.x"
SVN=/usr/bin/svn

# PATH parameter
TEMP_PATH="/tmp"
EXTRACT_PATH=$TEMP_PATH/$MAJORVERSION
SCRIPT_PATH=$PWD

# Delete old files
if [ -e $EXTRACT_PATH ]; then	
	rm -Rf $EXTRACT_PATH	
fi

# Export Projectsource
$SVN export $PROJECT_URL $EXTRACT_PATH

# Dump subversion infos
$SVN info $PROJECT_URL > $EXTRACT_PATH/svn.info

# Create tarball
cd $TEMP_PATH
tar -cvzf $SCRIPT_PATH/$TARBALL_NAME $MAJORVERSION/*

# rsync auf kplanas01
cd $SCRIPT_PATH
rsync -a --delete $SCRIPT_PATH/$TARBALL_NAME $RSYNC_DEPLOY

exit 0
