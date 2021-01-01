#include "NetworkNode.h"

NetworkNode::NetworkNode() : listener(INVALID_SOCKET), is_running(false), router(addr_broadcast)
{
}

void NetworkNode::Run(u_short port)
{
	if (is_running)
		return;

	listener = Socket();
	Listen(listener, port);

	auto r_proc = [](NetworkNode* node) {
		node->recv_proc();
	};

	auto w_proc = [](NetworkNode* node) {
		node->relay_proc();
	};

	is_running = true;
	recv_thread = std::thread(r_proc, this);
	relay_thread = std::thread(w_proc, this);
}

void NetworkNode::Stop()
{
	is_running = false;

	if (recv_thread.joinable())
		recv_thread.join();
	if (relay_thread.joinable())
		relay_thread.join();
}

void NetworkNode::recv_proc()
{
	SequentArrayList<WSAEVENT, max_connection> events;
	std::unordered_map<WSAEVENT, std::pair<std::shared_ptr<Socket>, size_t>> nodes; //heap
	RecyclerBuffer<temporary_buffer_size> buffers[max_connection];
	SequentArrayList<size_t, max_connection> buffer_pool;
	std::array<size_t, max_connection> build_length;
	WSANETWORKEVENTS networkEvents;
	DWORD idx;

	while (is_running)
	{
		idx = WSAWaitForMultipleEvents(events.size(), events.getArray(), FALSE, 1000, FALSE);
		if (idx == WSA_WAIT_FAILED || idx == WSA_WAIT_TIMEOUT)
			continue;

		idx = idx - WSA_WAIT_EVENT_0;
		auto evt = events.at(idx);
		auto& st = nodes[evt].first;
		auto bidx = nodes[evt].second;

		if (WSAEnumNetworkEvents(st->get(), evt, &networkEvents) == SOCKET_ERROR)
			continue;

		if (networkEvents.lNetworkEvents & FD_ACCEPT) {
			sockaddr_in addr;
			int szAcp = sizeof(sockaddr_in);
			SOCKET s = accept(st->get(), reinterpret_cast<sockaddr*>(&addr), &szAcp);
			
			//If is not full
			WSAEVENT e = WSACreateEvent();
			WSAEventSelect(s, e, FD_READ | FD_CLOSE);
			events.push(e);
			auto bidx = buffer_pool.at(0);
			buffer_pool.pop(0);

			nodes.insert(std::make_pair(e, std::make_pair(std::make_shared<Socket>(s), bidx)));
		}

		if (networkEvents.lNetworkEvents & FD_READ) {
			auto& buffer = buffers[bidx];
			u_long hasCount;
			ioctlsocket(st->get(), FIONREAD, &hasCount);
			if (buffer.Length - buffer.Used() >= hasCount)
			{
				std::array<char, temporary_buffer_size> tmp_buffer;
				int readbytes = st->Recv(tmp_buffer.data(), hasCount);
				buffer.push(tmp_buffer.data(), readbytes);
			}
			else
			{
				//버퍼 오버플로
			}

			if (build_length[bidx] == 0 && buffer.Used() >= sizeof(long)) {
				std::array<char, 4> cBuffer;
				buffer.poll(cBuffer.data(), 4);
				build_length[bidx] = ntohl(*reinterpret_cast<u_long*>(cBuffer.data()));
			}

			if (build_length[bidx] != 0 && buffer.Used() >= (build_length[bidx] + 4)) {
				std::array<char, temporary_buffer_size> packetBuffer;
				buffer.poll(packetBuffer.data(), build_length[bidx]);
				build_length[bidx] = 0;

				auto packet = Packet::Decode(reinterpret_cast<Packet::PacketFrame*>(packetBuffer.data()));
				HandlePacket(packet, st);
			}
		}

		if (networkEvents.lNetworkEvents & FD_CLOSE) {
			auto bidx = nodes[evt].second;
			buffers[bidx].Clear();
			buffer_pool.push(bidx);

			{
				auto ptr = nodes[evt].first;
				std::lock_guard gd(lock_router);
				router.Remove(ptr);
			}
			nodes.erase(evt);
		}
	}
}

