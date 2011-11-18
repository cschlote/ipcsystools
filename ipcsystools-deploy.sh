#!/bin/bash

set -e

DEPLOYDATE=`date +%Y.%m.%d` 
echo ""
echo "CAUTION: Dangerous script"
echo "  You should have a clean working copy and brnaches pushed to"
echo "  master repository before you continue!"
echo ""
echo "Enter 'Yes' to continue"
read a
if ! test "Yes" = "$a" ; then
	echo "aborted."
	exit
fi

echo "Switching to upstream branch, merge master"
git checkout upstream
git merge master

echo "Tag upstream release"
git tag -a -m "Upstream release $DEPLOYDATE" upstream/$DEPLOYDATE

echo "Export orig tarball"
git archive --format tar.gz -9 upstream > ../ipcsystools_$DEPLOYDATE.orig.tar.gz

echo "Memorize orig pristine tarball"
pristine-tar commit ../ipcsystools_$DEPLOYDATE.orig.tar.gz

echo "Switching to debian branch, merge upstreammaster"
git checkout debian
git merge master
git-dch --git-author --verbose -N $DEPLOYDATE-1lucid1

echo "Fixup change log for new base version, commit and built packages with"
echo "$ git-buildpackage --git-verbose --git-tag --git-retag -tc -sa "

exit 0
