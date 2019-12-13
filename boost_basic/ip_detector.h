#pragma once

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>


class IpAddressPool;

using IpDetectCallback = std::function<void(const std::string&)>;

// This class is used to detect valid local ip which can receive multicast data.
class IpDetector {
public:
    IpDetector(const std::string& multicast_ip, uint16_t multicast_port);
    ~IpDetector();
    bool StartDetect(IpDetectCallback callback);

private:
    bool InitSockets();
    void DoStartReceive();
    void DoAsyncReceive();
    void ReceiveHandler(const boost::system::error_code& error,
        std::size_t bytes_transferred,
        const std::string& ip);
    void CloseAllSockets();

private:
    boost::asio::io_service io_service_;
    boost::asio::io_service::work work_;
    std::unique_ptr<IpAddressPool> ip_address_pool_;
    IpDetectCallback callback_;
    std::string multicast_ip_;
    uint16_t multicast_port_;
    std::map<std::string, boost::asio::ip::udp::socket> sockets_;
    std::map<std::string, std::unique_ptr<uint8_t>> recv_buffers_;
    std::map<std::string, boost::asio::ip::udp::endpoint> sender_endpoints_;

    std::thread detect_thread_;

    // The definition of this variable may cause crash, in boardcast project.
    std::mutex socket_mutex_;
};

void TestIpDetector();