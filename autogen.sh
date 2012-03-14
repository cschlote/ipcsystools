#!/bin/bash

aclocal $ACLOCAL_FLAGS

autoreconf \
	--force \
	--install \
	--warnings=syntax \
	--warnings=obsolete \
	--warnings=unsupported \
	--verbose
