#include "NetEventHandler.hpp"

namespace LimeEngine::Net
{
	void NetEventHandler::StartRead(SocketContext& socketContext)
	{
		socketContext.receiveContext.SetMessageLength(bufferPool.size);
		socketContext.receiveContext.SetNextBuffer(bufferPool.TakeBuffer());
	}

	void NetEventHandler::Read(SocketContext& socketContext, uint32_t bytesTransferred)
	{
		IOContext& ioContext = socketContext.receiveContext;
		auto [buffer, size] = ioContext.GetBuffer();

		NetLogger::LogCore("Read: {}", buffer);

		if (buffer[bytesTransferred - 1] == '\0')
		{
			NetReceivedMessage fullMsg = NetReceivedMessage(ConcatBuffers(ioContext.GetBuffers(), bufferPool.size));
			socketContext.connection->receivedMessages.emplace(std::move(fullMsg));

			NetLogger::LogCore("[msg end]");

			bufferPool.ReturnBuffers(ioContext.GetBuffers());
			ioContext.Reset();
		}
		ioContext.SetNextBuffer(bufferPool.TakeBuffer());
	}

	bool NetEventHandler::StartWrite(SocketContext& socketContext)
	{
		if (!socketContext.connection->messagesToSend.empty())
		{
			auto& sendMsg = socketContext.connection->messagesToSend.back();
			if (sendMsg.sended) return false;

			socketContext.sendContext.SetMessageLength(sendMsg.msg.length() + 1);
			socketContext.sendContext.SetNextBuffer(sendMsg.msg.c_str());
			sendMsg.sended = true;
			return true;
		}
		return false;
	}

	bool NetEventHandler::Write(SocketContext& socketContext, uint32_t bytesTransferred)
	{
		socketContext.connection->messagesToSend.pop();
		socketContext.sendContext.Reset();
		return StartWrite(socketContext);
	}

	bool NetEventHandler::ReadyToWrite(SocketContext& socketContext)
	{
		NetLogger::LogCore("ReadyToWrite {}", !socketContext.connection->messagesToSend.empty());
		return !socketContext.connection->messagesToSend.empty();
	}

	bool NetEventHandler::Disconnect(SocketContext& socketContext)
	{
		socketContext.connection->ChangeStateToClose();
		return true;
	}
}