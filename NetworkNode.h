#pragma once
#include "Socket.h"
#include "Buffer.h"
#include "Packet.h"
#include "Router.h"

#include "StaticAlloc.h"
#include <set>
#include <thread>
#include <array>
#include <concurrent_queue.h>
#include <unordered_map>
#include <memory>
#include <mutex>

constexpr size_t max_connection = 3;
constexpr size_t temporary_buffer_size = 2048;

class NetworkNode
{
private:
	Socket listener;
	Router router;

	void HandlePacket(Packet::PacketFrame* packet, std::shared_ptr<Socket>& ctx);
	void RelayPacket(Packet::PacketFrame* packet);
	void GenerateSignal();

	bool is_running;
	std::thread recv_thread, relay_thread;
	std::mutex lock_router;
	concurrency::concurrent_queue<Packet::PacketFrame*> queue_relay;

public:
	NetworkNode();
	void Run(u_short port);
	void Stop();

	void recv_proc();
	void relay_proc();
};

