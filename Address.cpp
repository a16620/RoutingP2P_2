#include "Address.h"
#pragma comment(lib, "Rpcrt4.lib")
#include <WinSock2.h>

UUID UUIDNtoH(UUID uuid) {
	uuid.Data1 = ntohl(uuid.Data1);
	uuid.Data2 = ntohs(uuid.Data2);
	uuid.Data3 = ntohs(uuid.Data3);
	return uuid;
}

UUID UUIDHtoN(UUID uuid) {

	uuid.Data1 = htonl(uuid.Data1);
	uuid.Data2 = htons(uuid.Data2);
	uuid.Data3 = htons(uuid.Data3);
	return uuid;
}