#include "ip_detector.h"

#include "ip_address_pool.h"
#include "logger.h"

#include <boost/bind.hpp>

namespace {
    constexpr uint16_t kBufferLen = 1500;
}

IpDetector::IpDetector(const std::string& multicast_ip, uint16_t multicast_port)
    : multicast_ip_(multicast_ip),
    multicast_port_(multicast_port),
    work_(io_service_) {
    ip_address_pool_.reset(new IpAddressPool(io_service_));
}

IpDetector::~IpDetector() {
    io_service_.stop();
    if (detect_thread_.joinable()) {
        detect_thread_.join();
    }
}

bool IpDetector::StartDetect(IpDetectCallback callback) {
    callback_ = std::move(callback);
    
    if (!InitSockets()) {
        return false;
    }

    detect_thread_ = std::move(std::thread(&IpDetector::DoStartReceive, this));
    return true;
}

bool IpDetector::InitSockets() {
    auto ip_v4_list = ip_address_pool_->GetIpV4AddressList();
    boost::system::error_code ec;
    boost::asio::ip::address multicast_address =
        boost::asio::ip::address::from_string(multicast_ip_, ec);
    for (size_t i = 0; i < ip_v4_list.size(); ++i) {
        auto socket = boost::asio::ip::udp::socket(io_service_);
        socket.open(boost::asio::ip::udp::v4(), ec);
        if (ec) {
            LOG_ERROR << "Open socket failed! " << ec.message();
            return false;
        }

        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true), ec);
        boost::asio::ip::address local_address =
            boost::asio::ip::address::from_string(ip_v4_list[i], ec);
        socket.set_option(boost::asio::ip::multicast::join_group(
            multicast_address.to_v4(), local_address.to_v4()), ec);
        if (ec) {
            LOG_ERROR << ip_v4_list[i] << " join group failed! Error code : " << ec;
            return false;
        }
        socket.set_option(boost::asio::socket_base::receive_buffer_size(1000 * 1024), ec);

        // 1. If bind local address here, linux platform can't receive multicast data.
        // 2. If bind 0.0.0.0, linux can receive and send, but windows only can receive.
        // 3. If bind multicast address, linux is OK, but windows unsupported.
        boost::asio::ip::udp::endpoint listen_endpoint(
            boost::asio::ip::address::from_string("0.0.0.0"),
            multicast_port_);
        socket.bind(listen_endpoint, ec);
        if (ec) {
            LOG_ERROR << "Socket bind error: " << ec;
            return false;
        }

        std::unique_ptr<uint8_t> buffer(new uint8_t[kBufferLen]);
        recv_buffers_.insert({ ip_v4_list[i], std::move(buffer) });
        sender_endpoints_.insert({ ip_v4_list[i], boost::asio::ip::udp::endpoint() });
        sockets_.insert({ ip_v4_list[i], std::move(socket) });
    }
    return true;
}

void IpDetector::DoStartReceive() {
    DoAsyncReceive();
    io_service_.run();
}

void IpDetector::DoAsyncReceive() {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    for (auto iter = sockets_.begin(); iter != sockets_.end(); ++iter) {
        if (iter->second.is_open()) {
            iter->second.async_receive_from(
                boost::asio::buffer(recv_buffers_[iter->first].get(), kBufferLen),
                sender_endpoints_[iter->first],
                boost::bind(&IpDetector::ReceiveHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    iter->first));
        }
    }
}

void IpDetector::ReceiveHandler(const boost::system::error_code& error,
                                std::size_t bytes_transferred,
                                const std::string& ip) {
    if (error) {
        LOG_WARN << "Receive data error: " << error << ENDLINE;
        return;
    }

    if (io_service_.stopped()) {
        LOG_INFO << "The io_service has been stopped.";
        return;
    }

    if (bytes_transferred > 0) {
        LOG_INFO << "Ip detected is: " << ip << ENDLINE;

        io_service_.stop();
        CloseAllSockets();

        callback_(ip);
    }
}

void IpDetector::CloseAllSockets() {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    for (auto iter = sockets_.begin(); iter != sockets_.end(); ++iter) {
        iter->second.close();
    }
}

void TestDetectorCallback(const std::string& ip) {
    LOG_INFO << "Valid ip: " << ip << ENDLINE;
}

void TestIpDetector() {
    IpDetector detector("239.0.0.100", 6667);
    detector.StartDetect(TestDetectorCallback);
    system("pause");
}