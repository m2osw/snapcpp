#!/bin/bash

if [ "$#" -ne 1 ]
then
  echo "usage: $0 DOMAIN"
  exit 1
fi

ARG_DOMAIN=$1

if ! [ -d "/etc/postfix" ]
then
  echo "postfix is not installed!"
  exit 1
fi

################################################################################
# Add required milter stuff if not in the main.cf
#
MAIN=/etc/postfix/main.cf
if ! grep smtpd_milters ${MAIN} > /dev/null
then
  cat >> ${MAIN} <<EOF

########
# Set up SPF email verification, DKIM and DMARC
#
policy-spf_time_limit        = 3600s
smtpd_recipient_restrictions = permit_mynetworks,
permit_sasl_authenticated,
reject_unauth_destination,
check_policy_service unix:private/policy-spf
milter_protocol       = 6
milter_default_action = accept
smtpd_milters         = inet:localhost:12345,inet:localhost:54321
non_smtpd_milters     = inet:localhost:12345,inet:localhost:54321
EOF
fi

################################################################################
# Add the spf policy to the master.cf, if it isn't already in there.
#
MASTER=/etc/postfix/master.cf
if ! grep policy-spf ${MASTER} > /dev/null
then
  cat >> ${MASTER} <<EOF

########
# `date`
# Turn on SPF processing
policy-spf  unix  -       n       n       -       -       spawn
user=nobody argv=/usr/bin/policyd-spf
EOF
fi


################################################################################
# Setup SPF support
#
sed \
  -e "s/HELO_reject = SPF_Not_Pass/HELO_reject = False/" \
  -e "s/Mail_From_reject = Fail/Mail_From_reject = False/"
    /etc/postfix-policyd-spf-python/policyd-spf.conf > /etc/postfix-policyd-spf-python/policyd-spf.conf


################################################################################
# Set up DKIM support
#
OPENDKIM=/etc/opendkim.conf
if ! grep "^KeyTable.*/etc/opendkim/key_table" ${OPENDKIM} > /dev/null
then
  cat >> /etc/opendkim.conf <<EOF

########
# `date`
# Values used by Snap!
#
KeyTable              /etc/opendkim/key_table
SigningTable          /etc/opendkim/signing_table
ExternalIgnoreList    /etc/opendkim/trusted_hosts
InternalHosts         /etc/opendkim/trusted_hosts
AutoRestart           Yes
AutoRestartRate       10/1h
Mode                  sv
PidFile               /var/run/opendkim/opendkim.pid
SignatureAlgorithm    rsa-sha256
Canonicalization      relaxed/simple
UserID                opendkim:opendkim
EOF
fi
#
#
mkdir -p /etc/opendkim
if ! [ -f /etc/opendkim/trusted_hosts ]
then
  cat > /etc/opendkim/trusted_hosts <<EOF
# local host
127.0.0.1
# local subnets that are trusted and do not need to be verified
10.8.0.0/24
EOF
fi
#
if ! grep "^SOCKET=\"inet:12345@localhost\"" /etc/default/opendkim > /dev/null
then
  cat >> /etc/default/opendkim <<EOF
SOCKET="inet:12345@localhost"
EOF
fi

DOMAIN_DKIM_DIR=/etc/opendkim/${ARG_DOMAIN}
if [ -d ${DOMAIN_DKIM_DIR} ]
then
  # Blow away the old information and redo from start.
  rm -rf ${DOMAIN_DKIM_DIR}
fi
mkdir -p ${DOMAIN_DKIM_DIR}
cd ${DOMAIN_DKIM_DIR}
opendkim-genkey -s mail -d ${ARG_DOMAIN}
chown opendkim:opendkim mail.private
cd -
cat >> /etc/opendkim/key_table <<EOF
mail._domainkey.mail.${ARG_DOMAIN} ${ARG_DOMAIN}:mail:/etc/opendkim/${ARG_DOMAIN}/mail.private
EOF

cat >> /etc/opendkim/signing_table <<EOF
${ARG_DOMAIN} mail._domainkey.mail.${ARG_DOMAIN}
EOF


################################################################################
# Set up DMARC
#
mkdir -p /etc/opendmarc
if ! grep ^IgnoreHosts > /dev/null
then
  cat >> /etc/opendmarc.conf <<EOF

# `date`
# Values used by Snap!
#
AuthservID         mail.${ARG_DOMAIN}
TrustedAuthservIDs mail.${ARG_DOMAIN}
IgnoreHosts        /etc/opendmarc/ignore.hosts
HistoryFile        /var/run/opendmarc/opendmarc.dat
EOF
fi

cat > /etc/opendmarc/ignore.hosts <<EOF
localhost
10.8.0.0/24
EOF

if ! grep "^SOCKET=\"inet:54321@localhost\"" /etc/default/opendmarc > /dev/null
then
  cat >> /etc/default/opendmarc <<EOF
SOCKET="inet:54321@localhost"
EOF
fi


################################################################################
# Append the spf/dkim/dmarc TXT records to the zone file for our domain
#
# TODO: bump the serial number of the zone file.
#
ZONEFILE=/etc/bind/${ARG_DOMAIN}.zone
if [ -f ${ZONEFILE} ]
then
  if ! grep "^;==== ${ARG_DOMAIN} TXT records" > /dev/null
  then
    echo ";==== ${ARG_DOMAIN} TXT records" >> ${ZONEFILE}
    cat >> ${ZONEFILE} <<EOF
mail       1800 IN TXT "v=spf1 a:mail.${ARG_DOMAIN} -all"
adsp._domainkey.mail 1800 IN TXT "dkim=all"
EOF
    cat /etc/opendkim/${ARG_DOMAIN}/mail.txt >> ${ZONEFILE}
    cat >> ${ZONEFILE} <<EOF
_dmarc.mail          1800 IN TXT "v=DMARC1; p=quarantine; fo=0; adkim=r; aspf=r; pct=100; rf=afrf; sp=quarantine"
EOF
    echo ";==== end ${ARG_DOMAIN} TXT records" >> ${ZONEFILE}
  fi
fi

################################################################################
# (re)start services with the correct parameters
#
# also the tripwire bundle may disable postfix so here we make sure
# it is enabled
#
for i in opendkim opendmarc postfix bind9
do
  systemctl enable  $i
  systemctl restart $i
done

# vim: ts=2 sw=2 et
