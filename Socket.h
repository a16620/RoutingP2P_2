#pragma once
#include <WinSock2.h>

class Socket
{
private:
	SOCKET sock;
public:
	Socket();
	Socket(SOCKET s);
	Socket(const Socket&) = delete;
	Socket(Socket&& s) noexcept;
	~Socket();

	Socket& operator=(Socket&) = delete;
	Socket& operator=(Socket&& s) noexcept;

	SOCKET	get() const noexcept;
	void	reset(SOCKET s);
	bool	empty() const noexcept;
	void	Close();

	int		Send(const char* buffer, size_t length);
	int		Recv(char* buffer, size_t max_length);

	void SendCompletly(const char* buffer, size_t length);

};

void Listen(Socket& s, u_short port);