<?php
# Display this directory to users
# (code used to display test results)

$dir = glob("libtld*");

echo "<html><head><title>libtld coverage</title></head>";
echo "<body><h1>libtld coverage</h1><table border=\"1\" cellpadding=\"10\" cellspacing=\"0\"><tbody><tr><th>Coverage</th></tr>";
foreach($dir as $d)
{
	echo "<tr>";

	echo "<td><a href=\"", $d, "\">", $d, "</a></td>";

        echo "</tr>";
}
echo "</tbody></table></body></html>";
