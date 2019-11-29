#pragma once

#include <boost/asio.hpp>
#include <string>
#include <vector>

void TestIpAddress();

class IpAddressPool
{
public:
    IpAddressPool(boost::asio::io_service& io_service);
    ~IpAddressPool();

    std::vector<std::string> GetIpV4AddressList() const;
    std::vector<std::string> GetIpV6AddressList() const;

    size_t GetIpV4AddressListsSize() const;
    size_t GetIpV6AddressListsSize() const;

    void PrintIpV4Address() const;
    void PrintIpV6Address() const;

private:
    void ParseIpAddress();

private:
    boost::asio::io_service& io_service_;
    std::vector<std::string> ip_v4_list_;
    std::vector<std::string> ip_v6_list_;
};

