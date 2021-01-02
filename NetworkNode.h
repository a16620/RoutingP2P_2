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
	using connection_info = std::pair<std::shared_ptr<Socket>, size_t>;
	std::shared_ptr<Socket> listener;

	SequentArrayList<WSAEVENT, max_connection + 1> events;
	std::unordered_map<WSAEVENT, connection_info> nodes;
	SequentArrayList<size_t, max_connection> buffer_pool;

	Router router;
	std::mutex lock_router;
	concurrency::concurrent_queue<Packet::PacketFrame*> queue_relay;

	void Evt_Connected(SOCKET s);
	void Evt_Close(WSAEVENT evt, connection_info& info);
	void Evt_DataLoaded(Packet::DataPacket* dataPacket);
	void Evt_Cmd();

	bool Connect(ULONG addr, u_short port);
	void HandlePacket(Packet::PacketFrame* packet, std::shared_ptr<Socket>& ctx);
	void RelayPacket(Packet::PacketFrame* packet);
	void GenerateSignal();

	bool is_running;
	std::thread recv_thread, relay_thread;
	

public:
	NetworkNode() = delete;
	NetworkNode(Address);
	void Run(u_short port);
	void Stop();

	void recv_proc();
	void relay_proc();
};

void SendWithLength(std::shared_ptr<Socket>& s, const char* buffer, size_t length);