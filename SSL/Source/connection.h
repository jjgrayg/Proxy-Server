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
#include <boost/asio/ssl.hpp>

namespace ssl = boost::asio::ssl;
typedef ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

using std::string;

const size_t MAX_BUFF_SIZE = 8096 * 20;

// Connection structure that tracks the socket, streambuf, and request in string form
struct Connection {
public:
    Connection(boost::asio::io_service& io_service, ssl::context& ssl_context) : resolver_(io_service), request_socket_(io_service),
        response_socket_(io_service, ssl_context), request_buffer_(), response_buffer_() { }
    Connection(boost::asio::io_service& io_service, ssl::context& ssl_context, size_t max_buffer_size) : resolver_(io_service), request_socket_(io_service),
        response_socket_(io_service, ssl_context), request_buffer_(max_buffer_size), response_buffer_(max_buffer_size) { }
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket request_socket_;
    ssl_socket response_socket_;
    boost::asio::streambuf request_buffer_, response_buffer_;
    unsigned char request_data_[MAX_BUFF_SIZE], response_data_[MAX_BUFF_SIZE];
    string request_, response_;
    size_t req_size_, res_size_;
    string connect_time_, remote_host_;
};

#endif // CONNECTION_H
