#include <fcntl.h>     // open
#include <sys/types.h> // read
#include <sys/uio.h>   // read
#include <unistd.h>    // read, close
#include <strings.h>   // bcopy
#include <string.h>    // strlen
#include <stdarg.h>    // format, ...
#include <stdlib.h>    // abs

using namespace std;

char decimal[100];

int recursive_itoa( int arg ) {
  int div = arg / 10;
  int mod = arg % 10;
  int index = 0;
  if ( div > 0 )
    index = recursive_itoa( div );
  decimal[index] = mod + '0';
  return ++index;
}



char *itoa( const int arg ) {
  bzero( decimal, 100 );
  int order = recursive_itoa( arg );
  char *new_decimal = new char[order + 1];
  bcopy( decimal, new_decimal, order + 1 );
  return new_decimal;
}



int printf( const void *format, ... ) {
  va_list list;
  va_start( list, format );

  char *msg = ( char * )format;
  char buf[1024];
  int nWritten = 0;

  int i = 0, j = 0, k = 0;
  while ( msg[i] != '\0') {
    if ( msg[i] == '%' && msg[i + 1] == 'd' ) {
      buf[j] = '\0';
      nWritten += write( 1, buf, j );
      j = 0;
      i += 2;

      int int_val = va_arg( list, int );
      char *dec = itoa( abs( int_val ) );
      if ( int_val < 0 )
	nWritten += write( 1, "-", 1 );
      nWritten += write( 1, dec, strlen( dec ) );
      delete dec;
    }
    else
      buf[j++] = msg[i++];
  }
  if ( j > 0 )
    nWritten += write( 1, buf, j );
  va_end( list );
}



int setvbuf( FILE *stream, char *buf, int mode, size_t size ) {
  if ( mode != _IONBF && mode != _IOLBF && mode != _IOFBF )
    return -1;
  stream->mode = mode;
  stream->pos = 0;

  if ( stream->buffer != (char *)0 && stream->bufown == true )
    delete stream->buffer;

  switch ( mode ) {
  case _IONBF:
    stream->buffer = (char *)0;
    stream->size = 0;
    stream->bufown = false;
    break;
  case _IOLBF:
  case _IOFBF:
    if ( buf != (char *)0 ) {
      stream->buffer = buf;
      stream->size   = size;
      stream->bufown = false;
    }
    else {
      stream->buffer = new char[BUFSIZ];
      stream->size = BUFSIZ;
      stream->bufown = true;
    }
    break;
  }

  return 0;
}

void setbuf( FILE *stream, char *buf ) {
  setvbuf( stream, buf, ( buf != (char *)0 ) ? _IOFBF : _IONBF , BUFSIZ );
}

FILE *fopen( const char *path, const char *mode ) {
  FILE *stream = new FILE( );
  setvbuf( stream, (char *)0, _IOFBF, BUFSIZ );

  // fopen( ) mode
  // r or rb           =  O_RDONLY
  // w or wb           =  O_WRONLY | O_CREAT | O_TRUNC
  // a or ab           =  O_WRONLY | O_CREAT | O_APPEND
  // r+ or rb+ or r+b  =  O_RDWR
  // w+ or wb+ or w+b  =  O_RDWR   | O_CREAT | O_TRUNC
  // a+ or ab+ or a+b  =  O_RDWR   | O_CREAT | O_APPEND

  switch( mode[0] ) {
  case 'r':
    if ( mode[1] == '\0' )            // r
      stream->flag = O_RDONLY;
    else if ( mode[1] == 'b' ) {
      if ( mode[2] == '\0' )          // rb
	stream->flag = O_RDONLY;
      else if ( mode[2] == '+' )      // rb+
	stream->flag = O_RDWR;
    }
    else if ( mode[1] == '+' )        // r+  r+b
      stream->flag = O_RDWR;
    break;
  case 'w':
    if ( mode[1] == '\0' )            // w
      stream->flag = O_WRONLY | O_CREAT | O_TRUNC;
    else if ( mode[1] == 'b' ) {
      if ( mode[2] == '\0' )          // wb
	stream->flag = O_WRONLY | O_CREAT | O_TRUNC;
      else if ( mode[2] == '+' )      // wb+
	stream->flag = O_RDWR | O_CREAT | O_TRUNC;
    }
    else if ( mode[1] == '+' )        // w+  w+b
      stream->flag = O_RDWR | O_CREAT | O_TRUNC;
    break;
  case 'a':
    if ( mode[1] == '\0' )            // a
      stream->flag = O_WRONLY | O_CREAT | O_APPEND;
    else if ( mode[1] == 'b' ) {
      if ( mode[2] == '\0' )          // ab
	stream->flag = O_WRONLY | O_CREAT | O_APPEND;
      else if ( mode[2] == '+' )      // ab+
	stream->flag = O_RDWR | O_CREAT | O_APPEND;
    }
    else if ( mode[1] == '+' )        // a+  a+b
      stream->flag = O_RDWR | O_CREAT | O_APPEND;
    break;
  }

  mode_t open_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

  if ( ( stream->fd = open( path, stream->flag, mode ) ) == -1 ) {
    delete stream;
    printf( "fopen failed\n" );
    stream = NULL;
  }

  return stream;
}


