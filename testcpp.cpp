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
	auto ad = GenerateAddress();
	char* dd;
	UuidToStringA(&ad, (RPC_CSTR*)&dd);
	cout << dd << endl;
	NetworkNode node(ad);
	node.Run(p);
	cout << "Run" << endl;
	while (true)
	{
		cin >> p;
		if (p == 0)
			break;
	}
	node.Stop();
	WSACleanup();
	return 0;
}