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
  <xsl:variable name="editor-modified">2014-03-16 00:36:48</xsl:variable>

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

  <!-- DROPPED FILE WITH PREVIEW WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not properly get defined without such trickery -->
  <xsl:template name="snap:dropped-file-with-preview">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div field_type="dropped-file-with-preview">
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class"><xsl:if
            test="$action = 'edit'">snap-editor </xsl:if>editable dropped-file-with-preview-box <xsl:value-of
            select="$name"/><xsl:if test="@drop or /editor-form/drop"> drop</xsl:if><xsl:if
            test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
            test="$name = /editor-form/focus/@refid"> auto-focus</xsl:if> <xsl:value-of
            select="classes"/></xsl:attribute>
        <xsl:if test="background-value != ''">
          <!-- by default "snap-editor-background" objects have "display: none"
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
          <!-- TBD: should we use "image" instead of "attachment" since we show images? -->
          <xsl:attribute name="class">editor-content attachment dropped-file-with-preview<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if><xsl:if test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
          <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
            <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="sizes/min"><xsl:attribute name="min-sizes"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
          <xsl:if test="sizes/resize"><xsl:attribute name="resize-sizes"><xsl:value-of select="sizes/resize"/></xsl:attribute></xsl:if>
          <xsl:if test="sizes/max"><xsl:attribute name="max-sizes"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
          <!-- now the actual value of this line -->
          <xsl:choose>
            <xsl:when test="post != ''">
              <!-- use the post value when there is one, it has priority -->
              <xsl:copy-of select="post/node()"/>
            </xsl:when>
            <xsl:when test="value != ''">
              <!-- use the current value when there is one -->
              <xsl:copy-of select="value/node()"/>
            </xsl:when>
          </xsl:choose>
        </div>
        <xsl:call-template name="snap:common-parts"/>
      </div>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='dropped-file-with-preview']">
    <xsl:call-template name="snap:dropped-file-with-preview">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- IMAGE BOX WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not properly get defined without such trickery -->
  <xsl:template name="snap:image-box">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div field_type="image-box">
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class"><xsl:if
            test="$action = 'edit'">snap-editor </xsl:if>editable image-box <xsl:value-of
            select="$name"/><xsl:if test="@drop or /editor-form/drop"> drop</xsl:if><xsl:if
            test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
            test="$name = /editor-form/focus/@refid"> auto-focus</xsl:if> <xsl:value-of
            select="classes"/></xsl:attribute>
        <xsl:if test="background-value != ''">
          <!-- by default "snap-editor-background" objects have "display: none"
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
          <xsl:attribute name="class">editor-content image<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if><xsl:if test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
          <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
            <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="sizes/min"><xsl:attribute name="min-sizes"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
          <xsl:if test="sizes/resize"><xsl:attribute name="resize-sizes"><xsl:value-of select="sizes/resize"/></xsl:attribute></xsl:if>
          <xsl:if test="sizes/max"><xsl:attribute name="max-sizes"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
          <!-- now the actual value of this line -->
          <xsl:choose>
            <xsl:when test="post != ''">
              <!-- use the post value when there is one, it has priority -->
              <xsl:copy-of select="post/node()"/>
            </xsl:when>
            <xsl:when test="value != ''">
              <!-- use the current value when there is one -->
              <xsl:copy-of select="value/node()"/>
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

  <!-- DROPDOWN WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not properly get defined without such trickery -->
  <xsl:template name="snap:dropdown">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div field_type="dropdown">
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable <xsl:value-of select="classes"/> dropdown <xsl:value-of select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:if test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
        <xsl:if test="background-value != ''">
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
          <xsl:attribute name="class">editor-content<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if><xsl:if test="not(@mode) or @mode = 'select-only'"> read-only</xsl:if></xsl:attribute>
          <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
            <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="not(@mode) or @mode = 'select-only'">
            <!-- avoid spellcheck of non-editable widgets -->
            <xsl:attribute name="spellcheck">false</xsl:attribute>
          </xsl:if>

          <xsl:choose>
            <xsl:when test="value/item[@default='default']">
              <xsl:attribute name="value"><xsl:copy-of select="value/item[@default='default']/@value"/></xsl:attribute>
              <xsl:copy-of select="value/item[@default='default']/node()"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:copy-of select="default/node()"/>
            </xsl:otherwise>
          </xsl:choose>
        </div>
        <div>
          <xsl:choose>
            <xsl:when test="count(value/item)">
              <xsl:attribute name="class">dropdown-items zordered</xsl:attribute>
              <ul class="dropdown-selection">
                <xsl:for-each select="value/item">
                  <li>
                    <xsl:attribute name="class">dropdown-item<xsl:if test="@default='default'"> selected</xsl:if></xsl:attribute>
                    <xsl:if test="@value"><xsl:attribute name="value"><xsl:value-of select="@value"/></xsl:attribute></xsl:if>
                    <xsl:copy-of select="./node()"/>
                  </li>
                </xsl:for-each>
              </ul>
            </xsl:when>
            <xsl:otherwise>
              <xsl:attribute name="class">dropdown-items zordered disabled</xsl:attribute>
              <div class="no-selection">No selection...</div>
            </xsl:otherwise>
          </xsl:choose>
        </div>
        <xsl:if test="required = 'required'"> <span class="required">*</span></xsl:if>

        <xsl:call-template name="snap:common-parts"/>
      </div>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='dropdown']">
    <xsl:variable name="value">
      <xsl:choose>
        <!-- use the post value when there is one, it has priority -->
        <xsl:when test="post != ''"><xsl:copy-of select="post/node()"/></xsl:when>
        <!-- use the current value when there is one -->
        <xsl:when test="value != ''"><xsl:copy-of select="value/node()"/></xsl:when>
        <xsl:otherwise></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:call-template name="snap:dropdown">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- CHECKMARK WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not properly get defined without such trickery -->
  <xsl:template name="snap:checkmark">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <xsl:param name="value"/>
    <widget path="{$path}">
      <div field_type="checkmark">
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable <xsl:value-of select="classes"/> checkmark <xsl:value-of select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:if test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
        <div>
          <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
          <xsl:attribute name="class">editor-content<xsl:if test="@no-toolbar or /editor-form/no-toolbar"> no-toolbar</xsl:if></xsl:attribute>
          <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
            <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>

          <div class="checkmark-flag">
            <div class="flag-box"></div>
            <!-- the actual value of a checkmark is used to know whether the checkmark is shown or not -->
            <div><xsl:attribute name="class">checkmark-area<xsl:if test="$value != '0'"> checked</xsl:if></xsl:attribute></div>
          </div>

          <xsl:copy-of select="label/node()"/>
          <xsl:if test="required = 'required'"> <span class="required">*</span></xsl:if>
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
          <xsl:copy-of select="post/node()"/>
        </xsl:when>
        <xsl:when test="value != ''">
          <!-- use the current value when there is one -->
          <xsl:copy-of select="value/node()"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:call-template name="snap:checkmark">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
      <xsl:with-param name="value" select="$value"/>
    </xsl:call-template>
  </xsl:template>

  <!-- TEXT EDIT WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:text-edit">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div field_type="text-edit">
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class"><xsl:if
              test="$action = 'edit'">snap-editor </xsl:if>editable text-edit <xsl:value-of select="$name"/><xsl:if
              test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if
              test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if> <xsl:value-of select="classes"/><xsl:if
              test="state = 'disabled'"> disabled</xsl:if><xsl:if
              test="state = 'read-only'"> read-only</xsl:if></xsl:attribute>
        <xsl:if test="@state = 'read-only' or @state = 'disabled'">
          <!-- avoid spellcheck of non-editable widgets -->
          <xsl:attribute name="spellcheck">false</xsl:attribute>
        </xsl:if>
        <xsl:if test="background-value != ''">
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
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="sizes/min"><xsl:attribute name="minlength"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
          <xsl:if test="sizes/max"><xsl:attribute name="maxlength"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
          <!-- now the actual value of this text widget -->
          <xsl:choose>
            <xsl:when test="post != ''">
              <!-- use the post value when there is one, it has priority -->
              <xsl:copy-of select="post/node()"/>
            </xsl:when>
            <xsl:when test="value != ''">
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
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='text-edit']">
    <xsl:call-template name="snap:text-edit">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- LINE EDIT WIDGET -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:line-edit">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div field_type="line-edit">
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor </xsl:if>editable line-edit <xsl:value-of select="$name"/><xsl:if test="@immediate or /editor-form/immediate"> immediate</xsl:if><xsl:if test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if> <xsl:value-of select="classes"/><xsl:if test="state = 'disabled'"> disabled</xsl:if><xsl:if test="state = 'read-only'"> read-only</xsl:if></xsl:attribute>
        <xsl:if test="@state = 'read-only' or @state = 'disabled'">
          <!-- avoid spellcheck of non-editable widgets -->
          <xsl:attribute name="spellcheck">false</xsl:attribute>
        </xsl:if>
        <xsl:if test="background-value != ''">
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
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="sizes/min"><xsl:attribute name="minlength"><xsl:value-of select="sizes/min"/></xsl:attribute></xsl:if>
          <xsl:if test="sizes/max"><xsl:attribute name="maxlength"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
          <!-- now the actual value of this line -->
          <xsl:choose>
            <xsl:when test="post != ''">
              <!-- use the post value when there is one, it has priority -->
              <xsl:copy-of select="post/node()"/>
            </xsl:when>
            <xsl:when test="value != ''">
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
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='line-edit']">
    <xsl:call-template name="snap:line-edit">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- BUTTON WIDGET (an anchor) -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:button">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <a field_type="button">
        <!-- name required as a field name? xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute-->
        <xsl:attribute name="class"><xsl:value-of select="classes"/> button <xsl:value-of select="$name"/><xsl:if test="@id = /editor-form/focus/@refid"> auto-focus</xsl:if><xsl:if test="state = 'disabled'"> disabled</xsl:if></xsl:attribute>
        <xsl:if test="/editor-form/taborder/tabindex[@refid=$name]">
          <xsl:attribute name="tabindex"><xsl:value-of select="/editor-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1 + $tabindex_base"/></xsl:attribute>
        </xsl:if>
        <xsl:if test="tooltip">
          <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
        </xsl:if>
        <xsl:attribute name="href"><xsl:copy-of select="value/node()"/></xsl:attribute>
        <!-- use the label as the anchor text -->
        <xsl:copy-of select="label/node()"/>
      </a>
      <xsl:call-template name="snap:common-parts"/>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='button']">
    <xsl:call-template name="snap:button">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- CUSTOM ("user" data, we just copy the <value> tag over) -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:custom">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div field_type="custom" style="display: none;">
        <xsl:attribute name="class"><xsl:value-of select="classes"/> snap-editor-custom <xsl:value-of select="$name"/></xsl:attribute>
        <div class="snap-content">
          <xsl:copy-of select="value/node()"/>
        </div>
      </div>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='custom']">
    <xsl:call-template name="snap:custom">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- SILENT (a value such as the editor form session identifier which
              is not returned to the editor on a Save) -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:silent">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div field_type="silent">
        <xsl:attribute name="class"><xsl:value-of select="classes"/> snap-editor-silent <xsl:value-of select="$name"/></xsl:attribute>
        <div class="snap-content">
          <xsl:copy-of select="value/node()"/>
        </div>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='silent']">
    <xsl:call-template name="snap:silent">
      <xsl:with-param name="path" select="@path"/>
      <xsl:with-param name="name" select="@id"/>
    </xsl:call-template>
  </xsl:template>

  <!-- HIDDEN (a value hidden from the user and always returned on Save) -->
  <!-- WARNING: we use this sub-template because of a Qt bug with variables
                that do not get defined properly without such trickery -->
  <xsl:template name="snap:hidden">
    <xsl:param name="path"/>
    <xsl:param name="name"/>
    <widget path="{$path}">
      <div field_type="hidden">
        <xsl:attribute name="field_name"><xsl:value-of select="$name"/></xsl:attribute>
        <xsl:attribute name="class"><xsl:value-of select="classes"/> snap-editor snap-editor-hidden <xsl:value-of select="$name"/></xsl:attribute>
        <div class="editor-content">
          <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
          <xsl:copy-of select="value/node()"/>
        </div>
      </div>
      <xsl:call-template name="snap:common-parts"/>
    </widget>
  </xsl:template>
  <xsl:template match="widget[@type='hidden']">
    <xsl:call-template name="snap:hidden">
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
