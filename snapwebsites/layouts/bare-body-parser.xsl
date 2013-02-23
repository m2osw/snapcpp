<?xml version="1.0"?>
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
															xmlns:xs="http://www.w3.org/2001/XMLSchema"
															xmlns:fn="http://www.w3.org/2005/xpath-functions"
															xmlns:snap="snap:snap">
	<xsl:param name="layout-name">bare</xsl:param>
	<xsl:param name="layout-area">body</xsl:param>
	<xsl:param name="layout-modified">2013-01-01 02:53:54</xsl:param>
	<xsl:param name="year" select="year-from-date(current-date())"/>
	<xsl:param name="use_dcterms">yes</xsl:param>
	<!-- get the website URI (i.e. URI without any folder other than the website base folder) -->
	<xsl:variable name="website_uri" as="xs:string">
		<xsl:for-each select="snap">
			<xsl:value-of select="head/metadata/desc[@type='website_uri']/data"/>
		</xsl:for-each>
	</xsl:variable>
	<!-- compute the protocol from the main URI -->
	<xsl:variable name="protocol" as="xs:string">
		<xsl:value-of select="substring-before($website_uri, '://')"/>
	</xsl:variable>
	<!-- compute the domain from the main URI -->
	<xsl:variable name="domain" as="xs:string">
		<xsl:choose>
			<xsl:when test="contains($website_uri, '://www.')">
				<xsl:value-of select="substring-before(substring-after($website_uri, '://www.'), '/')"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="substring-before(substring-after($website_uri, '://'), '/')"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>
	<!-- get the base URI (i.e. parent URI to this page) -->
	<xsl:variable name="base_uri" as="xs:string">
		<xsl:for-each select="snap">
			<xsl:value-of select="head/metadata/desc[@type='base_uri']/data"/>
		</xsl:for-each>
	</xsl:variable>
	<!-- compute the path from the main URI to this page -->
	<xsl:variable name="path" as="xs:string">
		<xsl:value-of select="substring-after($base_uri, $website_uri)"/>
	</xsl:variable>
	<!-- compute the full path from the main URI to this page -->
	<xsl:variable name="full_path" as="xs:string">
		<xsl:value-of select="concat(substring-after(substring-after($website_uri, '://'), '/'), $path)"/>
	</xsl:variable>
	<!-- get the page URI (i.e. the full path to this page) -->
	<xsl:variable name="page_uri" as="xs:string">
		<xsl:for-each select="snap">
			<xsl:value-of select="head/metadata/desc[@type='page_uri']/data"/>
		</xsl:for-each>
	</xsl:variable>
	<!-- compute the full path from the main URI to this page -->
	<xsl:variable name="page" as="xs:string">
		<xsl:value-of select="substring-after($page_uri, $base_uri)"/>
	</xsl:variable>
	<!-- get the year the page was created -->
	<xsl:variable name="created_year" select="year-from-date(snap/page/body/created)"/>
	<xsl:variable name="year_range">
		<xsl:value-of select="$created_year"/><xsl:if test="$created_year != xs:integer($year)">-<xsl:value-of select="$year"/></xsl:if>
	</xsl:variable>
	<!-- get the page language -->
	<xsl:variable name="lang">
		<xsl:choose><!-- make sure we get some language, we default to English -->
			<xsl:when test="snap/page/body/lang"><xsl:value-of select="snap/page/body/lang"/></xsl:when>
			<xsl:otherwise>en</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>

	<!-- transform the specified path in a full path as required -->
	<xsl:function name="snap:prepend-base" as="xs:string">
		<xsl:param name="website_uri" as="xs:string"/>
		<xsl:param name="base_uri" as="xs:string"/>
		<xsl:param name="path" as="xs:string"/>
		<xsl:variable name="lpath"><xsl:value-of select="lower-case($path)"/></xsl:variable>

		<xsl:choose>
			<xsl:when test="matches($lpath, '^[a-z]+://')">
				<!-- full path, use as is -->
				<xsl:value-of select="$path"/>
			</xsl:when>
			<xsl:when test="starts-with($lpath, '/')">
				<!-- root path, use with website URI -->
				<xsl:value-of select="concat($website_uri, substring($path, 2))"/>
			</xsl:when>
			<xsl:otherwise>
				<!-- relative path, use with base URI -->
				<xsl:value-of select="concat($base_uri, $path)"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:function>
	<!--xsl:template name="snap:content" match="@*|node()">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template-->
	<xsl:template name="snap:message">
		<xsl:param name="type"/>
		<xsl:param name="id"/>
		<xsl:param name="title"/>
		<xsl:param name="body"/>
		<div id="{$id}" class="message message-{$type}">
			<xsl:if test="not($body)">
				<xsl:attribute name="class">message message-<xsl:value-of select="$type"/> message-<xsl:value-of select="$type"/>-title-only</xsl:attribute>
			</xsl:if>
			<h2><xsl:value-of select="$title"/></h2>
			<xsl:if test="$body">
				<p><xsl:value-of select="$body"/></p>
			</xsl:if>
		</div>
	</xsl:template>

	<xsl:template match="snap">
		<!-- circumvent a QXmlQuery bug, if global variables are not accessed
		     at least once then they may appear as undefined in a function. -->
		<!--xsl:if test="$base_uri != ''"></xsl:if>
		<xsl:if test="$website_uri != ''"></xsl:if-->

		<output lang="{$lang}">
			<div id="content">
				<!-- add the messages at the top -->
				<xsl:if test="page/body/messages">
					<div class="user-messages">
						<xsl:for-each select="page/body/messages/message">
							<xsl:call-template name="snap:message">
								<xsl:with-param name="type" select="@type"/>
								<xsl:with-param name="id" select="@id"/>
								<xsl:with-param name="title" select="title"/>
								<xsl:with-param name="body" select="body"/>
							</xsl:call-template>
						</xsl:for-each>
					</div>
				</xsl:if>
				<xsl:copy-of select="page/body/content/*"/>
			</div>
			<div id="footer">White Footer</div>
			<!--div id="content2">
			<xsl:for-each select="page/body/content/*">
				<xsl:call-template name="snap:content"/>
			</xsl:for-each>
			</div-->
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
