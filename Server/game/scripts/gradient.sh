#! /bin/sh
# See gradient.js for details
cd scripts
deno run --allow-net --allow-env gradient.js "$*"
