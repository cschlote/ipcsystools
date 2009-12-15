#! /bin/bash

aclocal $ACLOCAL_FLAGS

libtoolize --force --copy
autoheader
automake
autoreconf \
	--force \
	--install \
	--warnings=cross \
	--warnings=syntax \
	--warnings=obsolete \
	--warnings=unsupported
						
