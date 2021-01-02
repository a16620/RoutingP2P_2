#pragma comment(lib, "ws2_32.lib")
#include "Socket.h"
#include <stdexcept>

Socket::Socket()
{
	sock = make_tcp_socket();
}

Socket::Socket(SOCKET s)
{
	sock = s;
}

Socket::Socket(Socket&& s) noexcept
{
	sock = s.sock;
	s.sock = INVALID_SOCKET;
}

Socket& Socket::operator=(Socket&& s) noexcept
{
	reset(s.sock);
	s.sock = INVALID_SOCKET;
	return *this;
}

SOCKET Socket::get() const noexcept
{
	return sock;
}

void Socket::reset(SOCKET s)
{
	Close();
	sock = s;
}

bool Socket::empty() const noexcept
{
	return sock == INVALID_SOCKET;
}

void Socket::Close()
{
	closesocket(sock);
	sock = INVALID_SOCKET;
}

int Socket::Send(const char* buffer, size_t length)
{
	auto r = send(sock, buffer, length, 0);
#ifdef DISCON_ERROR
	if (r <= 0)
#else
	if (r == SOCKET_ERROR)
#endif
		throw std::runtime_error("���� ����");
	return r;
}

int Socket::Recv(char* buffer, size_t max_length)
{
	auto r = recv(sock, buffer, max_length, 0);
#ifdef DISCON_ERROR
	if (r <= 0)
#else
	if (r == SOCKET_ERROR)
#endif
		throw std::runtime_error("���� ����");
	return r;
}

void Socket::SendCompletly(const char* buffer, size_t length)
{
	size_t sent = 0;
	do
	{
		sent += Send(buffer + sent, length - sent);
	} while (sent < length);
}

Socket::~Socket()
{
	Close();
}

void Listen(std::shared_ptr<Socket>& s, u_short port)
{
	if (s->get() == INVALID_SOCKET)
		throw std::invalid_argument("����� ������ ���������Դϴ�");

	sockaddr_in addr;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	int optval = 1;
	setsockopt(s->get(), SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

	bind(s->get(), reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in));
	listen(s->get(), SOMAXCONN);
}

SOCKET AcceptNoAddress(std::shared_ptr<Socket>& s)
{
	sockaddr_in addr;
	int szAcp = sizeof(sockaddr_in);
	SOCKET ss = accept(s->get(), reinterpret_cast<sockaddr*>(&addr), &szAcp);
	if (ss == INVALID_SOCKET)
	{
		throw std::runtime_error("���� ���� ����");
	}
	return ss;
}

SOCKET make_tcp_socket()
{
	auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		throw std::runtime_error("���� ���� ����");
	return sock;
}
