#! /bin/sh
cd scripts
deno run --allow-net --allow-env gradient.js "$*"
