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

        boost::asio::ip::udp::endpoint listen_endpoint(local_address, multicast_port_);
        socket.bind(listen_endpoint, ec);
        if (ec) {
            LOG_ERROR << "Socket bind error: " << ec;
            return false;
        }

        recv_buffers_.push_back(new uint8_t[kBufferLen]);
        sender_endpoints_.emplace_back(boost::asio::ip::udp::endpoint());
        sockets_.emplace_back(std::move(socket));
    }
    return true;
}

void IpDetector::DoStartReceive() {
    DoAsyncReceive();
    io_service_.run();
}

void IpDetector::DoAsyncReceive() {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    for (size_t i = 0; i < sockets_.size(); ++i) {
        if (sockets_[i].is_open()) {
            sockets_[i].async_receive_from(
                boost::asio::buffer(recv_buffers_[i], kBufferLen),
                sender_endpoints_[i],
                boost::bind(&IpDetector::ReceiveHandler, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    std::ref(sockets_[i])));
        }
    }
}

void IpDetector::ReceiveHandler(const boost::system::error_code& error,
                                std::size_t bytes_transferred,
                                boost::asio::ip::udp::socket& socket) {
    if (error) {
        LOG_WARN << "Receive data error: " << error << ENDLINE;
        return;
    }

    if (bytes_transferred > 0) {
        auto ip_str = socket.local_endpoint().address().to_string();
        LOG_INFO << "Ip detected is: " << ip_str << ENDLINE;
        CloseAllSockets();

        io_service_.stop();
        callback_(ip_str);
    }
}

void IpDetector::CloseAllSockets() {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    for (size_t i = 0; i < sockets_.size(); ++i) {
        sockets_[i].close();
        if (recv_buffers_[i]) {
            delete[] recv_buffers_[i];
            recv_buffers_[i] = nullptr;
        }
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