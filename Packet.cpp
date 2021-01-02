#include <WinSock2.h>
#include "Packet.h"

Packet::PacketFrame* Packet::Encode(PacketFrame* packet)
{
	if (packet->SubType >= PRT_TRANSMITION)
	{
		auto ptr = reinterpret_cast<Packet::TransmitionPacket*>(packet);
		if (packet->SubType == PRT_TRANSMITION_ICMP)
		{
			auto ptr2 = reinterpret_cast<Packet::ICMPPacket*>(packet);
			ptr2->flag = htonl(ptr2->flag);
		}
	}
	else
	{
		auto ptr = reinterpret_cast<Packet::RoutingPacket*>(packet);
		ptr->hop = htonl(ptr->hop);
	}
	packet->payload_length = htonl(packet->payload_length);
	packet->SubType = htons(packet->SubType);
	return packet;
}

Packet::PacketFrame* Packet::BuildDataPacket(Address from, Address to, const char* data, size_t data_length)
{
	using namespace Packet;
	auto p = new BYTE[sizeof(DataPacket) + data_length];
	new (p) DataPacket(from, to, data_length);
	memcpy(p + sizeof(DataPacket), data, data_length);
	return reinterpret_cast<PacketFrame*>(p);
}

Packet::PacketFrame* Packet::Decode(PacketFrame* packet)
{
	packet->payload_length = ntohl(packet->payload_length);
	packet->SubType = ntohs(packet->SubType);
	if (packet->SubType >= PRT_TRANSMITION)
	{
		auto ptr = reinterpret_cast<Packet::TransmitionPacket*>(packet);
		if (packet->SubType == PRT_TRANSMITION_ICMP)
		{
			auto ptr2 = reinterpret_cast<Packet::ICMPPacket*>(packet);
			ptr2->flag = ntohl(ptr2->flag);
		}
	}
	else
	{
		auto ptr = reinterpret_cast<Packet::RoutingPacket*>(packet);
		ptr->hop = ntohl(ptr->hop);
	}
	return packet;
}
