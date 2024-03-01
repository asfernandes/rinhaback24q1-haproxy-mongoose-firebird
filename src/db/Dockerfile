# syntax=docker/dockerfile:1

FROM ubuntu:23.10

ADD https://github.com/FirebirdSQL/firebird/releases/download/v5.0.0/Firebird-5.0.0.1306-0-linux-x64.tar.gz /tmp/

RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get -y install \
		libicu72 \
		libtommath1 \
		netcat-traditional && \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*

ENV FIREBIRD=/opt/firebird

RUN tar xzf /tmp/Firebird-5.0.0.1306-0-linux-x64.tar.gz -C /tmp && \
	tar xzf /tmp/Firebird-5.0.0.1306-0-linux-x64/buildroot.tar.gz -C / && \
	<<EOF cat > $FIREBIRD/firebird.conf
RemoteServicePort = 9996
ServerMode = Super
DefaultTimeZone = GMT+0
EOF

RUN $FIREBIRD/bin/gfix -write async $FIREBIRD/security5.fdb && \
	$FIREBIRD/bin/gfix -housekeeping 0 $FIREBIRD/security5.fdb && \
	echo "create user sysdba password 'masterkey';" | $FIREBIRD/bin/isql -q -user sysdba $FIREBIRD/security5.fdb

COPY ./src/db/database.sql /sql/database.sql
COPY ./src/db/start.sh /usr/bin/start.sh

HEALTHCHECK --interval=5s --timeout=5s --start-period=10s --start-interval=5s --retries=30 \
	CMD nc -z 127.0.0.1 9996 || exit 1

WORKDIR $FIREBIRD/bin

CMD ["/usr/bin/start.sh"]
