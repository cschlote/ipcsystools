#! /bin/bash

aclocal $ACLOCAL_FLAGS

libtoolize --force --copy

autoconf \
	--force \
	--install \
	--warnings=cross \
	--warnings=syntax \
	--warnings=obsolete \
	--warnings=unsupported
						
