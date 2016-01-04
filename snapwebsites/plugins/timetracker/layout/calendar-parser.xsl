<?xml version="1.0"?>
<!--
Snap Websites Server == timetracker calendar parser
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
	<xsl:variable name="calendar">calendar-parser</xsl:variable>
	<xsl:variable name="calendar-modified">2016-01-02 18:07:08</xsl:variable>

	<xsl:template match="no-day">
		<!-- entry that does not include a day for that month -->
		<td class="no-day">&#160;</td>
	</xsl:template>

	<xsl:template match="day">
		<td class="day">
			<xsl:attribute name="class">day<xsl:if test="@today"> today</xsl:if></xsl:attribute>
			<xsl:value-of select="."/>
		</td>
	</xsl:template>

	<xsl:template match="line">
		<tr class="days-line">
			<td class="week-column">
				<xsl:attribute name="class">week-column<xsl:if test="*/@today"> this-week</xsl:if></xsl:attribute>
				<xsl:value-of select="@week"/>
			</td>
			<xsl:apply-templates select="*"/>
		</tr>
	</xsl:template>

	<xsl:template match="snap">
		<output> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div id="content">

				<h3>Time Tracking Calendar</h3>
				<div class="calendar">

					<table>
						<theader>
							<th>
								<td class="buttons"><a href="#previous-year" title="Previous Year">&lt;&lt;</a></td>
								<td class="buttons"><a href="#previous-month" title="Previous Month">&lt;</a></td>
								<td class="month" colspan="4" data-month="{month/@mm}" data-year="{year}"><xsl:copy-of select="month"/> <xsl:copy-of select="year"/></td>
								<td class="buttons"><a href="#next-month" title="Next Month">&gt;</a></td>
								<td class="buttons"><a href="#next-year" title="Next Year">&gt;&gt;</a></td>
							</th>
						</theader>

						<tbody class="days">
							<tr class="day-columns">
								<td class="week-column">Wk#</td>
								<td class="day-column">Sun</td>
								<td class="day-column">Mon</td>
								<td class="day-column">Tue</td>
								<td class="day-column">Wed</td>
								<td class="day-column">Thu</td>
								<td class="day-column">Fri</td>
								<td class="day-column">Sat</td>
							</tr>
							<xsl:apply-templates select="days/*"/>
						</tbody>
					</table>

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
