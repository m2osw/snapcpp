<?xml version="1.0"?>
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                              xmlns:xs="http://www.w3.org/2001/XMLSchema"
                              xmlns:fn="http://www.w3.org/2005/xpath-functions"
                              xmlns:snap="http://snapwebsites.info/snap-functions">
  <xsl:param name="form-name">core</xsl:param>
  <xsl:param name="form-modified">2012-11-14 03:44:54</xsl:param>
  <xsl:param name="year" select="year-from-date(current-date())"/>
  <xsl:param name="unique_id" select="34"/>

  <xsl:template name="snap:parse-widget">
    <xsl:param name="refid" select="@refid"/>
    <xsl:apply-templates select="../../widget[@id=$refid]"/>
  </xsl:template>

  <xsl:template name="snap:required">
    <xsl:if test="required = 'required'">
      <span class="form-item-required">*</span>
    </xsl:if>
  </xsl:template>

  <xsl:template name="snap:common-parts">
    <xsl:param name="type" select="@type"/>
    <xsl:if test="description != ''">
      <div class="form-description {$type}-description">
        <xsl:copy-of select="description/node()"/>
      </div>
    </xsl:if>
    <xsl:if test="help != ''">
      <!-- make this hidden by default because it is expected to be -->
      <div class="form-help {$type}-help" style="display: none;">
        <xsl:copy-of select="help/node()"/>
      </div>
    </xsl:if>
    <xsl:if test="error">
      <!-- there is an error message for this widget -->
      <div idref="{error/@idref}" class="form-error {$type}-error" style="display: none;">
        <h2><xsl:copy-of select="error/title/node()"/></h2>
        <xsl:if test="error/message">
          <p><xsl:copy-of select="error/message/node()"/></p>
        </xsl:if>
      </div>
    </xsl:if>
  </xsl:template>

  <!-- HIDDEN WIDGET -->
  <xsl:template match="widget[@type='hidden']">
    <input type="hidden">
      <xsl:attribute name="id"><xsl:value-of select="@id"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
      <xsl:attribute name="value"><xsl:value-of select="value"/></xsl:attribute>
    </input>
  </xsl:template>

  <!-- FIELDSET WIDGET -->
  <xsl:template name="snap:fieldset">
    <xsl:param name="name"/>
    <fieldset>
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="class"><xsl:value-of select="classes"/></xsl:attribute>
      <xsl:if test="label != ''">
        <legend>
          <xsl:if test="/snap-form/taborder/tabindex[@refid=$name]/position() != 0">
            <xsl:attribute name="tabindex"><xsl:value-of select="/snap-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="/snap-form/accesskeys/key[@refid=$name] != ''">
            <xsl:attribute name="accesskey"><xsl:value-of select="/snap-form/accesskeys/key[@refid=$name]"/></xsl:attribute>
          </xsl:if>
          <xsl:if test="tooltip != ''">
            <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
          </xsl:if>
          <xsl:value-of select="label"/>
        </legend>
      </xsl:if>
      <div class="field-set-content">
        <xsl:call-template name="snap:common-parts"/>
        <xsl:choose>
          <xsl:when test="widgetorder">
            <xsl:for-each select="widgetorder/widgetpriority">
              <xsl:call-template name="snap:parse-widget">
                <xsl:with-param name="refid" select="@refid"/>
              </xsl:call-template>
            </xsl:for-each>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="widget"/>
          </xsl:otherwise>
        </xsl:choose>
      </div>
    </fieldset>
  </xsl:template>
  <xsl:template match="widget[@type='fieldset']">
    <div class="form-item fieldset">
      <xsl:call-template name="snap:fieldset">
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
    </div>
  </xsl:template>

  <!-- LINE EDIT WIDGET -->
  <xsl:template name="snap:line-edit">
    <xsl:param name="name"/>
    <input type="text">
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class"> line-edit-input <xsl:value-of select="classes"/></xsl:attribute>
      <xsl:if test="/snap-form/taborder/tabindex[@refid=$name]/position() != 0">
        <xsl:attribute name="tabindex"><xsl:value-of select="/snap-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="/snap-form/accesskeys/key[@refid=$name] != ''">
        <xsl:attribute name="accesskey"><xsl:value-of select="/snap-form/accesskeys/key[@refid=$name]"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="tooltip != ''">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="help != ''">
        <!-- we use the help for the alternate because blind people will not see that text otherwise -->
        <xsl:attribute name="alt"><xsl:value-of select="help"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="sizes/width"><xsl:attribute name="size"><xsl:value-of select="sizes/width"/></xsl:attribute></xsl:if>
      <xsl:if test="sizes/max"><xsl:attribute name="maxlength"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
      <xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if>
      <xsl:if test="state = 'readonly'"><xsl:attribute name="readonly">readonly</xsl:attribute></xsl:if>
      <xsl:choose>
        <xsl:when test="post != ''">
          <!-- use the post value when there is one, it has priority -->
          <xsl:attribute name="value"><xsl:value-of select="post"/></xsl:attribute>
        </xsl:when>
        <xsl:when test="value != ''">
          <!-- use the current value when there is one -->
          <xsl:attribute name="value"><xsl:value-of select="value"/></xsl:attribute>
        </xsl:when>
      </xsl:choose>
    </input>
  </xsl:template>
  <xsl:template match="widget[@type='line-edit']">
    <div>
      <xsl:attribute name="class">form-item line-edit <xsl:if test="error">error</xsl:if></xsl:attribute>
      <xsl:choose>
        <xsl:when test="label != ''">
          <label class="line-edit-label">
            <xsl:attribute name="for"><xsl:value-of select="@id"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
            <xsl:if test="tooltip != ''">
              <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
            </xsl:if>
            <span class="line-edit-label-span"><xsl:value-of select="label"/>:</span>
            <xsl:call-template name="snap:required"/>
            <xsl:call-template name="snap:line-edit">
              <xsl:with-param name="name" select="@id"/>
            </xsl:call-template>
          </label>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="snap:line-edit">
            <xsl:with-param name="name" select="@id"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>

  <!-- PASSWORD WIDGET -->
  <xsl:template name="snap:password">
    <xsl:param name="name"/>
    <input type="password">
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class">password-input <xsl:value-of select="classes"/></xsl:attribute>
      <xsl:if test="/snap-form/taborder/tabindex[@refid=$name]/position() != 0">
        <xsl:attribute name="tabindex"><xsl:value-of select="/snap-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="/snap-form/accesskeys/key[@refid=$name] != ''">
        <xsl:attribute name="accesskey"><xsl:value-of select="/snap-form/accesskeys/key[@refid=$name]"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="@type = 'safe-password'">
        <!-- this generally doesn't work but some browsers may
             recognize it and not the form autocomplete... -->
        <xsl:attribute name="autocomplete">off</xsl:attribute>
      </xsl:if>
      <xsl:if test="tooltip != ''">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="help != ''">
        <!-- we use the help for the alternate because blind people will not see that text otherwise -->
        <xsl:attribute name="alt"><xsl:value-of select="help"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="sizes/width"><xsl:attribute name="size"><xsl:value-of select="sizes/width"/></xsl:attribute></xsl:if>
      <xsl:if test="sizes/max"><xsl:attribute name="maxlength"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
      <xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if>
      <xsl:if test="state = 'readonly'"><xsl:attribute name="readonly">readonly</xsl:attribute></xsl:if>
    </input>
  </xsl:template>
  <xsl:template match="widget[@type='password' or @type='safe-password']">
    <div>
      <xsl:attribute name="class">form-item password <xsl:if test="error">error</xsl:if></xsl:attribute>
      <xsl:choose>
        <xsl:when test="label != ''">
          <label class="password-label">
            <xsl:attribute name="for"><xsl:value-of select="@id"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
            <xsl:if test="tooltip != ''">
              <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
            </xsl:if>
            <span class="password-label-span"><xsl:value-of select="label"/>:</span>
            <xsl:call-template name="snap:required"/>
            <xsl:call-template name="snap:password">
              <xsl:with-param name="name" select="@id"/>
            </xsl:call-template>
          </label>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="snap:password">
            <xsl:with-param name="name" select="@id"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>

  <!-- CHECKBOX WIDGET -->
  <xsl:template name="snap:checkbox">
    <xsl:param name="name"/>
    <xsl:variable name="checked_status">
      <xsl:choose>
        <!-- use the post value when there is one, it has priority -->
        <xsl:when test="post = 'on'">checked</xsl:when>
        <xsl:when test="post = 'off'"></xsl:when>
        <xsl:when test="value = 'on'">checked</xsl:when>
      </xsl:choose>
    </xsl:variable>
    <input type="checkbox">
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class">checkbox-input <xsl:value-of select="classes"/></xsl:attribute>
      <xsl:if test="/snap-form/taborder/tabindex[@refid=$name]/position() != 0">
        <xsl:attribute name="tabindex"><xsl:value-of select="/snap-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="/snap-form/accesskeys/key[@refid=$name] != ''">
        <xsl:attribute name="accesskey"><xsl:value-of select="/snap-form/accesskeys/key[@refid=$name]"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="tooltip != ''">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="help != ''">
        <!-- we use the help for the alternate because blind people will not see that text otherwise -->
        <xsl:attribute name="alt"><xsl:value-of select="help"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="sizes/width"><xsl:attribute name="size"><xsl:value-of select="sizes/width"/></xsl:attribute></xsl:if>
      <xsl:if test="sizes/max"><xsl:attribute name="maxlength"><xsl:value-of select="sizes/max"/></xsl:attribute></xsl:if>
      <xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if>
      <xsl:if test="$checked_status != ''"><xsl:attribute name="checked">checked</xsl:attribute></xsl:if>
    </input>
    <!-- checkboxes have a bug in Firefox browsers (and maybe others)
         on Reload they don't get reset as expected, the following script
         fixes the problem by setting the state as it should be on load -->
    <script type="text/javascript"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/>.checked="<xsl:value-of select="$checked_status"/>";</script>
  </xsl:template>
  <xsl:template match="widget[@type='checkbox']">
    <div class="form-item checkbox">
      <xsl:choose>
        <xsl:when test="label != ''">
          <label class="checkbox-label">
            <xsl:attribute name="for"><xsl:value-of select="@id"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
            <xsl:if test="tooltip != ''">
              <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
            </xsl:if>
            <xsl:call-template name="snap:required"/>
            <xsl:call-template name="snap:checkbox">
              <xsl:with-param name="name" select="@id"/>
            </xsl:call-template>
            <span class="checkbox-label-span"><xsl:value-of select="label"/></span>
          </label>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="snap:checkbox">
            <xsl:with-param name="name" select="@id"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>

  <!-- FILE WIDGET -->
  <xsl:template name="snap:file">
    <xsl:param name="name" select="@id"/>
    <input type="file">
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="class">file <xsl:value-of select="classes"/></xsl:attribute>
      <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:if test="/snap-form/taborder/tabindex[@refid=$name]/position() != 0">
        <xsl:attribute name="tabindex"><xsl:value-of select="/snap-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="/snap-form/accesskeys/key[@refid=$name] != ''">
        <xsl:attribute name="accesskey"><xsl:value-of select="/snap-form/accesskeys/key[@refid=$name]"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="tooltip != ''">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if>
    </input>
  </xsl:template>
  <xsl:template match="widget[@type='file']">
    <div class="form-item file">
      <xsl:choose>
        <xsl:when test="label != ''">
          <label class="file-label">
            <xsl:attribute name="for"><xsl:value-of select="@id"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
            <xsl:if test="tooltip != ''">
              <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
            </xsl:if>
            <span class="file-label-span"><xsl:value-of select="label"/>:</span>
            <xsl:call-template name="snap:required"/>
            <xsl:call-template name="snap:file">
              <xsl:with-param name="name" select="@id"/>
            </xsl:call-template>
          </label>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="snap:file">
            <xsl:with-param name="name" select="@id"/>
          </xsl:call-template>
          <xsl:call-template name="snap:required"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>

  <!-- IMAGE WIDGET -->
  <xsl:template name="snap:image">
    <xsl:param name="name" select="@id"/>
    <xsl:variable name="existing_image">
      <xsl:choose>
        <!-- use the post value when there is one, it has priority -->
        <xsl:when test="post != ''"><xsl:copy-of select="post"/></xsl:when>
        <xsl:when test="value != ''"><xsl:copy-of select="value"/></xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$existing_image">
      <xsl:copy-of select="$existing_image"/>
    </xsl:if>
    <input type="file">
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class">file <xsl:value-of select="classes"/></xsl:attribute>
      <xsl:if test="/snap-form/taborder/tabindex[@refid=$name]/position() != 0">
        <xsl:attribute name="tabindex"><xsl:value-of select="/snap-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="/snap-form/accesskeys/key[@refid=$name] != ''">
        <xsl:attribute name="accesskey"><xsl:value-of select="/snap-form/accesskeys/key[@refid=$name]"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="tooltip != ''">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if>
    </input>
  </xsl:template>
  <xsl:template match="widget[@type='image']">
    <div class="form-item file image">
      <xsl:choose>
        <xsl:when test="label != ''">
          <label class="image-label">
            <xsl:attribute name="for"><xsl:value-of select="@id"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
            <xsl:if test="tooltip != ''">
              <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
            </xsl:if>
            <span class="image-label-span"><xsl:value-of select="label"/>:</span>
            <xsl:call-template name="snap:required"/>
            <xsl:call-template name="snap:image">
              <xsl:with-param name="name" select="@id"/>
            </xsl:call-template>
          </label>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="snap:image">
            <xsl:with-param name="name" select="@id"/>
          </xsl:call-template>
          <xsl:call-template name="snap:required"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>

  <!-- SUBMIT BUTTON WIDGET -->
  <xsl:template name="snap:submit">
    <xsl:param name="name" select="@id"/>
    <input type="submit" disabled="disabled">
      <xsl:variable name="default_button"><xsl:if test="default-button/@refid = @id">default-button</xsl:if></xsl:variable>
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="name"><xsl:value-of select="$name"/></xsl:attribute>
      <xsl:attribute name="class">submit-input <xsl:value-of select="classes"/> <xsl:value-of select="$default_button"/></xsl:attribute>
      <xsl:if test="/snap-form/taborder/tabindex[@refid=$name]/position() != 0">
        <xsl:attribute name="tabindex"><xsl:value-of select="/snap-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="/snap-form/accesskeys/key[@refid=$name] != ''">
        <xsl:attribute name="accesskey"><xsl:value-of select="/snap-form/accesskeys/key[@refid=$name]"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="tooltip != ''">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="help != ''">
        <!-- we use the help for the alternate because blind people will not see that text otherwise -->
        <xsl:attribute name="alt"><xsl:value-of select="help"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="sizes/width"><xsl:attribute name="size"><xsl:value-of select="sizes/width"/></xsl:attribute></xsl:if>
      <!--xsl:if test="state = 'disabled'"><xsl:attribute name="disabled">disabled</xsl:attribute></xsl:if-->
      <xsl:attribute name="value"><xsl:value-of select="value"/></xsl:attribute>
    </input>
  </xsl:template>
  <xsl:template match="widget[@type='submit']">
    <div class="form-item submit">
      <xsl:call-template name="snap:submit">
        <xsl:with-param name="name" select="@id"/>
      </xsl:call-template>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>

  <!-- LINK WIDGET -->
  <xsl:template name="snap:link">
    <xsl:param name="name" select="@id"/>
    <a>
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="class">link <xsl:value-of select="classes"/></xsl:attribute>
      <xsl:attribute name="href"><xsl:value-of select="value"/></xsl:attribute>
      <xsl:if test="/snap-form/taborder/tabindex[@refid=$name]/position() != 0">
        <xsl:attribute name="tabindex"><xsl:value-of select="/snap-form/taborder/tabindex[@refid=$name]/count(preceding-sibling::tabindex) + 1"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="/snap-form/accesskeys/key[@refid=$name] != ''">
        <xsl:attribute name="accesskey"><xsl:value-of select="/snap-form/accesskeys/key[@refid=$name]"/></xsl:attribute>
      </xsl:if>
      <xsl:if test="tooltip != ''">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:value-of select="label"/>
    </a>
  </xsl:template>
  <xsl:template name="snap:disabled_link">
    <xsl:param name="name" select="@id"/>
    <span>
      <xsl:attribute name="id"><xsl:value-of select="$name"/>_<xsl:value-of select="$unique_id"/></xsl:attribute>
      <xsl:attribute name="class">disabled-link <xsl:value-of select="classes"/></xsl:attribute>
      <xsl:if test="tooltip != ''">
        <xsl:attribute name="title"><xsl:value-of select="tooltip"/></xsl:attribute>
      </xsl:if>
      <xsl:value-of select="label"/>
    </span>
  </xsl:template>
  <xsl:template match="widget[@type='link']">
    <div class="form-item link">
      <xsl:choose>
        <xsl:when test="state = 'disabled'">
          <xsl:call-template name="snap:disabled_link">
            <xsl:with-param name="name" select="@id"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="snap:link">
            <xsl:with-param name="name" select="@id"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="snap:common-parts"/>
    </div>
  </xsl:template>

  <!-- THE FORM AS A WHOLE -->
  <xsl:template match="snap-form">
    <!--
      WARNING: do not expect the form-wrapper div to appear in the output as
               the form process removes it when used with a [form::...] token.
    -->
    <div class="form-wrapper">
      <div class="snap-form">
        <form id="form_{$unique_id}" method="post" accept-charset="utf-8">
          <xsl:if test="(@safe = 'safe') or (auto-reset) or ((//widget)[@type='safe-password'])">
            <xsl:attribute name="autocomplete">off</xsl:attribute>
          </xsl:if>
          <xsl:if test="(//widget)[@type='file' or @type='image']">
            <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>
          </xsl:if>
          <xsl:if test="default-button/@refid">
            <!-- the event check should be in a function and it should verify that the element with focus is an input[type=text] -->
            <xsl:attribute name="onkeypress">javascript:if((event.which&amp;&amp;event.which==13)||(event.keyCode&amp;&amp;event.keyCode==13))fire_event(<xsl:value-of select="default-button/@refid"/>_<xsl:value-of select="$unique_id"/>,'click');</xsl:attribute>
          </xsl:if>
          <input id="form_session" name="form_session" type="hidden" value="{$form_session}"/>
          <xsl:choose>
            <xsl:when test="widgetorder">
              <xsl:for-each select="widgetorder/widgetpriority">
                <xsl:call-template name="snap:parse-widget">
                  <xsl:with-param name="refid" select="@refid"/>
                </xsl:call-template>
              </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="widget"/>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:if test="focus/@refid">
            <!-- force focus in the specified widget -->
            <script type="text/javascript"><xsl:value-of select="focus/@refid"/>_<xsl:value-of select="$unique_id"/>.focus();<xsl:value-of select="focus/@refid"/>_<xsl:value-of select="$unique_id"/>.select();</script>
          </xsl:if>
          <xsl:if test="auto-reset">
            <!-- TODO: reset timer each time the user makes a modification so it
                       doesn't trigger too soon! -->
            <script type="text/javascript">function auto_reset_<xsl:value-of select="$unique_id"/>(){form_<xsl:value-of select="$unique_id"/>.reset();}window.setInterval(auto_reset_<xsl:value-of select="$unique_id"/>,<xsl:value-of select="auto-reset/@minutes * 60000"/>);</script>
          </xsl:if>
          <xsl:if test="default-button/@refid">
            <script type="text/javascript">
              <!-- TODO: move this one in a JS -->
              function fire_event(element, event_type)
              {
                if(element.fireEvent)
                {
                  element.fireEvent('on' + event_type);
                }
                else
                {
                  var event = document.createEvent('Events');
                  event.initEvent(event_type, true, false);
                  element.dispatchEvent(event);
                }
              }
            </script>
          </xsl:if>
        </form>
        <!-- enable buttons after the form is fully loaded to avoid problems (early submissions) -->
        <xsl:for-each select="//widget[@type='submit']">
          <xsl:if test="not(state = 'disabled')">
            <script type="text/javascript"><xsl:value-of select="@id"/>_<xsl:value-of select="$unique_id"/>.disabled="";</script>
          </xsl:if>
        </xsl:for-each>
      </div>
    </div>
  </xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2 et
-->
