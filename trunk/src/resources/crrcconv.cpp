#include <stdio.h>
int main( int argc, const char ** argv )
{
	if ( argc!=4 ) {
		printf("usage: crrcconv INFILE OUTFILE VARNAME\n");
		return -1;
	}
	FILE * in = fopen( argv[1], "rb" );
	FILE * out = fopen( argv[2], "wt" );
	if ( !in || !out ) {
		printf("Cannot open file(s)\n");
		return -2;
	}
	unsigned char buf[16];
	fprintf( out, "static unsigned char %s[] = {\n", argv[3] );
	for (;;) {
		int bytesRead = fread( buf, 1, 16, in );
		if ( bytesRead<=0 )
			break;
		for ( int i=0; i<bytesRead; i++ )
			fprintf( out, "0x%02x,", buf[i] );
		fprintf( out, "\n" );
	}
	fprintf( out, "};\n" );
	fclose(in);
	fclose(out);
}
