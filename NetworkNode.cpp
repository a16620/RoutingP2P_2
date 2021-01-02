#include "NetworkNode.h"
#include "IntervalTimer.h"

void SendTemporaryPacketBySocket(Packet::PacketFrame* packet, std::shared_ptr<Socket>& s)
{
	auto payload_length = packet->payload_length;
	s->SendCompletly(reinterpret_cast<char*>(Packet::Encode(packet)), sizeof(Packet::PacketFrame) + payload_length);
	DESTROY_PACKET(packet);
}

NetworkNode::NetworkNode(Address local) : listener(nullptr), is_running(false), router(local)
{
}

void NetworkNode::Run(u_short port)
{
	if (is_running)
		return;

	listener = std::make_shared<Socket>();
	Listen(listener, port);

	for (size_t i = 0; i < max_connection; i++)
		buffer_pool.push(i);

	{
		WSAEVENT e = WSACreateEvent();
		WSAEventSelect(listener->get(), e, FD_ACCEPT);
		events.push(e);
		nodes.insert(std::make_pair(e, make_pair(listener, max_connection)));
	}

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

	while (!queue_relay.empty())
	{
		Packet::PacketFrame* pk;
		if (queue_relay.try_pop(pk))
		{
			DESTROY_PACKET(pk);
		}
	}

	for (int i = 0; i < events.size(); ++i)
	{
		WSACloseEvent(events.at(i));
	}
}

void NetworkNode::recv_proc()
{
	RecyclerBuffer<temporary_buffer_size> buffers[max_connection];
	std::array<u_long, max_connection> build_length;
	WSANETWORKEVENTS networkEvents;
	DWORD idx;

	memset(build_length.data(), 0, build_length.size() * sizeof(u_long));

	IntervalTimer signalTimer(30), commandTimer(3);
	while (is_running)
	{
		if (commandTimer.IsPassed())
		{
			
		}
		if (signalTimer.IsPassed())
		{
			GenerateSignal();
			auto [it, end] = router.GetConstIteratorRoute();
			for (; it != end; ++it)
			{
				auto pt_ = Packet::Generate<Packet::RoutingPacket>(router.GetMyAddress(), it->first, it->second.first_hop+1);
				RelayPacket(pt_);
			}
			signalTimer.Reset();
		}
		idx = WSAWaitForMultipleEvents(events.size(), events.getArray(), FALSE, 1000, FALSE);
		if (idx == WSA_WAIT_FAILED || idx == WSA_WAIT_TIMEOUT)
			continue;

		idx = idx - WSA_WAIT_EVENT_0;
		auto evt = events.at(idx);
		auto& node_ = nodes[evt];
		auto& st = node_.first;
		auto bidx = node_.second;

		if (WSAEnumNetworkEvents(st->get(), evt, &networkEvents) == SOCKET_ERROR)
			continue;

		if (networkEvents.lNetworkEvents & FD_ACCEPT) {
			auto s = AcceptNoAddress(st);
			Evt_Connected(s);
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
				//�����÷� ����
				networkEvents.lNetworkEvents &= FD_CLOSE;
			}

			if (build_length[bidx] == 0 && buffer.Used() >= sizeof(u_long)) {
				std::array<char, 4> cBuffer;
				buffer.poll(cBuffer.data(), 4);
				build_length[bidx] = ntohl(*reinterpret_cast<u_long*>(cBuffer.data()));
			}

			if (build_length[bidx] != 0 && buffer.Used() >= build_length[bidx]) {
				std::array<char, temporary_buffer_size> packetBuffer;
				buffer.poll(packetBuffer.data(), build_length[bidx]);
				build_length[bidx] = 0;

				auto packet = Packet::Decode(reinterpret_cast<Packet::PacketFrame*>(packetBuffer.data()));
				HandlePacket(packet, st);
			}
		}

		if (networkEvents.lNetworkEvents & FD_CLOSE) {
			buffers[bidx].Clear();
			build_length[bidx] = 0;

			Evt_Close(evt, node_);
		}
	}
}

