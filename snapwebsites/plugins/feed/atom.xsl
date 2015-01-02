<?xml version="1.0"?>
<!--
Snap Websites Server == feed page data to Atom XML
Copyright (C) 2011-2015  Made to Order Software Corp.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
-->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
															xmlns:xs="http://www.w3.org/2001/XMLSchema"
															xmlns:fn="http://www.w3.org/2005/xpath-functions"
															xmlns:snap="span:snap">
  <!-- get the year the page was created -->
  <xsl:variable name="year" select="year-from-date(current-date())"/>
  <xsl:variable name="created_year" select="year-from-date(/snap/page/body/created)"/>
  <xsl:variable name="year_range">
		<xsl:value-of select="$created_year"/><xsl:if test="$created_year != xs:integer($year)">-<xsl:value-of select="$year"/></xsl:if>
  </xsl:variable>

	<xsl:template match="output">
		<entry>
			<title type="xhtml"><div ns="xmlns=http://www.w3.org/1999/xhtml"><xsl:copy-of select="titles/title/node()"/></div></title>
			<link href="{url/node()}" rel="alternate" type="text/html"/>
			<!-- TODO: once we manage languages properly, add one alternate link per
			     language except the language being managed here -->
			<xsl:if test="teaser">
				<summary type="xhtml"><xsl:copy-of select="teaser/node()"/></summary>
			</xsl:if>
			<content type="xhtml" xml:lang="{@lang}" base="{/snap/head/metadata/desc[@base_uri]/data/node()}">
				<div ns="xmlns=http://www.w3.org/1999/xhtml" class="content">
					<xsl:copy-of select="description/div/node()"/>
				</div>
			</content>
			<xsl:copy-of select="author"/>
			<!--link href="..." rel="enclosure" type=".../..." length="..."/-->
			<!--author>
				<name>...</name>
				<uri>.../user/...</uri>
				<email>abc@example.com</email>
			</author-->
			<id><xsl:copy-of select="url/node()"/></id>
			<published><xsl:copy-of select="created/node()"/></published>
			<xsl:choose>
				<xsl:when test="modified/node()">
					<updated><xsl:copy-of select="modified/node()"/></updated>
				</xsl:when>
				<xsl:otherwise>
					<updated><xsl:copy-of select="created/node()"/></updated>
				</xsl:otherwise>
			</xsl:choose>
			<!--source url="{/snap/head/metadata/desc[@type='website_uri']/data/node()}"><xsl:copy-of select="/snap/head/metadata/desc[@type='name']/data/node()"/></source-->
		</entry>
	</xsl:template>

	<xsl:template match="snap">
		<feed ns="xmlns=http://www.w3.org/2005/Atom">
			<snap-info extension="atom" mimetype="application/atom+xml">
				<title><xsl:copy-of select="head/metadata/desc[@type='name']/data/node()"/></title>
			</snap-info>
			<title type="xhtml"><div ns="xmlns=http://www.w3.org/1999/xhtml"><xsl:copy-of select="head/metadata/desc[@type='name']/data/node()"/></div></title>
			<!-- TODO: see whether we get a slogan like thingy to put in the sub-title -->
			<!--sub-title type="xhtml"><xsl:copy-of select="head/metadata/desc[@type='slogan']/data/node()"/></sub-title-->
			<id><xsl:value-of select="head/metadata/desc[@type='website_uri']/data/node()"/></id>
			<link href="{head/metadata/desc[@type='website_uri']/data/node()}" rel="via" type="text/html" title="Atom Source"/>
			<link href="{head/metadata/desc[@type='feed::uri']/data/node()}/{head/metadata/desc[@type='feed::name']/data/node()}.atom" rel="self" type="application/rss+xml" title="Atom Data"/>
			<!-- TODO: once we manage languages properly, add one alternate link per
			     language except the language being managed here -->
			<xsl:if test="page/body/owner != ''">
				<rights>Copyright &#xA9; <xsl:copy-of select="$year_range"/> by <xsl:copy-of select="page/body/owner"/></rights>
			</xsl:if>
			<!--author><!== owner of the website ==>
				<name>...</name>
				<uri>.../user/...</uri>
				<email>abc@example.com</email>
			</author-->
			<updated><xsl:value-of select="head/metadata/desc[@type='feed::now']/data/node()"/></updated>
			<generator uri="http://snapwebsites.org/"><xsl:attribute
					name="version"><xsl:value-of
					select="head/metadata/desc[@type='version']/data/node()"/></xsl:attribute>Snap! Websites</generator>
			<!--logo>...url...</logo-->

			<xsl:apply-templates select="page/body/output"/>
		</feed>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
