#pragma once

extern "C"
{

time_t dos2unixtime( unsigned long dostime );

unsigned long dostime(
	int y, /* year   */
	int n, /* month  */
	int d, /* day    */
	int h, /* hour   */
	int m, /* minute */
	int s  /* second */
);

unsigned long unix2dostime( time_t *t );

}

