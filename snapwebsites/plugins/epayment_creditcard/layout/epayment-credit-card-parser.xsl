<?xml version="1.0"?>
<!--
Snap Websites Server == epayment credit catd settings parser
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
	<xsl:variable name="layout-area">epayment-credit-card-parser</xsl:variable>
	<xsl:variable name="layout-modified">2016-01-29 00:39:07</xsl:variable>
	<xsl:variable name="layout-editor">epayment-credit-card-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div id="content" class="editor-form" form_name="info">
				<xsl:attribute name="session"><xsl:copy-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>

				<!-- xsl:if test="$action != 'edit' and $can_edit = 'yes'">
					<a class="settings-edit-button" href="?a=edit">Edit</a>
				</xsl:if>
				<xsl:if test="$action = 'edit'">
					<a class="settings-save-button" href="#">Save Changes</a>
					<a class="settings-cancel-button right-aligned" href="{/snap/head/metadata/desc[@type='page_uri']/data}">Cancel</a>
				</xsl:if-->
				<h3>Billing Information</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset class="site-name">
						<legend>Credit Card</legend>

						<div class="editor-block">
							<label for="card_number" class="editor-title">Card Number:</label>
							<xsl:copy-of select="page/body/epayment/card_number/node()"/>
						</div>

						<div class="editor-block">
							<label for="security_code" class="editor-title">Security Code:</label>
							<xsl:copy-of select="page/body/epayment/security_code/node()"/>
						</div>

						<div class="editor-block">
							<label for="expiration_year" class="editor-title">Expiration Year:</label>
							<xsl:copy-of select="page/body/epayment/expiration_year/node()"/>
						</div>

						<div class="editor-block">
							<label for="expiration_month" class="editor-title">Expiration Month:</label>
							<xsl:copy-of select="page/body/epayment/expiration_month/node()"/>
						</div>
					</fieldset>

					<fieldset class="breadcrumbs">
						<legend>User Information</legend>

						<div class="editor-block">
							<label for="user_name" class="editor-title">First and Last Name:</label>
							<xsl:copy-of select="page/body/epayment/user_name/node()"/>
						</div>

						<div class="editor-block">
							<label for="address1" class="editor-title">Address:</label>
							<xsl:copy-of select="page/body/epayment/address1/node()"/>
							<xsl:copy-of select="page/body/epayment/address2/node()"/>
						</div>

						<div class="editor-block">
							<label for="city" class="editor-title">City:</label>
							<xsl:copy-of select="page/body/epayment/city/node()"/>
						</div>

						<div class="editor-block">
							<label for="province" class="editor-title">Province / State:</label>
							<xsl:copy-of select="page/body/epayment/province/node()"/>
						</div>

						<div class="editor-block">
							<label for="postal_code" class="editor-title">Postal Code:</label>
							<xsl:copy-of select="page/body/epayment/postal_code/node()"/>
						</div>

						<div class="editor-block">
							<label for="country" class="editor-title">Country:</label>
							<xsl:copy-of select="page/body/epayment/country/node()"/>
						</div>
					</fieldset>

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
