#include <fcntl.h>

#include <DB/IO/ReadBufferFromFile.h>
#include <DB/IO/WriteHelpers.h>
#include <DB/Common/ProfileEvents.h>


namespace ProfileEvents
{
	extern const Event FileOpen;
}


namespace DB
{

namespace ErrorCodes
{
	extern const int FILE_DOESNT_EXIST;
	extern const int CANNOT_OPEN_FILE;
	extern const int CANNOT_CLOSE_FILE;
}


ReadBufferFromFile::ReadBufferFromFile(
	const std::string & file_name_,
	size_t buf_size,
	int flags,
	char * existing_memory,
	size_t alignment)
	: ReadBufferFromFileDescriptor(-1, buf_size, existing_memory, alignment), file_name(file_name_)
{
	ProfileEvents::increment(ProfileEvents::FileOpen);

#ifdef __APPLE__
	bool o_direct = (flags != -1) && (flags & O_DIRECT);
	if (o_direct) {
		flags = flags & ~O_DIRECT;
	}
#endif
	fd = open(file_name.c_str(), flags == -1 ? O_RDONLY : flags);

	if (-1 == fd)
		throwFromErrno("Cannot open file " + file_name, errno == ENOENT ? ErrorCodes::FILE_DOESNT_EXIST : ErrorCodes::CANNOT_OPEN_FILE);
#ifdef __APPLE__
	if (o_direct)
		if (fcntl(fd, F_NOCACHE, 1) == -1)
			throwFromErrno("Cannot set F_NOCACHE on file " + file_name, ErrorCodes::CANNOT_OPEN_FILE);
#endif
}


ReadBufferFromFile::ReadBufferFromFile(
	int fd,
	size_t buf_size,
	int flags,
	char * existing_memory,
	size_t alignment)
	: ReadBufferFromFileDescriptor(fd, buf_size, existing_memory, alignment), file_name("(fd = " + toString(fd) + ")")
{
}


ReadBufferFromFile::~ReadBufferFromFile()
{
	if (fd < 0)
		return;

	::close(fd);
}


void ReadBufferFromFile::close()
{
	if (0 != ::close(fd))
		throw Exception("Cannot close file", ErrorCodes::CANNOT_CLOSE_FILE);

	fd = -1;
	metric_increment.destroy();
}

}
