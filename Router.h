#pragma once
#include "Socket.h"
#include "Address.h"
#include <memory>
#include <tuple>
#include <unordered_set>
#include <unordered_map>

class Router
{

	struct RouteTable {
		Address first, second;
		size_t first_hop, second_hop, size;
	};

	std::unordered_map<Address, std::weak_ptr<Socket>> address_map;
	std::unordered_map<Address, RouteTable> route_map;
	std::unordered_set<Address> used;
	const Address myAddress;

public:
	using iter_type = typename std::unordered_map<Address, std::weak_ptr<Socket>>::const_iterator;
	using iter_type2 = typename std::unordered_map<Address, RouteTable>::const_iterator;
	Router(Address myAddress);
	void Register(Address& address, std::weak_ptr<Socket>&& ptr);
	void Remove(std::shared_ptr<Socket>& ptr);
	void RemoveAddress(const Address& addr);
	Address GetMyAddress() const noexcept;
	auto GetConstIteratorBroadcast() const -> std::tuple<iter_type, iter_type>;
	auto GetConstIteratorRoute() const -> std::tuple<iter_type2, iter_type2>;

	std::weak_ptr<Socket> Fetch(const Address& addr) const;
	void Update(Address next, Address dest, size_t hop);
	void RefreshUsed();
	bool isUsed(Address& addr) const;
};
