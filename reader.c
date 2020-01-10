/* Author: Robbert van Renesse 2018
 *
 * The interface is as follows:
 *	reader_t reader_create(int fd);
 *		Create a reader that reads characters from the given file descriptor.
 *
 *	char reader_next(reader_t reader):
 *		Return the next character or -1 upon EOF (or error...)
 *
 *	void reader_free(reader_t reader):
 *		Release any memory allocated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "shall.h"

struct reader {
	int fd;
	char buffer[512];
	int size;
	int point; 
};

reader_t reader_create(int fd){
	reader_t reader = (reader_t) calloc(1, sizeof(*reader));
	reader->fd = fd;
	reader->point = 0;
	return reader;
}

char reader_next(reader_t reader){
	if (reader->point==0){
		int n = read(reader->fd, &(reader->buffer), 512);
		reader->size=n;
		if(n==0){
			return EOF;
		}
	}
	char c=reader->buffer[reader->point];
	reader->point++;
	if(reader->point>=reader->size){
		reader->point=0;
	}
	return c;
}
	
	

void reader_free(reader_t reader){
	free(reader);
}
