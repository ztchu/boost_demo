#include "ip_address_pool.h"

#include <iostream>

#include "logger.h"

using boost::asio::ip::tcp;

void TestIpAddress() {
    boost::asio::io_service io_service;
    IpAddressPool ip_address(io_service);
    ip_address.PrintIpV4Address();
    ip_address.PrintIpV6Address();
    std::vector<std::string> ipv4_address = ip_address.GetIpV4AddressList();
    for (auto iter = ipv4_address.begin(); iter != ipv4_address.end(); ++iter) {
        std::cout << *iter << std::endl;
    }
}

IpAddressPool::IpAddressPool(boost::asio::io_service& io_service)
    : io_service_(io_service){
    ParseIpAddress();
}


IpAddressPool::~IpAddressPool() {
}

std::vector<std::string> IpAddressPool::GetIpV4AddressList() const {
    return ip_v4_list_;
}

std::vector<std::string> IpAddressPool::GetIpV6AddressList() const {
    return ip_v6_list_;
}

size_t IpAddressPool::GetIpV4AddressListsSize() const {
    return ip_v4_list_.size();
}

size_t IpAddressPool::GetIpV6AddressListsSize() const {
    return ip_v6_list_.size();
}

void IpAddressPool::PrintIpV4Address() const {
    if (ip_v4_list_.size() < 1) {
        LOG_INFO << "There is no ip v4 address." << ENDLINE;
        return;
    }
    else if (ip_v4_list_.size() == 1) {
        LOG_INFO << "The ip v4 address is: " << ENDLINE;
    }
    else {
        LOG_INFO << "The ip v4 addresses are: " << ENDLINE;
    }

    for (auto iter = ip_v4_list_.begin(); iter != ip_v4_list_.end(); ++iter) {
        LOG_INFO << *iter << ENDLINE;
    }
}

void IpAddressPool::PrintIpV6Address() const {
    if (ip_v6_list_.size() < 1) {
        LOG_INFO << "There is no ip v6 address." << ENDLINE;
        return;
    }
    else if (ip_v6_list_.size() == 1) {
        LOG_INFO << "The ip v6 address is: " << ENDLINE;
    }
    else {
        LOG_INFO << "The ip v6 addresses are: " << ENDLINE;
    }

    for (auto iter = ip_v6_list_.begin(); iter != ip_v6_list_.end(); ++iter) {
        std::cout << *iter << std::endl;
    }
}

void IpAddressPool::ParseIpAddress() {
    using resolver = boost::asio::ip::tcp::resolver;
    resolver ip_resolver(io_service_);
    resolver::query query(boost::asio::ip::host_name(), "");
    resolver::iterator iter = ip_resolver.resolve(query);
    resolver::iterator end; // End marker.
    std::vector<std::string> ip_address;
    while (iter != end) {
        tcp::endpoint ep = *iter++;
        if (ep.address().is_v4()) {
            ip_v4_list_.push_back(ep.address().to_string());
        }
        else {
            ip_v6_list_.push_back(ep.address().to_string());
        }
    }
}
