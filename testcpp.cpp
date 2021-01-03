#include "NetworkNode.h"
#include <iostream>
using namespace std;

int main()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	int p;
	cout << "port: ";
	cin >> p;
	auto address = GenerateAddress();
	NetworkNode node(address);
	node.Run(p);
	cout << "Run" << endl;
	string cmd;
	while (true)
	{
		cout << '(' << node.outdata.unsafe_size() << ")>";
		cin >> cmd;
		if (cmd == "addr")
		{
			RPC_CSTR dd;
			UuidToStringA(&address, &dd);
			cout << dd << endl;
			RpcStringFreeA(&dd);
		}
		else if (cmd == "send")
		{
			cout << "�ּ�: ";
			cin >> cmd;
			Address addr_to;
			UuidFromStringA((RPC_CSTR)(cmd.c_str()), &addr_to);
			cout << "����: ";
			char data[512];
			cin >> data;
			auto length = strlen(data) + 1;
			node.PushPost(addr_to, data, length);
		}
		else if (cmd == "daa")
		{
			while (!node.outdata.empty())
			{
				Packet::DataPacket* dp;
				if (node.outdata.try_pop(dp))
				{
					RPC_CSTR dd;
					UuidToStringA(&dp->from, &dd);
					cout << "�߽���: " << dd;
					RpcStringFreeA(&dd);
					printf(" %d����Ʈ ����:\n%s\n", dp->dataLength(), (char*)dp + sizeof(Packet::DataPacket));
					DESTROY_PACKET(dp);
				}
			}
		}
		else if (cmd == "con")
		{
			cout << "�ּ�: ";
			cin >> cmd;
			ULONG target;
			if (cmd == "loopback" || cmd == "localhost" || cmd == "n")
				target = htonl(INADDR_LOOPBACK);
			else
				target = inet_addr(cmd.c_str());
			cout << "��Ʈ: ";
			USHORT port;
			cin >> port;

			auto cmd_ = Command();
			cmd_.mode = CMD_CONN;
			cmd_.ninfo = NodeInfo{ target, port };
			node.PushCommand(cmd_);
		}
		else if (cmd == "addrs")
		{
			auto cmd_ = Command();
			cmd_.mode = CMD_FETCH_ADDR;
			node.PushCommand(cmd_);
		}
		else if (cmd == "qry")
		{
			auto cmd_ = Command();
			cmd_.mode = CMD_QUERY;
			cout << "�ּ�: ";
			cin >> cmd;
			Address addr_to;
			UuidFromStringA((RPC_CSTR)(cmd.c_str()), &addr_to);
			cmd_.qinfo.address = addr_to;
			node.PushCommand(cmd_);
		}
		else if (cmd == "exit")
		{
			break;
		}
		else
		{
			cout << "..." << endl;
		}
	}
	node.Stop();
	WSACleanup();
	return 0;
}