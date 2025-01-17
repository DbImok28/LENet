// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#pragma once
#include <queue>
#include <list>
#include <cstddef>
#include <sstream>
#include "NetLogger.hpp"

namespace LimeEngine::Net
{
	std::string ConcatBuffers(const std::list<char*>& buffers, size_t bufferSize);

	template <typename Iterator>
	std::string ConcatBuffers(Iterator begin, Iterator end, size_t bufferSize)
	{
		size_t totalSize = 0;
		size_t lastBufferSize;
		{
			auto it = begin;
			while (std::next(it) != end)
			{
				totalSize += bufferSize;
				++it;
			}
			lastBufferSize = strlen(*it);
			totalSize += lastBufferSize;
		}
		std::string result(totalSize, '\0');

		char* dst = const_cast<char*>(result.data());
		auto it = begin;
		while (std::next(it) != end)
		{
			memcpy(dst, *it, bufferSize);
			dst += bufferSize;
			++it;
		}
		memcpy(dst, *it, lastBufferSize);

		return result;
	}

	template <size_t BufferSize>
	class BufferPool
	{
	public:
		explicit BufferPool(size_t bufferCount = 8)
		{
			NetLogger::LogCore("[InitBuffers {}]", bufferCount);
			for (int i = 0; i < bufferCount; ++i)
			{
				buffersQueue.push(buffers.emplace_back().data());
			}
		}

		char* TakeBuffer()
		{
			if (buffersQueue.empty())
			{
				NetLogger::LogCore("[TakeBuffer +{}/{}]", buffersQueue.size(), buffers.size() + 1);
				return buffers.emplace_back().data();
			}

			NetLogger::LogCore("[TakeBuffer {}/{}]", buffersQueue.size() - 1, buffers.size());
			auto buf = buffersQueue.front();
			buffersQueue.pop();
			return buf;
		}

		void ReturnBuffer(char* returnBuffer)
		{
			NetLogger::LogCore("[ReturnBuffer {}/{}]", buffersQueue.size() + 1, buffers.size());
			buffersQueue.push(returnBuffer);
		}

		void ReturnBuffers(const std::list<char*>& returnBuffers)
		{
			NetLogger::LogCore("[ReturnBuffer {}/{}]", buffersQueue.size() + buffers.size(), buffers.size());
			for (const auto& returnBuffer : returnBuffers)
			{
				buffersQueue.push(returnBuffer);
			}
		}

	private:
		std::queue<char*> buffersQueue;
		std::list<std::array<char, BufferSize>> buffers;

	public:
		static constexpr size_t size = BufferSize;
	};

	template <size_t BufferSize>
	class BufferChain
	{
	public:
		explicit BufferChain(BufferPool<BufferSize>& bufferPool) : bufferPool(bufferPool) {}
		~BufferChain()
		{
			for (auto& buffer : buffers)
			{
				bufferPool.ReturnBuffer(buffer);
			}
		}

		char* TakeBuffer()
		{
			char* buffer = bufferPool.TakeBuffer();
			buffers.emplace_back(buffer);
			return buffer;
		}

		void ReturnAllBuffers()
		{
			for (auto& buffer : buffers)
			{
				bufferPool.ReturnBuffer(buffer);
			}
			buffers.clear();
		}

		std::string Concat()
		{
			return ConcatBuffers(buffers, bufferPool.size);
		}

		auto begin() noexcept
		{
			return buffers.begin();
		}
		auto end() noexcept
		{
			return buffers.end();
		}
		auto begin() const noexcept
		{
			return buffers.begin();
		}
		auto end() const noexcept
		{
			return buffers.end();
		}
		auto cbegin() const noexcept
		{
			return buffers.cbegin();
		}
		auto cend() const noexcept
		{
			return buffers.cend();
		}
		auto size() const noexcept
		{
			return buffers.size();
		}

	private:
		BufferPool<BufferSize>& bufferPool;
		std::list<char*> buffers;
	};
}
