#!/bin/bash

OPTS="--delete --compact --memory --verify"
echo "Benching with options: ${OPTS}..."
for level in {1..9}; do
echo "=====> Benching level-$level ...."
for app in test-PR-unix test-C-unix test-Orig-unix; do
./obj/bin/${app} ${OPTS} --level=${level} $1
done
done
