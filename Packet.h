#pragma once
#include "Address.h"
#include <memory>
#define DESTROY_PACKET(pk) delete[] (char*)(pk)

namespace Packet {
	enum {
		PRT_NEIGHBOUR_ROUTING,
		PRT_TRANSMITION,
		PRT_TRANSMITION_ICMP,
		PRT_TRANSMITION_DATA
	};

	enum {
		ICMP_NAREACH,
		ICMP_INFORM_ADDR,
		ICMP_TRAFFIC_INFO
	};

	struct PacketFrame {
		u_long payload_length;
		u_short SubType, ext;

		PacketFrame(u_long payload_length, u_short subType) : payload_length(payload_length), SubType(subType) {}
	};

	struct RoutingPacket : PacketFrame {
		Address next;
		Address dest;
		u_long hop;

		RoutingPacket(Address local, Address dest, size_t hop) : PacketFrame(sizeof(Address)*2 + sizeof(u_long), PRT_NEIGHBOUR_ROUTING) {
			this->next = local;
			this->dest = dest;
			this->hop = hop;
		}
	};
	
	struct TransmitionPacket : PacketFrame {
		Address from, to;
		TransmitionPacket(u_long payload_length, u_short subType, Address from, Address to) : PacketFrame(sizeof(Address) * 2 + payload_length, subType), from(from), to(to) {}
	};

	struct DataPacket : TransmitionPacket {
		DataPacket(Address from, Address to, u_long data_length) : TransmitionPacket(data_length, PRT_TRANSMITION_DATA, from, to) {}
		size_t dataLength() const {
			return payload_length - (sizeof(TransmitionPacket) - sizeof(PacketFrame));
		}
	};

	struct ICMPPacket : TransmitionPacket {
		ULONG flag;
		ICMPPacket(Address from, Address to, ULONG flag) : TransmitionPacket(sizeof(ULONG) + 2 * sizeof(Address), PRT_TRANSMITION_ICMP, from, to), flag(flag) {}
	};

	PacketFrame* Encode(PacketFrame* packet);
	PacketFrame* Decode(PacketFrame* packet);

	template <class Base, typename... T>
	PacketFrame* Generate(T&&... ty)
	{
		auto p = new BYTE[sizeof(Base)];
		new (p) Base(std::forward<T>(ty)...);
		return reinterpret_cast<PacketFrame*>(p);
	}

	PacketFrame* BuildDataPacket(Address from, Address to, const char* data, size_t data_length);
	
}