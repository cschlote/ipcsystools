#!/bin/bash
# Get and output as ASCII the field strengh of GSM/UMTS connection
umtscardtool -f
echo $?
exit 0
