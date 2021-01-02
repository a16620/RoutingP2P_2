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
			auto cmd_ = Command();
			cmd_.mode = CMD_SEND;
			cout << "����: ";
			char data[512];
			cin >> data;
			auto length = strlen(data) + 1;
			cmd_.dinfo = DataInfo{addr_to, (uintptr_t)data, length};
			node.PushCommand(cmd_);
		}
		else if (cmd == "data")
		{
			while (!node.outdata.empty())
			{
				Packet::DataPacket* dp;
				if (node.outdata.try_pop(dp))
				{
					printf("%d����Ʈ ����: %s\n", dp->dataLength(), dp + sizeof(Packet::DataPacket));
					DESTROY_PACKET(dp);
				}
			}
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