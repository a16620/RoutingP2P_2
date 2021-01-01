#pragma once
#define WIN32_LEAN_AND_MEAN
#include <rpc.h>
#include <random>

using Address = UUID;
UUID UUIDHtoN(UUID uuid);
UUID UUIDNtoH(UUID uuid);

constexpr Address addr_broadcast = Address{ 0,0,0,{0} };
inline bool isBroadcast(const Address& addr)
{
	return !(addr.Data1 || addr.Data2 || addr.Data3 || *(__int64*)(&addr.Data4[0]));
}

namespace std {
	template<>
	struct hash<UUID> {
		size_t operator()(const UUID& o) const {
			size_t h = o.Data1 ^ o.Data2 ^ o.Data3;
			for (int i = 0; i < 8; ++i)
				h ^= o.Data4[i];
			return h;
		}
	};
}