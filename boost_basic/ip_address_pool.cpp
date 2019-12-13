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

#ifdef _WIN32
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
#endif

#ifdef IOS_MAC
bool IpAddressDetector::ParseIpAddress() {
    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
          // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            ip_v4_list_.push_back(addressBuffer);
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
       //TODO
        }
    }
    if (ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
    PrintIpV4Address();
    if (ip_v4_list_.empty())
        return false;
    return true;
}
#endif

#ifdef ANDROID
bool IpAddressDetector::ParseIpAddress() {
    // File descriptor for socket
    int socketfd;
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd >= 0) {
        struct ifconf conf;
        char data[4096];
        conf.ifc_len = sizeof(data);
        conf.ifc_buf = (caddr_t)data;
        if (ioctl(socketfd, SIOCGIFCONF, &conf) < 0) {
            return false;
        }

        struct ifreq *ifr;
        ifr = (struct ifreq*)data;
        char address_buffer[INET6_ADDRSTRLEN];
        while ((char*)ifr < data + conf.ifc_len) {
            switch (ifr->ifr_addr.sa_family) {
            case AF_INET:
                if (inet_ntop(AF_INET, &((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr,
                    address_buffer, INET_ADDRSTRLEN)) {
                    ip_v4_list_.push_back(address_buffer);
                }
                break;
            case AF_INET6:
                // TODO: handle ipv6 here.
                break;
            }
            ifr = (struct ifreq*)((char*)ifr + sizeof(*ifr));
        }
        close(socketfd);
    }
    else {
        return false;
    }

    if (ip_v4_list_.empty())
        return false;
    PrintIpV4Address();
    return true;
}
#endif


