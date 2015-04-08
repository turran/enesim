<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <xsl:output method="xml" version="1.0" indent="no" standalone="yes" />

<xsl:variable name="doc-to-merge" select="document($tomerge)"/>
<xsl:key name="name" match="callback" use="@name"/>

<xsl:template match="@* | node()" name="identity">
  <xsl:copy>
    <xsl:apply-templates select="@* | node()"/>
  </xsl:copy>
</xsl:template>

<xsl:template match="callback">
  <xsl:variable name="this" select="."/>
  <xsl:variable name="name" select="$this/@name"/>
  <xsl:choose>
    <xsl:when test="$doc-to-merge//callback[@name = $name]">
      <xsl:copy-of select="$doc-to-merge//callback[@name = $name]"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$this"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
