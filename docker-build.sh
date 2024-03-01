#!/bin/sh
set -e

docker buildx build --progress plain -f src/api/Dockerfile -t asfernandes/rinhaback24q1:haproxy-mongoose-firebird-api .
docker buildx build --progress plain -f src/db/Dockerfile -t asfernandes/rinhaback24q1:haproxy-mongoose-firebird-db .
