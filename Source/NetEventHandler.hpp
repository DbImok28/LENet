// Copyright (C) Pavel Jakushik - All rights reserved
// See the LICENSE file for copyright and licensing details.

#pragma once
#include "NetContext.hpp"
#include "BufferPool.hpp"

namespace LimeEngine::Net
{
	class NetEventHandler
	{
	public:
		void StartRead(SocketContext& socketContext);
		void Read(SocketContext& socketContext, uint32_t bytesTransferred);

		bool StartWrite(SocketContext& socketContext);
		bool Write(SocketContext& socketContext, uint32_t bytesTransferred);

		bool ReadyToWrite(SocketContext& socketContext);
		bool Disconnect(SocketContext& socketContext);

	private:
		BufferPool<1024> bufferPool;
	};
}
