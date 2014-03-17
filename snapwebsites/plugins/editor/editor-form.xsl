<?xml version="1.0"?>
<!--
Snap Websites Server == editor form XSLT, generate HTML from editor widgets
Copyright (C) 2014  Made to Order Software Corp.

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
  <xsl:variable name="editor-name">editor</xsl:variable>
  <xsl:variable name="editor-modified">2014-03-14 21:03:48</xsl:variable>

  <!-- COMMAND PARTS -->
  <xsl:template name="snap:common-parts">
    <xsl:param name="type" select="@type"/>
    <xsl:if test="help != ''">
      <!-- make this hidden by default because it is expected to be -->
      <div class="editor-help {$type}-help" style="display: none;">
        <xsl:copy-of select="help/node()"/>
      </div>
    </xsl:if>
  </xsl:template>

  <!-- IMAGE BOX WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt but with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:image-box">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div>
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">snap-editor editable image-box <xsl:value-of select="$name"/><xsl:if test="@drop or /editor-form/drop"> drop</xsl:if><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if test="$name = /editor-form/focus/@refid"> auto-focus</xsl:if> <xsl:value-of select="classes"/></xsl:attribute>
        <xsl:if test="background-value != ''">
          <!-- by default "snap-editor-background" have "display: none"
               a script shows them on load once ready AND if the value is empty
               also it is a "pointer-event: none;" -->
          <div class="snap-editor-background">
            <!-- this div is placed OVER the next div -->
            <xsl:copy-of select="background-value"/>
          </div>
        </xsl:if>
        <div>
          <xsl:attribute name="id">editor_widget_<xsl:value-of select="$name"/></xsl:attribute>
          <xsl:attribute name="class">editor-content image<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
          <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
            <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="sizes/min"><xsl:attribute name="min-sizes"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
          <xsl:if test="sizes/max"><xsl:attribute name="max-sizes"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
          <xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if>
          <!-- now the actual value of this line -->
          <xsl:choose>
            <xsl:when test="post != ''">
              <!-- use the post value when there is one, it has priority -->
              <xsl:value-of select="post"/>
            </xsl:when>
            <xsl:when test="value != ''">
              <!-- use the current value when there is one -->
              <xsl:value-of select="value"/>
            </xsl:when>
            <xsl:when test="default-value != ''">
              <!-- use the current value when there is one -->
              <xsl:value-of select="default-value"/>
            </xsl:when>
          </xsl:choose>
        </div>
        <xsl:call-template name="snap:common-parts"/>
      </div>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='image-box']">
    <xsl:call-template name="snap:image-box">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- CHECKMARK WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt but with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:checkmark">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="value"/>
    <widget path="{$path}">
      <div>
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">snap-editor editable checkmark <xsl:value-of select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if> <xsl:value-of select="classes"/></xsl:attribute>
        <div>
          <xsl:attribute name="id">editor_widget_<xsl:value-of select="$name"/></xsl:attribute>
          <xsl:attribute name="class">editor-content<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
          <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
            <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if>

          <div class="checkmark-flag">
            <div class="flag-box"></div>
            <!-- the actual value of a checkmark is used to know whether the checkmark is shown or not -->
            <div><xsl:attribute name="class">checkmark-area<xsl:if test="$value != 0"> checkmark</xsl:if></xsl:attribute></div>
          </div>

          <xsl:copy-of select="label/node()"/>
        </div>
        <xsl:call-template name="snap:common-parts"/>
      </div>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='checkmark']">
    <xsl:variable name="value">
      <xsl:choose>
        <xsl:when test="post != ''">
          <!-- use the post value when there is one, it has priority -->
          <xsl:value-of select="post"/>
        </xsl:when>
        <xsl:when test="value != ''">
          <!-- use the current value when there is one -->
          <xsl:value-of select="value"/>
        </xsl:when>
        <xsl:when test="default-value != ''">
          <!-- use the current value when there is one -->
          <xsl:value-of select="default-value"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:call-template name="snap:checkmark">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
      <xsl:with-param name="value" select="$value"/>
    </xsl:call-template>
  </xsl:template>

  <!-- LINE EDIT WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt but with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:line-edit">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div>
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class">snap-editor editable line-edit <xsl:value-of select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if> <xsl:value-of select="classes"/></xsl:attribute>
        <xsl:if test="background-value != ''">
          <!-- by default "snap-editor-background" have "display: none"
               a script shows them on load once ready AND if the value is empty
               also it is a "pointer-event: none;" -->
          <div class="snap-editor-background">
            <!-- this div is placed OVER the next div -->
            <xsl:copy-of select="background-value"/>
          </div>
        </xsl:if>
        <div>
          <xsl:attribute name="id">editor_widget_<xsl:value-of select="$name"/></xsl:attribute>
          <xsl:attribute name="class">editor-content<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
          <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
            <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="sizes/min"><xsl:attribute name="minlength"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
          <xsl:if test="sizes/max"><xsl:attribute name="maxlength"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
          <xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if>
          <!-- now the actual value of this line -->
          <xsl:choose>
            <xsl:when test="post != ''">
              <!-- use the post value when there is one, it has priority -->
              <xsl:value-of select="post"/>
            </xsl:when>
            <xsl:when test="value != ''">
              <!-- use the current value when there is one -->
              <xsl:value-of select="value"/>
            </xsl:when>
            <xsl:when test="default-value != ''">
              <!-- use the current value when there is one -->
              <xsl:value-of select="default-value"/>
            </xsl:when>
          </xsl:choose>
        </div>
        <xsl:call-template name="snap:common-parts"/>
      </div>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='line-edit']">
    <xsl:call-template name="snap:line-edit">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- THE EDITOR FORM AS A WHOLE -->
  <xsl:template match="editor-form">
    <!--
      WARNING: remember that this transformation generates many tags in the
               output, each of which goes in a different place in the XML
               document representing your current page.
    -->
    <xsl:apply-templates select="widget"/>
  </xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
