#pragma once
#define WIN32_LEAN_AND_MEAN
#include <rpc.h>
#include <random>
#include <stdexcept>
#include <string>

using Address = UUID;
UUID UUIDHtoN(UUID uuid);
UUID UUIDNtoH(UUID uuid);
Address GenerateAddress();

constexpr Address addr_broadcast = Address{ 0,0,0,{0} };
inline bool isBroadcast(const Address& addr)
{
	return !(addr.Data1 || addr.Data2 || addr.Data3 || *(__int64*)(&addr.Data4[0]));
}

inline std::string to_string(const UUID& uuid)
{
	RPC_CSTR dd;
	UuidToStringA(&uuid, &dd);
	auto str = std::string(reinterpret_cast<char*>(dd));
	RpcStringFreeA(&dd);
	return std::move(str);
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