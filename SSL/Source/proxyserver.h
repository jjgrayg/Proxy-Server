////////////////////////////////////////////////////////
///
///     "proxyserver.h"
///
///     Class declaration for ProxyServer
///     This is the main object that handles the functionality
///     of the proxy server
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <QMutex>
#include <iterator>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include "connection.h"
#include "utilities.h"
#include "ui_servercontroller.h"
#include "ioServiceThread.h"
#include "cachemanager.h"

typedef boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO> rcv_timeout_option;

using std::vector;

class ProxyServer : public QObject {
signals:
    void ready_write_console(QString to_insert);
    void new_req(size_t);
    void cache_hit(size_t);
    void add_cache_bytes(size_t);
    void add_req_bytes(size_t);

public:
    ProxyServer(unsigned int cache_size = 10240, uint16_t prt = 8080, bool log = true, bool debug = false, string name = "") :
        QObject(), logging_(log), debugging_(debug), port_(prt), cache_size_(cache_size),
        name_(name), log_writer_(), m_ioservice_(), m_acceptor_(m_ioservice_), m_connections_(),
        req_num(0), req_bytes(0), cache_hits(0), cache_hit_size(0), ctx(boost::asio::ssl::context::sslv23) { }

    void stop();
    void start();

    bool is_running();

private:
    Q_OBJECT
    bool logging_;
    bool debugging_;
    uint16_t port_;
    size_t cache_size_;

    string name_;

    IOServiceThread *io_service_thread_;
    QMutex writer_mutex_, con_mutex_, resolver_mutex_;

    std::ofstream log_writer_;
    boost::asio::io_service m_ioservice_;
    boost::asio::ip::tcp::acceptor m_acceptor_;
    std::list<Connection> m_connections_;
    using con_handle_t = std::list<Connection>::iterator;

    CacheManager *cache_manager_;

    size_t req_num, req_bytes, cache_hits, cache_hit_size;

    boost::asio::ssl::context ctx;

    // Utility funcs
    void write_to_outputs(string);
    void write_to_log(string);
    string request_to_str_buff(con_handle_t, size_t buff_size = 0);
    string response_to_str_buff(con_handle_t, size_t buff_size = 0);
    string get_req_path(string const&, string const&);
    size_t get_http_size(string const&);
    string get_response_encoding(string const&);
    string get_transfer_encoding(string const&);
    size_t get_max_age(string const&);
    size_t get_age(string const&);
    void handle_local_request(con_handle_t);

    bool verify_certificate(bool, boost::asio::ssl::verify_context&);


    // Connection funcs
    void start_accept();
    void handle_accept(con_handle_t, boost::system::error_code const&);
    void handle_up_connect(con_handle_t, const std::string &, const std::string &, boost::system::error_code const&);
    void handle_up_handshake(con_handle_t, boost::system::error_code const&);
    void establish_up_connection(con_handle_t, string const&, string const&);
    void handle_bidirectional_connection(con_handle_t, string const&, string const&, boost::system::error_code const&);
    void close_connection(con_handle_t);
    void do_down_async_read(con_handle_t);

    // Parsing funcs
    void handle_parse(con_handle_t, boost::system::error_code const&, size_t);
    string get_request_type(con_handle_t);
    string get_host(con_handle_t);
    std::string get_type(string const&);

    // Routines
    // GET ROUTINE
    void do_http_routine(con_handle_t, string const&);
    void close_http_connection(con_handle_t, boost::system::error_code const&, size_t, double);
    void read_http_header(con_handle_t, string const&, boost::system::error_code const&);
    void read_http_response(con_handle_t, string const&, boost::system::error_code const&, size_t);
    void write_http_response(con_handle_t, string const&, double, size_t buff_size = 0);
    string formulate_request(con_handle_t, string const&);
    // END GET ROUTINE
    // CONNECT ROUTINE
    void do_connect_routine(con_handle_t);
    void e2e_connection_handler(con_handle_t, boost::system::error_code const&);
    void read_connect_response(con_handle_t, boost::system::error_code const&);
    void write_connect_response(con_handle_t, boost::system::error_code const&, size_t);
    void read_connect_request(con_handle_t, boost::system::error_code const&);
    void write_connect_request(con_handle_t, boost::system::error_code const&, size_t);
    void write_and_close_request(con_handle_t, boost::system::error_code const&);
    void write_and_close_response(con_handle_t, boost::system::error_code const&);
    // END CONNECT ROUTINE

};

string get_content_encoding(string const&);
string convert_to_encoding(string const&);
string convert_to_transfer_encoding(string const&);

#endif // PROXYSERVER_H