void NetworkNode::HandlePacket(Packet::PacketFrame* packet, std::shared_ptr<Socket>& ctx)
{
	if (packet->SubType >= Packet::PRT_TRANSMITION)
	{
		auto rp = reinterpret_cast<Packet::TransmitionPacket*>(packet);
		if (rp->SubType == Packet::PRT_TRANSMITION_ICMP)
		{
			auto ip = reinterpret_cast<Packet::ICMPPacket*>(rp);
			if (ip->flag == Packet::ICMP_NAREACH && !router.isUsed(ip->from))
			{
				{
					std::lock_guard gd(lock_router);
					router.RemoveAddress(ip->from);
				}
				RelayPacket(Packet::Generate<Packet::ICMPPacket>(ip->from, addr_broadcast, Packet::ICMP_NAREACH));
			}
		}

		if (rp->to == router.GetMyAddress() || rp->to == addr_broadcast)
		{
			if (rp->SubType == Packet::PRT_TRANSMITION_ICMP)
			{
				auto ip = reinterpret_cast<Packet::ICMPPacket*>(rp);
				if (ip->flag == Packet::ICMP_INFORM_ADDR)
				{
					std::lock_guard gd(lock_router);
					router.Register(ip->from, ctx);
				}
			}
			else
			{
				auto dp = reinterpret_cast<Packet::DataPacket*>(rp);
				dp->dataLength();
			}
			DESTROY_PACKET(packet);
		}
		else
		{
			RelayPacket(reinterpret_cast<Packet::TransmitionPacket*>(packet));
		}
	}
	else
	{
		auto rp = reinterpret_cast<Packet::RoutingPacket*>(packet);
		lock_router.lock();
		router.Update(rp->next, rp->dest, rp->hop);
		lock_router.unlock();
		DESTROY_PACKET(packet);
	}
	
}

void NetworkNode::RelayPacket(Packet::PacketFrame* packet)
{
	queue_relay.push(packet);
}

void NetworkNode::GenerateSignal()
{
	using namespace Packet;
	auto pt_ = Generate<ICMPPacket>(router.GetMyAddress(), addr_broadcast, ICMP_INFORM_ADDR);
	RelayPacket(pt_);
}

void NetworkNode::relay_proc()
{
	using namespace Packet;
	PacketFrame* pt;

	while (is_running)
	{
		if (queue_relay.try_pop(pt))
		{
			TransmitionPacket* packet = static_cast<TransmitionPacket*>(pt);
			auto payload_length = packet->payload_length;
			if (packet->to == addr_broadcast || pt->SubType == PRT_NEIGHBOUR_ROUTING)
			{
				std::lock_guard gd(lock_router);
				auto [begin, end] = router.GetConstIterator();
				Encode(pt);
				for (auto it = begin; it != end; ++it)
				{
					try {
						auto ptr = it->second;
						if (!ptr.expired())
						{
							auto p = ptr.lock();
							p->SendCompletly(reinterpret_cast<char*>(packet), sizeof(PacketFrame) + payload_length);
						}
					}
					catch (...)
					{

					}
				}
				DESTROY_PACKET(packet);
				continue;
			}
			else
			{
				try {
					std::weak_ptr<Socket> ptr;
					{
						std::lock_guard gd(lock_router);
						ptr = router.Fetch(packet->to);
					}
					if (!ptr.expired())
					{
						auto p = ptr.lock();
						Encode(pt);
						p->SendCompletly(reinterpret_cast<char*>(packet), sizeof(PacketFrame) + payload_length);
						DESTROY_PACKET(packet);
					}
					else
					{
						RelayPacket(packet);
					}
				}
				catch (std::runtime_error e)
				{
					RelayPacket(packet);
				}
				catch (std::out_of_range e)
				{
					auto pt_ = Generate<ICMPPacket>(packet->to, packet->from, ICMP_NAREACH);
					RelayPacket(pt_);
					DESTROY_PACKET(packet);
				}
			}
		}
		else
		{
			Sleep(100);
		}
	}
}
