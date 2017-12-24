#!/bin/bash
for files in $(grep -l "^& " txt/*.txt)
do echo ./mkindx ${files} $(echo ${files}|cut -f1 -d".").indx
done
