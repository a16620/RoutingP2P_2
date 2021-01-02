#pragma once

enum {
	CMD_CONN,
	CMD_SEND,
	CMD_FETCH_ADDR,
	CMD_QUERY
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

struct QueryInfo {
	Address address;
};

struct Command {
	union {
		DataInfo dinfo;
		NodeInfo ninfo;
		QueryInfo qinfo;
	};
	UINT mode;
};