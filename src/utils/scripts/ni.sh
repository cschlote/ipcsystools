#!/bin/bash

while :; do
  umtscardtool -i
  echo $?
  if [ $? = 0 ]; then
  	break
  fi
  sleep 1
done

exit $?