// The function fpurge() clears the buffers of the given stream.  For output streams this discards any unwritten
// output.  For input streams this discards any input read from the underlying object
// but not yet obtained via getc(3); this includes any text pushed back via ungetc(3).  See also fflush(3).
// Upon successful completion fpurge() returns 0.  On error, it returns -1 and sets errno appropriately.
int fpurge( FILE *stream ) {
	stream->pos = 0;
	stream->actual_size = 0;
	return 0;
}



// For output streams, fflush() forces a write of all user-space buffered data for the given output or update
// stream via the stream's underlying write function.  For input streams, fflush() dis-cards any buffered data
// that has been fetched from the underlying file, but has not been consumed by the application.  The open
// status of the stream is unaffected.
// If the stream argument is NULL, fflush() flushes all open output streams.
// For a nonlocking counterpart, see unlocked_stdio(3).
// Upon successful completion 0 is returned.  Otherwise, EOF is returned and errno is set to indicate the error.
int fflush( FILE *stream ) {
	write(stream->fd, stream->buffer, stream->actual_size);
	stream->pos = 0;
  	return 0;
}



// fread helper
void fillBuffer(FILE *stream) {
	// check if the buffer is either empty or been read all
	if(stream->actual_size == 0 || stream->actual_size - stream->pos == 0) {
		stream->actual_size = read(stream->fd, stream->buffer, stream->size);
		stream->pos = 0;
	}
    if ( stream->actual_size <= 0 )
    // reached EOF or caused an error                                                                                          
    stream->eof = true;
}



// The function fread() reads nmemb elements of data, each size bytes long, from the stream pointed to by
// stream, storing them at the location given by ptr.
// For nonlocking counterparts, see unlocked_stdio(3).
// On  success,  fread()  and  fwrite() return the number of items read or written.  This number equals the
// number of bytes transferred only when size is 1.  If an error occurs, or the end of the file is reached,
// the return value is a short item count (or zero).
// fread() does not distinguish between end-of-file and error, and callers must use feof(3) and ferror(3) to
// determine which occurred.
size_t fread( void *ptr, size_t size, size_t nmemb, FILE *stream ) {
  int original = size * nmemb;
  int remainder = original;
  while( !stream->eof && remainder > 0) {
	  fillBuffer(stream);
	  int nRead = (stream->actual_size - stream->pos >= remainder) ? remainder : stream->actual_size - stream->pos;
	  // copying from file buffer to the passed from driver buffer
	  bcopy((const void*)(stream->buffer), (void*)((char*)ptr + (original - remainder)), nRead);
	  stream->pos = stream->pos + nRead;
	  remainder = remainder - nRead;
	  return (original - remainder)/size;
  }
}



