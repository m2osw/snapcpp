<?xml version="1.0"?>
<!--
Snap Websites Server == date form XSLT, editor widget extensions
Copyright (C) 2014-2016  Made to Order Software Corp.

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
                              xmlns:snap="http://snapwebsites.info/snap-functions">
  <!-- xsl:variable name="editor-name">editor</xsl:variable>
  <xsl:variable name="editor-modified">2015-11-04 20:45:48</xsl:variable -->

  <!-- DATE EDIT WIDGET -->
  <!-- NOTE: we use a sub-template to allow for composite widgets -->
  <xsl:template name="snap:date-edit">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <div field_type="date-edit">
      <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable date-edit <xsl:value-of
        select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
        test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:value-of
        select="concat(' ', classes)"/><xsl:if test="state = 'disabled'"> disabled</xsl:if><xsl:if
        test="state = 'read-only'"> read-only</xsl:if><xsl:if
        test="state = 'auto-hide'"> auto-hide</xsl:if></xsl:attribute>
      <xsl:if test="state = 'read-only' or state = 'disabled'">
        <!-- avoid spellcheck of non-editable widgets -->
        <xsl:attribute name="spellcheck">false</xsl:attribute>
      </xsl:if>
      <xsl:if test="background-value">
        <!-- by default "snap-editor-background" has "display: none"
             a script shows them on load once ready AND if the value is empty
             also it is a "pointer-event: none;" -->
        <div class="snap-editor-background zordered">
          <div class="snap-editor-background-content">
            <!-- this div is placed OVER the next div -->
            <xsl:copy-of select="background-value/node()"/>
          </div>
        </div>
      </xsl:if>
      <div>
        <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">editor-content<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:copy-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:call-template name="snap:text-field-filters"/>

        <!-- now the actual value of this line -->
        <xsl:choose>
          <xsl:when test="post">
            <!-- use the post value when there is one, it has priority -->
            <xsl:copy-of select="post/node()"/>
          </xsl:when>
          <xsl:when test="value">
            <!-- use the current value when there is one -->
            <xsl:copy-of select="value/node()"/>
          </xsl:when>
          <xsl:when test="value/@default">
            <!-- transform the system default if one was defined -->
            <xsl:choose>
              <xsl:when test="value/@default = 'today'">
                <!-- US format & GMT... this should be a parameter, probably a variable we set in the editor before running the parser? -->
                <xsl:value-of select="month-from-date(current-date())"/>/<xsl:value-of select="day-from-date(current-date())"/>/<xsl:value-of select="year-from-date(current-date())"/>
              </xsl:when>
            </xsl:choose>
          </xsl:when>
        </xsl:choose>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </div>
    <!-- TODO: I think we should look into a better place for these includes -->
    <javascript name="date-widgets"/>
    <css name="date-widgets"/>
  </xsl:template>
  <xsl:template match="widget[@type='date-edit']">
    <widget path="{@path}">
      <xsl:call-template name="snap:date-edit">
        <xsl:with-param name="path" select="@path"/>
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </widget>
  </xsl:template>

</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
