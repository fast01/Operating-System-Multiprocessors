#include <aio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "scheduler.h"

ssize_t read_wrap(int fd, void* buf, size_t count)
{
	if ((int)count < 0)//return -1 to behave like read
	{
		errno = EFAULT;
		return -1;
	}
	
	int result_aio_error;
	int seekable = 1; //0: not seekable, 1: seekable
	int last_errorno = errno;//save the last errno (in case lseek fails)
	
	//init AIO control block 
	struct aiocb cb;
	memset(&cb, 0, sizeof(struct aiocb));
	cb.aio_fildes = fd;
	cb.aio_buf = buf;
	cb.aio_nbytes = count;
	cb.aio_offset = lseek(fd,0,SEEK_CUR);
	if (cb.aio_offset == -1)// if lseek fails
	{
		cb.aio_offset = 0;
		seekable = 0;
		errno = last_errorno;//set errono value back to the previous one to indicate that the read_wrap does not introduce error because of unseekable input
	}
	
	//initiate an appropriate asynchronous read 
	if (aio_read(&cb) == -1)//if request is not enqueued
		return -1;
	
	//yield until the request is complete
	result_aio_error = aio_error(&cb);
	while (result_aio_error == EINPROGRESS)
	{			
		yield();
		result_aio_error = aio_error(&cb);
	}
	
	//check status of AIO
	if (result_aio_error != 0)//not successful
	{
		return -1;
	}
	
	/*if AIO has completed*/
	if (seekable == 1)//if the input is seekable
	{
		//seek to the appropriate position in the file
		int end_of_file_offset_location = lseek(fd,0,SEEK_END);//seek to the end of the file
		if (end_of_file_offset_location == -1)// if lseek fails
			return -1;		
		if (cb.aio_offset + count < end_of_file_offset_location)//if end of file offset is greater than previous current offset + count -> count is the number of bytes read
		{
			if (lseek(fd,cb.aio_offset + count,SEEK_SET) == -1)//seek to previous current offset + count 
				return -1;
		}
	}
	//return result of AIO read 
	return aio_return(&cb);
}