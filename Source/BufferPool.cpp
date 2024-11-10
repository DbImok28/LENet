#include "BufferPool.hpp"

std::string LimeEngine::Net::ConcatBuffers(const std::list<char*>& buffers, size_t bufferSize)
{
	size_t totalSize = (buffers.size() - 1) * bufferSize + strlen(buffers.back());
	std::string result(totalSize, '\0');

	char* dst = const_cast<char*>(result.data());
	auto it = buffers.begin();
	while (std::next(it) != buffers.end())
	{
		memcpy(dst, *it, bufferSize);
		dst += bufferSize;
		++it;
	}
	memcpy(dst, buffers.back(), strlen(buffers.back()));

	return result;
}
