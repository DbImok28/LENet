#pragma once
#include <queue>
#include <list>
#include <cstddef>
#include <iostream>
#include <sstream>

namespace LimeEngine::Net
{
    template<size_t BufferSize>
    class BufferPool {
    public:
        explicit BufferPool(size_t bufferCount = 8)
        {
            for (int i = 0; i < bufferCount; ++i)
            {
                ReturnBuffer(buffers.emplace_back().data());
            }
        }

        char *TakeBuffer()
        {
            if (buffersQueue.empty())
            {
                std::cout << "[TakeBuffer +" << buffersQueue.size() << "/" << buffers.size() + 1 << "]" << std::endl;
                return buffers.emplace_back().data();
            }

            std::cout << "[TakeBuffer " << buffersQueue.size() - 1 << "/" << buffers.size() << "]" << std::endl;
            auto buf = buffersQueue.front();
            buffersQueue.pop();
            return buf;
        }

        void ReturnBuffer(char *buf)
        {
            std::cout << "[ReturnBuffer " << buffersQueue.size() + 1 << "/" << buffers.size() << "]" << std::endl;
            buffersQueue.push(buf);
        }

    private:
        std::queue<char *> buffersQueue;
        std::list<std::array<char, BufferSize>> buffers;

    public:
        static constexpr size_t size = BufferSize;
    };

    template<size_t BufferSize>
    class BufferChain
    {
    public:
        explicit BufferChain(BufferPool<BufferSize>& bufferPool) : bufferPool(bufferPool) {}
        ~BufferChain()
        {
            for (auto &buffer: buffers)
            {
                bufferPool.ReturnBuffer(buffer);
            }
        }

        char *TakeBuffer()
        {
            char* buffer = bufferPool.TakeBuffer();
            buffers.push_back(buffer);
            return buffer;
        }

        void ReturnAllBuffers()
        {
            for (auto &buffer: buffers)
            {
                bufferPool.ReturnBuffer(buffer);
            }
            buffers.clear();
        }

        std::string ConcatBuffers()
        {
            std::ostringstream oss;
            for (auto &buffer : buffers)
            {
                oss << buffer;
            }
            return oss.str();
        }

        auto begin() noexcept { return buffers.begin(); }
        auto end() noexcept { return buffers.end(); }
        auto begin() const noexcept { return buffers.begin(); }
        auto end() const noexcept { return buffers.end(); }
        auto cbegin() const noexcept { return buffers.cbegin(); }
        auto cend() const noexcept { return buffers.cend(); }
        auto size() const noexcept { return buffers.size(); }

    private:
        BufferPool<BufferSize>& bufferPool;
        std::list<char*> buffers;
    };
}