void NetworkNode::Evt_Connected(SOCKET s)
{
	if (nodes.size() <= max_connection)
	{
		WSAEVENT e = WSACreateEvent();
		WSAEventSelect(s, e, FD_READ | FD_CLOSE);
		events.push(e);
		auto bidx = buffer_pool.at(0);
		buffer_pool.pop(0);

		auto ss = std::make_shared<Socket>(s);
		nodes.insert(std::make_pair(e, std::make_pair(ss, bidx)));
		SendTemporaryPacketBySocket(Packet::Generate<Packet::ICMPPacket>(router.GetMyAddress(), addr_broadcast, Packet::ICMP_INFORM_ADDR), ss);
	}
	else
	{
		closesocket(s);
	}
}

void NetworkNode::Evt_Close(WSAEVENT evt, connection_info& info)
{
	buffer_pool.push(info.second);
	WSACloseEvent(evt);

	{
		std::lock_guard gd(lock_router);
		router.Remove(info.first);
	}
	nodes.erase(evt);
}

void NetworkNode::Evt_DataLoaded(Packet::DataPacket* dataPacket)
{
	printf("%d bytes msg: %s\n", dataPacket->dataLength(), dataPacket + sizeof(Packet::DataPacket));
}

bool NetworkNode::Connect(ULONG addr, u_short port)
{
	if (nodes.size() > max_connection)
		return;

	sockaddr_in tar;
	tar.sin_family = AF_INET;
	tar.sin_port = htons(port);
	tar.sin_addr.s_addr = addr;
	auto s = make_tcp_socket();
	int tried = 0;
	do {
		if (connect(s, reinterpret_cast<sockaddr*>(&tar), sizeof(sockaddr_in)) == 0)
			break;
		tried++;
	}
	while (tried < 3);

	if (tried == 3)
	{
		closesocket(s);
		return false;
	}

	Evt_Connected(s);
	return true;
}

void NetworkNode::HandlePacket(Packet::PacketFrame* packet, std::shared_ptr<Socket>& ctx)
{
	using namespace Packet;
	if (packet->SubType >= PRT_TRANSMITION)
	{
		auto rp = reinterpret_cast<TransmitionPacket*>(packet);
		if (rp->SubType == PRT_TRANSMITION_ICMP)
		{
			auto ip = reinterpret_cast<ICMPPacket*>(rp);
			if (ip->flag == ICMP_NAREACH && !router.isUsed(ip->from))
			{
				{
					std::lock_guard gd(lock_router);
					router.RemoveAddress(ip->from);
				}
				RelayPacket(Generate<ICMPPacket>(ip->from, addr_broadcast, ICMP_NAREACH));
			}
		}

		if (rp->to == router.GetMyAddress() || rp->to == addr_broadcast)
		{
			if (rp->SubType == PRT_TRANSMITION_ICMP)
			{
				auto ip = reinterpret_cast<ICMPPacket*>(rp);
				if (ip->flag == ICMP_INFORM_ADDR)
				{
					std::lock_guard gd(lock_router);
					router.Register(ip->from, ctx);
				}
				else if (ip->flag == ICMP_TRAFFIC_INFO)
				{

				}
			}
			else
			{
				auto dp = reinterpret_cast<DataPacket*>(rp);
				Evt_DataLoaded(dp);
			}
			DESTROY_PACKET(packet);
		}
		else
		{
			RelayPacket(reinterpret_cast<TransmitionPacket*>(packet));
		}
	}
	else
	{
		auto rp = reinterpret_cast<RoutingPacket*>(packet);
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
				lock_router.lock();
				auto [it, end] = router.GetConstIteratorBroadcast();
				Encode(pt);
				for (; it != end; ++it)
				{
					try {
						auto ptr = it->second;
						if (!ptr.expired())
						{
							auto p = ptr.lock();
							SendWithLength(p, reinterpret_cast<char*>(packet), sizeof(PacketFrame) + payload_length);
						}
					}
					catch (...)
					{

					}
				}
				lock_router.unlock();
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
						SendWithLength(p, reinterpret_cast<char*>(packet), sizeof(PacketFrame) + payload_length);
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
					if (packet->from != router.GetMyAddress() && packet->to != router.GetMyAddress())
					{
						auto pt_ = Generate<ICMPPacket>(packet->to, packet->from, ICMP_NAREACH);
						RelayPacket(pt_);
					}
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

void SendWithLength(std::shared_ptr<Socket>& s, const char* buffer, size_t length)
{
	u_long len_ = htonl(length);
	s->SendCompletly(reinterpret_cast<char*>(&len_), sizeof(u_long));
	s->SendCompletly(buffer, length);
}