bool writeBuffer(FILE *stream) {
  // write to the file what the buffer has:
  int nWritten = write( stream->fd, stream->buffer, stream->pos );

  if ( nWritten == -1 ) // error occurred
    return false;
  else if ( nWritten == stream->pos )// all buffer data was written
    stream->pos = 0;
  else {                             // some buffer contents need to shift left
    stream->pos = stream->pos - nWritten;
    for ( int i = 0; i < stream->pos; i++ )
      stream->buffer[i] = stream->buffer[nWritten + i];
  }
  return true;
}



// The function fwrite() writes nmemb elements of data, each size bytes long, to the stream pointed to by
// stream, obtaining them from the location given by ptr.
size_t fwrite( const void *ptr, size_t size, size_t nmemb, FILE *stream ) {

}



// reads the next character from stream and returns it as an unsigned char cast to an int, or EOF on end of
// file or error.
// return the character read as an unsigned char cast to an int or EOF on end of file or error.
int fgetc( FILE *stream ) {
	fillBuffer( stream );          // fill buffer if it is empty                                                                   
	if ( stream->actual_size - stream->pos > 0 ) {
		int c = stream->buffer[stream->pos++]; // read 1 byte                                                                        
		return c;
	}
	// EOF or error                                                                                                                
	return EOF;
}



// writes the character c, cast to an unsigned char, to stream.
// return the character written as an unsigned char cast to an int or EOF on error.
int fputc( int c, FILE *stream ) {
	size_t cWritten = write( stream->fd, &c, 1);
  return (int)cWritten;
}



// reads in at most one less than size characters from stream and stores them into the buffer pointed to by s.
// Reading stops after an EOF or a newline.  If  a  newline  is  read,  it  is stored into the buffer.
// A terminating null byte ('\0') is stored after the last character in the buffer.
// return s on success, and NULL on error or when end of file occurs while no characters have been read.
char *fgets( char *str, int size, FILE *stream ) {
	int original = size - 1; // leave one by for '\0'
	int remainder = original;
	//printf( "starting gets...\n" );
	while ( !stream->eof && remainder > 0 ) {
		//printf( "filling buffer...\n" );
		fillBuffer(stream);
		//printf( "buffer filled...\n" );
	  if( stream->eof == true ) {
		  return NULL;
	  }
		int nRead = stream->actual_size - stream->pos;
		//printf( "nRead = \n", nRead );
		char c = '\0';
		int i = 0;
		for ( ; i < nRead && c != '\n'; i++ ) {
			c = str[original - remainder + i] = stream->buffer[i + stream->pos];
		}			
		stream->pos = stream->pos + i;
		remainder = remainder - i;
		if ( c == '\n' )
			break;		
	 }
	 str[original - remainder] = '\0';
}



// writes the string s to stream, without its terminating null byte ('\0').
// return a nonnegative number on success, or EOF on error.
int fputs( const char *str, FILE *stream ) {
	size_t cWritten = fwrite( str, 1, strlen(str), stream );
	return (int)cWritten;
}



int feof( FILE *stream ) {
  return stream->eof == true;
}



// fseek() function sets the file position indicator for the stream pointed to by stream.  The new position,
// measured in bytes, is obtained by adding offset bytes to the position specified by whence.
// If whence is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset is relative to the start of the file, the
// current position indicator, or end-of-file, respectively.  A successful  call to the fseek() function clears
// the end-of-file indicator for the stream and undoes any effects of the ungetc(3) function on the same stream.
// returns the current offset.  Otherwise, -1 is returned and errno is set to indicate the error.
int fseek( FILE *stream, long offset, int whence ) {
 if ( whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END )
    return -1;
  off_t newOffset = lseek(stream->fd, offset, whence);
  return (int)newOffset;
}



// flushes the stream pointed to by fp (writing any buffered output data using fflush(3)) and closes the
// underlying file descriptor.
// Upon  successful completion 0 is returned.  Otherwise, EOF is returned and the global variable errno is
// set to indicate the error.  In either case any further access (including another call to fclose()) to the
// stream results in undefined behavior.
int fclose( FILE *stream ) {
	fflush(stream);
	close(stream->fd);
	return 0;
}






