#include "NetContext.hpp"

namespace LimeEngine::Net
{
	IOContext::IOContext(IOOperationType operationType) noexcept : operationType(operationType), netBuffer(0, nullptr)
	{
		nativeIoContext.Internal = 0;
		nativeIoContext.InternalHigh = 0;
		nativeIoContext.Offset = 0;
		nativeIoContext.OffsetHigh = 0;
		nativeIoContext.hEvent = nullptr;
	}

	void IOContext::SetNextBuffer(const char* buffer)
	{
		SetNextBuffer(const_cast<char*>(buffer));
	}

	void IOContext::SetNextBuffer(char* buffer)
	{
		netBuffer.buf = buffer;
		buffers.emplace_back(buffer);
	}

	void IOContext::SetMessageLength(uint32_t messageLen)
	{
		netBuffer.len = messageLen;
	}

	void IOContext::Reset()
	{
		buffers.clear();
	}

	std::pair<char*, uint32_t> IOContext::GetBuffer() const
	{
		return std::make_pair(netBuffer.buf, netBuffer.len);
	}

	const std::list<char*>& IOContext::GetBuffers() const
	{
		return buffers;
	}

	IOContext* IOContext::FromNativeIoContext(NativeIOContext* nativeIoContext) noexcept
	{
		return CONTAINING_RECORD(nativeIoContext, IOContext, nativeIoContext);
	}
}
