#pragma once

enum {
	CMD_CONN,
	CMD_SEND
};

struct DataInfo {
	Address address;
	uintptr_t data;
	size_t length;
};

struct NodeInfo {
	ULONG addr;
	USHORT port;
};

struct Command {
	union {
		DataInfo dinfo;
		NodeInfo ninfo;
	};
	UINT mode;
};