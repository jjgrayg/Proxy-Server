////////////////////////////////////////////////////////
///
///     "connection.h"
///
///     Connection struct definition
///     This object holds the necessary sockets and buffers
///     for reading and writing between hosts
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#ifndef CONNECTION_H
#define CONNECTION_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>

using std::string;

const size_t MAX_BUFF_SIZE = 8096 * 20;

// Connection structure that tracks the socket, streambuf, and request in string form
struct Connection {
public:
    Connection(boost::asio::io_service& io_service) : resolver_(io_service), request_socket_(io_service),
        response_socket_(io_service), request_buffer_(), response_buffer_() { }
    Connection(boost::asio::io_service& io_service, size_t max_buffer_size) : resolver_(io_service), request_socket_(io_service),
        response_socket_(io_service), request_buffer_(max_buffer_size), response_buffer_(max_buffer_size) { }
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket request_socket_, response_socket_;
    boost::asio::streambuf request_buffer_, response_buffer_;
    unsigned char request_data_[MAX_BUFF_SIZE], response_data_[MAX_BUFF_SIZE];
    string request_, response_;
    size_t req_size_, res_size_;
    string connect_time_, remote_host_;
};

#endif // CONNECTION_H
