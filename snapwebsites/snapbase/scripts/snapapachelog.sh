#!/bin/bash
LOGROTFILE="/etc/logrotate.d/apache2"

if [ -e "${LOGROTFILE}" ]
then
	if ! grep maxsize "${LOGROTFILE}" >&1 /dev/null
	then
		sed -i \
			-e "s/rotate 14/rotate 91/" \
			-e "s/}$/\tsu root adm\n\tmaxsize 10M\n}/" \
			${LOGROTFILE}
	fi
fi
