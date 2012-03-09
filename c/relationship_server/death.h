#ifndef _DEATH___
#define _DEATH___

#include <stdarg.h>

void death( const char * tmplate, ... )
{
	va_list ap;
	va_start ( ap, tmplate );
	printf ( "FATAL: " );
	vprintf ( tmplate, ap );
	printf ( "\n" );
	va_end ( ap );
	exit ( 1 );
}

#endif
