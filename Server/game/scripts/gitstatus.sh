#!/usr/bin/env bash
# Checks the status of your local repo versus the upstream repo.
# 0: diverged
# 1: up-to-date
# 2: behind
# 3: ahead

a="master" b="origin/master"
base=$( git merge-base $a $b )
aref=$( git rev-parse  $a )
bref=$( git rev-parse  $b )

if [[ $aref == "$bref" ]]; then
  echo 1
elif [[ $aref == "$base" ]]; then
  echo 2
elif [[ $bref == "$base" ]]; then
  echo 3
else
  echo 0
fi
