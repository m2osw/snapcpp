<?xml version="1.0"?>
<!--
Snap Websites Server == snapmanager.cgi parser
Copyright (C) 2016-2017  Made to Order Software Corp.

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

	<xsl:template match="manager">
		<html>
			<head>
				<meta charset="utf-8"/>
				<title>Snap! Manager</title>
				<meta name="generator" content="Snap! Manager CGI"/>
                <script type="text/javascript" src="https://code.jquery.com/jquery-1.12.4.js"></script>
                <script type="text/javascript" src="https://code.jquery.com/ui/1.12.1/jquery-ui.js"></script>
                <script type="text/javascript">
                    $(document).ready(function() {
                        $("#accordion").accordion();
                    });
                </script>
                <link href="/favicon.ico" type="image/x-icon" rel="shortcut icon"/>
				<style>
					body
					{
						font-family: sans;
					}
					div.access-warning
					{
						width: 80%;
						margin: 20px auto;
						border: 1px solid red;
						background: #ffe0e0;
						padding: 10px;
						padding-bottom: 0;
					}
					div.access-warning div.access-title
					{
						text-align: center;
						font-weight: bold;
						font-size: 120%;
					}
					table
					{
						border-top: 1px solid black;
						border-left: 1px solid black;
						border-spacing: 0;
						border-collapse: collapse;
					}
					table th
					{
						background-color: #e0e0e0;
					}
					table th,
					table td
					{
						border-bottom: 1px solid black;
						border-right: 1px solid black;
						padding: 3px;
						vertical-align: top;
					}
					table .errors td
					{
						border-bottom: 1px solid red;
						border-right: 1px solid red;
					}
					table tr.modified
					{
						background-color: #d6b2ff;
					}
					table tr.highlight
					{
						background-color: #fff881;
					}
					table tr.warnings
					{
						background-color: #ffde9f;
					}
					table tr.down
					{
						background-color: #ffdede;
						text-decoration: line-through;
					}
					table tr.errors
					{
						background-color: #ffd8d8;
						border-left: 1px solid red;
					}
					label
					{
						font-weight: bold;
						display: block;
					}
					input[type="input"]
					{
						width: calc(100% - 20px);
					}
					button
					{
						margin: 5px 10px;
					}
				</style>
			</head>
			<body>
				<div>
					<ul class="menu">
						<li class="menu-item"><a href="/snapmanager">Home</a></li>
						<!-- more entries... -->
						<xsl:for-each select="menu/item">
							<li class="menu-item"><a href="{@href}"><xsl:copy-of select="."/></a></li>
						</xsl:for-each>
						<li class="menu-item"><a href="/snapmanager?logout">Log Out</a></li>
					</ul>
				</div>
				<h1>Snap! Manager</h1>
                <div id="accordion">
					<xsl:copy-of select="output/node()"/>
				</div>
			</body>
		</html>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
