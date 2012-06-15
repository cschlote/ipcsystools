#!/bin/bash

set -e

DEPLOYDATE=`date +%Y.%m.%d`
echo ""
echo "CAUTION: Dangerous script"
echo "  You should have a clean working copy and branches pushed to"
echo "  master repository before you continue!"
echo ""
echo "Enter 'Yes' to continue"
read a
if ! test "Yes" = "$a" ; then
	echo "aborted."
	exit
fi

if ! which pristine-tar ; then
	echo "This script requires pristine-tar installed!"
	sudo apt-get install pristine-tar
fi
if ! which git-dch ; then
	echo "This script requires git-buildpackage installed!"
	sudo apt-get install git-buildpackage
fi

#-- Checkout master and update release tags
git checkout master
git clean -df
echo $DEPLOYDATE > ipcsystools-release
./autogen.sh
git commit -m "Deployed ipcsystools $DEPLOYDATE" ipcsystools-release configure

echo "Switching to upstream branch, merge master"
git checkout upstream
git merge master

echo "Tag upstream release"
git tag -f -a -m "Upstream release $DEPLOYDATE" upstream/$DEPLOYDATE

echo "Export orig tarball"
git archive --format tar upstream > ../ipcsystools_$DEPLOYDATE.orig.tar
gzip -9 ../ipcsystools_$DEPLOYDATE.orig.tar

echo "Memorize orig pristine tarball"
pristine-tar commit ../ipcsystools_$DEPLOYDATE.orig.tar.gz

echo "Switching to debian branch, merge upstream master"
git checkout debian
git merge master
git tag -f -a -m "Debian release $DEPLOYDATE" debian/$DEPLOYDATE-1lucid1
git-dch --git-author --verbose -N $DEPLOYDATE-1lucid1

joe debian/changelog

echo "Fixup change log for new base version, commit and built packages with"
echo "$ git-buildpackage --git-verbose --git-tag --git-retag -tc -sa "

echo "When package built properly, upload package to debian package repository"
echo " on kplanas using aptadmin@kplanas01:>dput local <changes-file>"

exit 0
