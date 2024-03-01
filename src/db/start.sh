#!/bin/sh

DATABASE=/data/firebird/database.fdb

rm -f /data/firebird/* /data/firebird-tmp/*
echo "create database '$DATABASE' page_size 8192; input '/sql/database.sql';" | $FIREBIRD/bin/isql -q
$FIREBIRD/bin/gfix -write async $DATABASE
$FIREBIRD/bin/gfix -housekeeping 0 $DATABASE
$FIREBIRD/bin/gfix -buffers 20480 $DATABASE
$FIREBIRD/bin/gfix -use full $DATABASE
exec $FIREBIRD/bin/firebird
