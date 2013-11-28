#!/bin/sh
# Test the cxpath tool

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
        <from>no-reply@m2osw.com</from>
	<param name="user-first-name">Alexis</param>
	<to>alexis@mail.example.com</to>
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

echo "*** Get sendmail param named user-first-name"
cxpath -c -p '/snap/header/metadata/sendmail/param[@name = "user-first-name"]' -o $TMP/test.xpath
cxpath -d -r -x $TMP/test.xpath $TMP/test.xml

