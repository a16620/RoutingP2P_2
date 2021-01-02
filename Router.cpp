#include "Router.h"
#include <stdexcept>

template<class T>
inline bool equal_ptr(std::shared_ptr<T>& lhs, std::weak_ptr<T>& rhs)
{
	return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}

Router::Router(Address myAddress) : myAddress(myAddress)
{
	Update(myAddress, myAddress, 0);
}

void Router::Register(Address& address, std::weak_ptr<Socket>&& ptr)
{
	if (used.count(address))
		return;

	address_map.insert(std::make_pair(address, ptr));
}

void Router::Remove(std::shared_ptr<Socket>& ptr)
{
	Address t = addr_broadcast;
	for (auto it = address_map.begin(); it != address_map.end(); ++it) {
		if (equal_ptr(ptr, it->second)) {
			t = it->first;
			address_map.erase(it);
			break;
		}
	}

	if (t != addr_broadcast)
		RemoveAddress(t);
}

void Router::RemoveAddress(const Address& addr)
{
	used.insert(addr);
	for (auto it = route_map.begin(); it != route_map.end();)
	{
		if (it->first == addr)
		{
			it = route_map.erase(it);
		}
		else
		{
			if (it->second.first == addr)
			{
				if (it->second.size == 1)
				{
					it = route_map.erase(it);
				}
				else
				{
					it->second.first = it->second.second;
					it->second.first_hop = it->second.second_hop;
					it->second.size--;
					it++;
				}
			}
			else if (it->second.size == 2 && it->second.second == addr)
			{
				it->second.size--;
				it++;
			}
			else
			{
				it++;
			}
		}
	}
}

Address Router::GetMyAddress() const noexcept
{
	return myAddress;
}

auto Router::GetConstIteratorBroadcast() const -> std::tuple<iter_type, iter_type>
{
	return std::tuple<iter_type, iter_type>{address_map.cbegin(), address_map.cend()};
}

auto Router::GetConstIteratorRoute() const -> std::tuple<iter_type2, iter_type2>
{
	return std::tuple<iter_type2, iter_type2>{route_map.cbegin(), route_map.cend()};
}

std::weak_ptr<Socket> Router::Fetch(const Address& addr) const
{
	auto it = route_map.find(addr);
	if (it == route_map.end())
		throw std::out_of_range("경로 탐색 실패");
	auto it2 = address_map.find(it->second.first);
	return it2->second;
}

void Router::Update(Address next, Address dest, size_t hop)
{
	if (used.count(dest))
		return;

	auto it = route_map.find(dest);
	if (it == route_map.end())
	{
		auto tb = RouteTable();
		tb.first = next;
		tb.first_hop = hop;
		tb.size = 1;

		route_map.insert(std::make_pair(dest, tb));
	}
	else
	{
		auto& tb = it->second;
		if (tb.first_hop > hop)
		{
			if (tb.first == next)
			{
				tb.first_hop = hop;
			}
			else
			{
				tb.second = tb.first;
				tb.second_hop = tb.first_hop;
				tb.first = next;
				tb.first_hop = hop;
				tb.size = 2;
			}
		}
		else if (tb.size == 2)
		{
			if (tb.second_hop > hop)
			{
				tb.second = next;
				tb.second_hop = hop;
			}
		}
		else if (tb.first != next)
		{
			tb.second = next;
			tb.second_hop = hop;
			tb.size = 2;
		}
	}
}

void Router::RefreshUsed()
{
	used.clear();
}

bool Router::isUsed(Address& addr) const
{
	return used.count(addr) != 0;
}
