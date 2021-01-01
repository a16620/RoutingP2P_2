#include "Address.h"
#pragma comment(lib, "Rpcrt4.lib")
#include <WinSock2.h>

UUID UUIDNtoH(UUID uuid) {
	uuid.Data1 = ntohl(uuid.Data1);
	uuid.Data2 = ntohs(uuid.Data2);
	uuid.Data3 = ntohs(uuid.Data3);
	return uuid;
}

Address GenerateAddress()
{
	UUID uuid;
	auto r = UuidCreate(&uuid);
	if (r != RPC_S_OK && r != RPC_S_UUID_LOCAL_ONLY)
		throw std::runtime_error("蜡老茄 林家 积己 角菩");
	return uuid;
}

UUID UUIDHtoN(UUID uuid) {

	uuid.Data1 = htonl(uuid.Data1);
	uuid.Data2 = htons(uuid.Data2);
	uuid.Data3 = htons(uuid.Data3);
	return uuid;
}