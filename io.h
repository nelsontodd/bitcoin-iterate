/*******************************************************************************
 *
 *  = io.h
 *
 *  Defines functions for handling generic files on disk.
 *
 *  All these functions exit on errors.
 */

#ifndef BITCOIN_PARSE_IO_H
#define BITCOIN_PARSE_IO_H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * file - Struct used to wrap files.
 *
 * @name: path (used in error reporting)
 * @fd: file descriptor
 * @len: size
 * @mmap: mmap pointer for file
 *
 */
struct file {
	const char *name;
	int fd;
	off_t len;
	void *mmap;
};

/* O_NOCTTY doesn't make sense for normal files, so overload it */
#define O_NO_MMAP O_NOCTTY

/**
 * file_read - Read from (possibly memory-mapped) file.
 *
 * @f: current file
 * @off: current offset
 * @size: bytes to read
 * @buf: buffer to read data into
 *
 * If file is mmap'd, just return the appropriate pointer.  Else,
 * return pointer to a buffer with the file's contents.
 */
void *file_read(struct file *f, off_t off, size_t size, void *buf);

/**
 * file_write - Write to (possibly memory-mapped) file.
 *
 * @f: current file
 * @off: current offset
 * @size: bytes to write
 * @buf: buffer to read data from
 *
 * If file is mmap'd, just check that there's enough room for the
 * write to succeed.  Else, perform the write.
 */
void file_write(struct file *f, off_t off, size_t size, const void *buf);

/**
 * file_append - Append to (possibly memory-mapped) file.
 *
 * @f: current file
 * @buf: buffer to read data from 
 * @size: bytes to write
 *
 * Assumes we're the only ones modifying size!
 */
void file_append(struct file *f, const void *buf, size_t size);

/**
 * file_open - Open (possibly memory-mapped) file.
 *
 * @f: current file
 * @name: path to file
 * @len: truncate file to this size (if non-empty)
 * @oflags: flags for opening file
 *
 * Uses permissions 0600 if `O_CREAT`.
 */
void file_open(struct file *f, const char *name, off_t len, int oflags);

/**
 * file_close - Close (possibly memory-mapped) file.
 *
 * @f: current file
 * 
 */
void file_close(struct file *f);
#endif /* BITCOIN_PARSE_IO_H */
