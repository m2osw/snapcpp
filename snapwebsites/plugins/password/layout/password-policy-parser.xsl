<?xml version="1.0"?>
<!--
Snap Websites Server == password policy settings parser
Copyright (C) 2014-2015  Made to Order Software Corp.

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
	<xsl:variable name="layout-area">password-policy-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-12-20 23:22:10</xsl:variable>
	<xsl:variable name="layout-editor">password-policy-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div id="content" class="editor-form" form_name="password">
				<xsl:attribute name="session"><xsl:copy-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>

				<h3>Password Policy</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset class="site-name">
						<legend>Minimum Counts</legend>

						<!-- minimum lowercase letters widget -->
						<div class="editor-block">
							<label for="minimum_lowercase_letters" class="editor-title">Minimum Lowercase Letters:</label>
							<xsl:copy-of select="page/body/password/minimum_lowercase_letters/node()"/>
						</div>

						<!-- minimum uppercase letters widget -->
						<div class="editor-block">
							<label for="minimum_uppercase_letters" class="editor-title">Minimum Uppercase Letters:</label>
							<xsl:copy-of select="page/body/password/minimum_uppercase_letters/node()"/>
						</div>

						<!-- minimum letters widget -->
						<div class="editor-block">
							<label for="minimum_letters" class="editor-title">Minimum Letters:</label>
							<xsl:copy-of select="page/body/password/minimum_letters/node()"/>
						</div>

						<!-- minimum digits widget -->
						<div class="editor-block">
							<label for="minimum_digits" class="editor-title">Minimum Digits:</label>
							<xsl:copy-of select="page/body/password/minimum_digits/node()"/>
						</div>

						<!-- minimum spaces widget -->
						<div class="editor-block">
							<label for="minimum_spaces" class="editor-title">Minimum Spaces:</label>
							<xsl:copy-of select="page/body/password/minimum_spaces/node()"/>
						</div>

						<!-- minimum special characters widget -->
						<div class="editor-block">
							<label for="minimum_specials" class="editor-title">Minimum Special Characters:</label>
							<xsl:copy-of select="page/body/password/minimum_specials/node()"/>
						</div>

						<!-- minimum unicode widget -->
						<div class="editor-block">
							<label for="minimum_unicode" class="editor-title">Minimum Unicode Characters:</label>
							<xsl:copy-of select="page/body/password/minimum_unicode/node()"/>
						</div>

					</fieldset>

					<fieldset class="blaclist">
						<legend>Blacklist</legend>

						<!-- check password against blacklist widget -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/password/check_blacklist/node()"/>
						</div>

					</fieldset>

					<fieldset class="check-passwords">
						<legend>Check Passwords</legend>

						<div class="editor-block">
							<p>
								TODO: add a widget so one can check a sample password
								against this policy.
							</p>
						</div>
					</fieldset>

					<!--div class="buttons">
						Add support for a delete in the parent instead?
						<a class="editor-save-button" href="#">Save Changes</a>
						<a class="editor-cancel-button center-aligned" href="{/snap/head/metadata/desc[@type='page_uri']/data}">Reset</a>
						<a class="editor-delete-button right-aligned" href="#">Delete</a>
					</div-->
				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
