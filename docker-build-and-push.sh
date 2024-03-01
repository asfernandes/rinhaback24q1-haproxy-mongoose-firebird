#!/bin/sh
set -e

./docker-build.sh
docker push asfernandes/rinhaback24q1:haproxy-mongoose-firebird-api
docker push asfernandes/rinhaback24q1:haproxy-mongoose-firebird-db
