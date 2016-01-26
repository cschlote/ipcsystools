#!/bin/bash
# Script should be maintained on master branch and merged to debian or upstream as needed!

set -e

#-- prerequistes -------------------------------------------------------

if ! which pristine-tar ; then
	echo "This script requires pristine-tar installed!"
	sudo apt-get install pristine-tar
fi
if ! which gbp ; then
	echo "This script requires git-buildpackage installed!"
	sudo apt-get install git-buildpackage
fi

#--  Last warning ------------------------------------------------------

DEPLOYDATE=`date +%Y.%m.%d`
echo ""
echo "CAUTION: Dangerous script"
echo "  You should have a clean working copy and branches pushed to"
echo "  master repository before you continue!"
echo ""
echo "DEPLOYDATE will be set to $DEPLOYDATE"
echo ""
echo "Enter 'Yes' to continue"
read a
if ! test "Yes" = "$a" ; then
	echo "aborted."
	exit
fi

#-- Checkout master and update release tags ----------------------------

echo "Checkout master and update release tag"
git checkout master
git clean -df
./autogen.sh
git citool || true
echo $DEPLOYDATE > ipcsystools-release
git commit -m "Deployed ipcsystools $DEPLOYDATE" ipcsystools-release

echo "Switching to upstream branch, merge master"
git checkout upstream
git merge master

echo "Tag new upstream release"
git tag -f -a -m "Upstream release $DEPLOYDATE" upstream/$DEPLOYDATE

echo "Export orig tarball"
git archive --format tar upstream > ../ipcsystools_$DEPLOYDATE.orig.tar
gzip -9 ../ipcsystools_$DEPLOYDATE.orig.tar

echo "Memorize orig pristine tarball"
pristine-tar commit ../ipcsystools_$DEPLOYDATE.orig.tar.gz

echo "Switching back to debian branch, merge upstream master"
git checkout debian
git merge master

export DEPLOYDATE=`cat ipcsystools-release`;
git tag -f -a -m "Debian release $DEPLOYDATE" debian/$DEPLOYDATE-1ubuntu1
gbp dch --git-author --verbose -N $DEPLOYDATE-1ubuntu1

joe debian/changelog

echo "Change and commit changelog for new base version, then built packages with"
echo "$ git-buildpackage --git-verbose --git-tag --git-retag -tc -sa "

echo "When package built properly, upload package to debian package repository"
echo " on kplanas using aptadmin@kplanas01:>dput local <changes-file>"

exit 0
