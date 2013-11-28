#!/bin/sh
# Test the cxpath tool

set -e

TMP=/tmp
PATH=$PATH:`cd ..; pwd`/BUILD/snapwebsites/src

cat >$TMP/test.xml <<EOF
<?xml version="1.0"?>
<snap>
  <header>
    <metadata>
      <robotstxt>
        <noindex/>
        <nofollow/>
      </robotstxt>
      <sendmail>
        <param name="from">no-reply@m2osw.com</param>
	<param name="user-first-name">Alexis</param>
	<param name="user-last-name">Wilke</param>
	<param name="to">alexis@mail.example.com</param>
      </sendmail>
    </metadata>
  </header>
  <page>
    <date>
      <created>2013-11-27</created>
    </date>
  </page>
</snap>
EOF

echo "*** Get root"
cxpath -c -p '/' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get sendmail if a param is named to"
cxpath -c -p '/snap/header/metadata/sendmail/param[@name = "to"]/..' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get ancestors of sendmail"
cxpath -c -p '/snap/header/metadata/sendmail/ancestor::*' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get sendmail param named user-first-name"
cxpath -c -p '/snap/header/metadata/sendmail/param[@name = "user-first-name"]|/snap/header/metadata/sendmail/param[@name = "user-last-name"]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

