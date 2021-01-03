#pragma once

enum {
	CMD_CONN,
	CMD_FETCH_ADDR,
	CMD_QUERY
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
		NodeInfo ninfo;
		QueryInfo qinfo;
	};
	UINT mode;
};