<?xml version="1.0"?>
<!--
Snap Websites Server == users settings parser
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
															xmlns:snap="snap:snap">

	<!-- some special variables to define the theme -->
    <xsl:variable name="layout-area">change-email-parser</xsl:variable>
    <xsl:variable name="layout-modified">2017-01-08 20:00:10</xsl:variable>
    <xsl:variable name="layout-editor">change-email-page</xsl:variable>

	<xsl:template match="snap">
		<output><!-- lang="{$lang}"-->
            <div id="content" class="editor-form" form_name="change-email">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

                <!--<h3>Change User Email Address</h3>-->
                <div class="settings editing">
                    <!--<fieldset class="site-name">-->
                    <legend>Change User Email Address</legend>

                    <div class="editor-block">
                        <div class="settings-title">
                            <label for="current_email_address" class="editor-title">Current Email Address:</label>
                        </div>
                        <div class="settings-value">
                            <xsl:copy-of select="page/body/users/current_email_address/node()"/>
                        </div>
                        <div class="clear-both"></div>
                    </div>

                    <div class="editor-block">
                        <div class="settings-title">
                            <label for="email_address" class="editor-title">New Email Address</label>
                        </div>
                        <div class="settings-value">
                            <xsl:copy-of select="page/body/users/email_address/node()"/>
                        </div>
                    </div>

                    <div class="editor-block">
                        <div class="settings-title">
                            <label for="repeat_email_address" class="editor-title">Repeat Email Address</label>
                        </div>
                        <div class="settings-value">
                            <xsl:copy-of select="page/body/users/repeat_email_address/node()"/>
                        </div>
                    </div>

                    <!--</fieldset>-->

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
