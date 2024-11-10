#include "NetBase.hpp"
#include "NetContext.hpp"

namespace LimeEngine::Net
{
    std::string GetWSAErrorMessage(int err)
    {
        char *s = nullptr;
        FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&s), 0, nullptr);
        std::string msg = s;
        LocalFree(s);
        return msg;
    }

    bool NetTCPDataHandler::Receive(NetSocket &socket, IOContext &context, int &outBytesTransferred)
    {
        if (socket.Receive(context.netBuffer.buf, context.netBuffer.len, outBytesTransferred))
        {
            NetLogger::LogCore("Receive {}b: {}", outBytesTransferred, context.netBuffer.buf);
            return true;
        }
        return false;
    }

    bool NetTCPDataHandler::Send(NetSocket &socket, IOContext &context, int &outBytesTransferred)
    {
        if (socket.Send(context.netBuffer.buf, context.netBuffer.len, outBytesTransferred))
        {
            NetLogger::LogCore("Send {}b{}: {}", outBytesTransferred,
                               ((outBytesTransferred != (context.netBuffer.len + 1ull)) ? " part" : ""),
                               context.netBuffer.buf);
            return true;
        }
        return false;
    }

    bool NetTCPDataHandler::ReceiveAsync(NetSocket &socket, IOContext &context)
    {
        if (socket.ReceiveAsync(&context.netBuffer, &context.nativeIoContext))
        {
            std::cout << "Async Receive started" << std::endl;
            return true;
        }
        return false;
    }

    bool NetTCPDataHandler::SendAsync(NetSocket &socket, IOContext &context)
    {
        if (socket.SendAsync(&context.netBuffer, &context.nativeIoContext))
        {
            std::cout << "Async Send started" << std::endl;
            return true;
        }
        return false;
    }
}
