////////////////////////////////////////////////////////
///
///     "proxyserver.cpp"
///
///     Implementation of functions declared in
///     proxyserver.h
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#include "proxyserver.h"

// Writes string to log file and console screen
void ProxyServer::write_to_outputs(string str) {
    writer_mutex_.lock();
    emit ready_write_console(QString::fromStdString("<span><span style=\"color:grey\">[" + get_current_date_and_time() + "]</span> ~ " + str + "\n"));
    if (logging_) {
        string log_file_name = "log.txt";
        log_writer_.open(log_file_name, std::ios::app);
        if (!log_writer_) emit ready_write_console(QString::fromStdString("span><span style=\"color:grey\">[" + get_current_date_and_time() +
                                                                          "]</span> ~ <span style=\"color:green\">SERVER</span>:: "
                                                                          "<span style=\"color:red\">!!ERROR!! </span>Could not create log. At line #" +
                                                                          std::to_string(__LINE__) + "</span>\n"));
        if (log_writer_ && str.find("Server stopped!") != string::npos){
            str = "Server stopped!\n";
            log_writer_ << str;
        }
        else if (log_writer_ && str.find("DEBUGGING::") == string::npos &&
                str.find("!!ERROR!!") == string::npos)
            log_writer_ << str;
        else if (log_writer_ && str.find("!!ERROR!!") != string::npos){
            str = "ERROR:: " + str.substr(str.find(" </span>") + 8, str.size());
            log_writer_ << str;
        }
        log_writer_.close();
    }
    writer_mutex_.unlock();
}

// Writes to csv log
void ProxyServer::write_to_log(string str) {
    writer_mutex_.lock();
    std::ofstream log_writer;
    log_writer.open("log_file.csv", std::ios::app);
    if (log_writer)
        log_writer << str;
    log_writer.close();
    writer_mutex_.unlock();
}

// Neatly closes connection
void ProxyServer::close_connection(con_handle_t con_handle) {
    con_mutex_.lock();
    if (con_handle->request_buffer_.size() > 0) con_handle->request_buffer_.consume(con_handle->request_buffer_.size());
    if (con_handle->response_buffer_.size() > 0) con_handle->response_buffer_.consume(con_handle->response_buffer_.size());
    try {
        if (debugging_)
            write_to_outputs("<span style=\"color:blue\">DEBUGGING:: </span> Closing Connection! to " + con_handle->request_socket_.remote_endpoint().address().to_string() + "\n");
        con_handle->request_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        con_handle->request_socket_.close();
    }
    catch(boost::system::system_error e) {}
    try {
        if (debugging_)
            write_to_outputs("<span style=\"color:blue\">DEBUGGING:: </span> Closing Connection! to " + con_handle->response_socket_.remote_endpoint().address().to_string() + "\n");
        con_handle->response_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        con_handle->response_socket_.close();
    }
    catch(boost::system::system_error e) {}
    if (con_handle != m_connections_.end())
        m_connections_.erase(con_handle);

    con_mutex_.unlock();
}

// Reads buffer into string
string ProxyServer::request_to_str_buff(con_handle_t con_handle, size_t buff_size) {
    string str;
    if (con_handle->request_buffer_.size() > 0) {
        str = string((std::istreambuf_iterator<char>(&con_handle->request_buffer_)),
                            std::istreambuf_iterator<char>());
    }
    else {
        str = string(con_handle->request_data_, con_handle->request_data_ + (buff_size == 0 ? MAX_BUFF_SIZE : buff_size));
    }
    try {
        con_handle->request_ = str;
    }
    catch (std::logic_error const& e) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + string(e.what()) + " at line #" + std::to_string(__LINE__) + "\n");
        con_handle->request_ = "";
    }

    con_handle->req_size_ = str.size();
    return str;
}

// Reads buffer into string
string ProxyServer::response_to_str_buff(con_handle_t con_handle, size_t buff_size) {
    string str;
    if (con_handle->response_buffer_.size() > 0) {
        str = string((std::istreambuf_iterator<char>(&con_handle->response_buffer_)),
                            std::istreambuf_iterator<char>());
    }
    else {
        str = string(con_handle->response_data_, con_handle->response_data_ + (buff_size == 0 ? MAX_BUFF_SIZE : buff_size));
    }
    try {
        con_handle->response_ = str;
    }
    catch (std::logic_error const& e) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + string(e.what()) + " at line #" + std::to_string(__LINE__) + "\n");
        con_handle->response_ = "";
    }

    con_handle->res_size_ = str.size();
    return str;
}

// Returns the request type (GET, POST, CONNECT, etc.)
string ProxyServer::get_request_type(con_handle_t con_handle) {
    size_t pos = con_handle->request_.find("\r\n\r\n");
    if (pos != string::npos) {
        vector<string> vec = split_string(con_handle->request_.substr(0, pos), ' ');
        if (vec.size() > 1)
            return vec[0];
        return "";
    }
    return "";
}

// Returns the requested host
string ProxyServer::get_host(con_handle_t con_handle) {
    vector<string> vec = split_string(con_handle->request_, ' ', '\r');
    auto it = std::find(vec.begin(), vec.end(), "\nHost:");
    if (it != vec.end())
        return *(++it);
    return "";
}

// Returns the requested path at the host
string ProxyServer::get_req_path(string const& s, string const& host) {
    vector<string> vec = split_string(s, ' ');
    string request_path = vec[1].substr(("https://" + host).length() - 1, vec[1].length());
    if (request_path[0] != '/') {
        request_path = vec[1].substr(("http://" + host).length() - 1, vec[1].length());
    }
    return request_path;
}

// Gets the size of the HTTP header
size_t ProxyServer::get_http_size(string const& buff) {
    vector<string> vec = split_string(buff.substr(0, buff.find("\r\n\r\n")), ' ', '\r');
    auto it = std::find(vec.begin(), vec.end(), "\nContent-Length:");
    if (it != vec.end())
        return std::stoul(*(++it));
    return std::stoul("0");
}

// Extracts transfer encoding from HTTP header
string ProxyServer::get_transfer_encoding(string const& request) {
    vector<string> vec = split_string(request.substr(0, request.find("\r\n\r\n")), ' ', '\r');
    auto it = std::find(vec.begin(), vec.end(), "\nTransfer-Encoding:");
    if (it != vec.end()) {
        ++it;
        if ((*it).find("\r\n") != string::npos)
            return (*it).substr(0, (*it).find("\r\n"));
        else
            return *it;
    }
    return "";
}

// Extracts max age of cached file from HTTP header
size_t ProxyServer::get_max_age(string const& buff) {
    vector<string> vec = split_string(buff.substr(0, buff.find("\r\n\r\n")), ' ', '\r');
    auto it = std::find(vec.begin(), vec.end(), "\nCache-Control:");
    string control;
    if (it != vec.end())
        control = *(++it);
    if(control.find("max-age=") != string::npos)
        return std::stoul(control.substr(control.find("max-age=") + 8, control.size()));
    return std::stoul("0");
}

// Extracts current age of file from HTTP header
size_t ProxyServer::get_age(string const& buff) {
    vector<string> vec = split_string(buff.substr(0, buff.find("\r\n\r\n")), ' ', '\r');
    auto it = std::find(vec.begin(), vec.end(), "\nAge:");
    if (it != vec.end())
        return std::stoul(*(++it));
    return std::stoul("0");
}

// Handles requests to the server itself (ex: localhost:8080/proxy_usage?)
void ProxyServer::handle_local_request(con_handle_t con_handle) {
    auto c = con_handle->request_.substr(con_handle->request_.find(' ') + 1, con_handle->request_.find("\r\n"));
    if (c.find("proxy_usage?") != string::npos) {
        string body = "<!DOCTYPE html>"
                      "<html lang='en'>"
                      "<head>"
                      "<style>"
                      "body {"
                      " text-align: center;"
                      "}"
                      "table, th, td {"
                      " border: 1px solid black;"
                      "}"
                      "</style>"
                      "</head>"
                      "<body>"
                      "<table>"
                      "<tr>"
                        "<th># Requests</th>"
                        "<th>Total Request Bytes</th>"
                        "<th>Cache Hits</th>"
                        "<th>Cache Fetched Size</th>"
                      "</tr>"
                      "<tr>"
                        "<td>" + std::to_string(req_num) +"</td>"
                        "<td>" + std::to_string(req_bytes) +"</td>"
                        "<td>" + std::to_string(cache_hits) +"</td>"
                        "<td>" + std::to_string(cache_hit_size) +"</td>"
                      "</tr>"
                      "</table></body>";
        string res = "HTTP/1.1 200 OK\r\n Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
        auto buff = std::make_shared<string>(res);
        auto handler = boost::bind(&ProxyServer::close_connection, this, con_handle);
        boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(*buff), handler);
    }
    else if (c.find("proxy_usage_reset?") != string::npos) {
        req_num = 0;
        req_bytes = 0;
        cache_hits = 0;
        cache_hit_size = 0;
        emit new_req(req_num);
        emit add_req_bytes(req_bytes);
        emit cache_hit(cache_hits);
        emit add_cache_bytes(cache_hit_size);
        log_writer_.open("log_file.csv");
        if (log_writer_)
            log_writer_ << "Requested time, Response time, Cache hit, Requested string\n";
        log_writer_.close();
        string res = "HTTP/1.1 200 OK\r\n\r\nUsage Reset!";
        auto buff = std::make_shared<string>(res);
        auto handler = boost::bind(&ProxyServer::close_connection, this, con_handle);
        boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(*buff), handler);
        write_to_outputs("<span style=\"color:green\">SERVER</span>:: Reset usage request received at " + get_current_date_and_time() + "\n");
    }
    else if (c.find("proxy_log?") != string::npos) {
        std::ifstream is("log_file.csv");
        std::stringstream buffer;
        buffer << is.rdbuf();
        string res = "HTTP/1.1 200 OK\r\n Content-Length: " + std::to_string(buffer.str().size()) + "\r\n\r\n" + buffer.str();
        auto buff = std::make_shared<string>(res);
        auto handler = boost::bind(&ProxyServer::close_connection, this, con_handle);
        boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(*buff), handler);
    }
}

// Extracts content type from HTTP header
string ProxyServer::get_type(string const& buff) {
    vector<string> vec = split_string(buff.substr(0, buff.find("\r\n\r\n")), '\r');
    vector<string>::iterator it;
    for (it = vec.begin(); it != vec.end(); ++it) {
        if ((*it).find("Content-Type:") != string::npos) break;
    }
    if (it != vec.end()) {
        if ((*it).find(" ") != string::npos) {
            return (*it).substr((*it).find(" ") + 1,  (*it).size());
        }
        else
            return *it;
    }
    return "";
}

// Extracts content encoding from HTTP header
string get_content_encoding(string const& buff) {
    vector<string> vec = split_string(buff.substr(0, buff.find("\r\n\r\n")), ' ', '\r');
    auto it = std::find(vec.begin(), vec.end(), "\nContent-Encoding:");
    if (it != vec.end()) {
        ++it;
        if ((*it).find("\r\n") != string::npos)
            return (*it).substr(0, (*it).find("\r\n"));
        else
            return *it;
    }
    return "";
}

// Creates correct Content-Encoding header param for a cache response
string convert_to_encoding(string const& type) {
    if (type != "")
        return "Content-Encoding: " + type + "\r\n";
    else
        return "";
}

// Creates correct Transfer-Encoding header param for a cache response
string convert_to_transfer_encoding(string const& type) {
    if (type != "")
        return "Transfer-Encoding: " + type + "\r\n";
    else
        return "";
}
//========================================

// Pars the downstream request to decide whether to cache (HTTP) or to utilize
// the CONNECT protocol (HTTPS)
void ProxyServer::handle_parse(con_handle_t con_handle, boost::system::error_code const& e, size_t bytes_transfered) {
    con_handle->connect_time_ = get_current_date_and_time();
    if ((!e || e == boost::asio::error::eof) && bytes_transfered != 0) {
        emit new_req(++req_num);
        request_to_str_buff(con_handle);
        if ((con_handle->request_.substr(con_handle->request_.find(' ') + 1, con_handle->request_.find("/r/n")))[0] == '/') {
            handle_local_request(con_handle);
            return;
        }
        string req_type = get_request_type(con_handle);
        string host = get_host(con_handle);
        emit add_req_bytes(req_bytes += con_handle->request_.size());
        establish_up_connection(con_handle, host, req_type);
    }
    else
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
}

// Handle what happens when both hosts are connected
void ProxyServer::handle_bidirectional_connection(con_handle_t con_handle, string const& req_type, string const& host, boost::system::error_code const& e) {
    if (!e) {
        con_handle->remote_host_ = split_string(con_handle->request_.substr(0, con_handle->request_.find("/r/n/r/n")), ' ')[1];
        if (debugging_) {
            write_to_outputs("<span style=\"color:blue\">DEBUGGING:: </span>Incoming connection from: " + con_handle->request_socket_.remote_endpoint().address().to_string() +
                             " and to: " + con_handle->response_socket_.remote_endpoint().address().to_string() + ". Requested path: " + con_handle->remote_host_ + "\n");
        }
        if (req_type != "CONNECT") {
            // Handle connection with cache
            do_http_routine(con_handle, host);
        }
        else if (req_type == "CONNECT") {
            // Handle using CONNECT protocol
            do_connect_routine(con_handle);
        }
        else {
            // Handle with error
        }
    }
    else
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
}

// Establish the upstream connection
void ProxyServer::establish_up_connection(con_handle_t con_handle, string const& host, string const& req_type) {
    vector<string> sv = split_string(host, ':');
    if (sv.size() > 0) {
        string port;
        if (sv.size() < 2) port = "80";
        else port = sv[1] == "443" ? "443" : "80";
        try {
            string host = sv[0];
            boost::asio::ip::tcp::resolver::query query(sv[0], port);
            boost::asio::ip::tcp::resolver::iterator iter = con_handle->resolver_.resolve(query);
            auto connect_handler = boost::bind(&ProxyServer::handle_bidirectional_connection, this, con_handle, req_type, host, boost::asio::placeholders::error);
            con_handle->response_socket_.open(iter->endpoint().protocol());
            con_handle->response_socket_.async_connect(iter->endpoint(), connect_handler);
        }
        catch (boost::system::error_code const& e) {
            write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
        }
        catch (std::exception const& e) {
            write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + string(e.what()) + " at line #" + std::to_string(__LINE__) + "\n");
        }
        catch (...) {
            write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>Unhandled exception at line #" + std::to_string(__LINE__) + "\n");
        }
    }
}

// Perform an asynchronous read on the connecction until the end of the request marked by \r\n\r\n
void ProxyServer::do_down_async_read(con_handle_t con_handle) {
    try {
        auto handler = boost::bind(&ProxyServer::handle_parse, this, con_handle, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        boost::asio::async_read_until(con_handle->request_socket_, con_handle->request_buffer_, "\r\n\r\n", handler);
    }
    catch (boost::system::error_code const& e) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
    }
}

// If the acception was successful begin reading from the downstream socket
void ProxyServer::handle_accept(con_handle_t con_handle, boost::system::error_code const& e) {
    if (!e) {
        do_down_async_read(con_handle);
    }
    else {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
        if (con_handle != m_connections_.end()) {
            close_connection(con_handle);
        }
    }
    start_accept();
}

// Add the connection to the list and asynchronously accept it
void ProxyServer::start_accept() {
    auto con_handle = m_connections_.emplace(m_connections_.begin(), m_ioservice_);
    auto handler = boost::bind(&ProxyServer::handle_accept, this, con_handle, boost::asio::placeholders::error);
    m_acceptor_.async_accept(con_handle->request_socket_, handler);
}

// Begin running the Boost ioservice and listening for incoming requests on the port provided and call start_accept for each new connection
void ProxyServer::start() {
    write_to_outputs("<span style=\"color:green\">SERVER</span>:: Starting sever on port: \"" + std::to_string(port_) + "\"\n");
    if (logging_) {
        std::ifstream log_file("log_file.csv");
        if (!log_file) {
            log_writer_.open("log_file.csv");
            if (log_writer_)
                log_writer_ << "Requested time, Response time, Cache hit, Requested string\n";
            log_writer_.close();
        }
    }
    cache_manager_ = new CacheManager(cache_size_);
    auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_);
    m_acceptor_.open(endpoint.protocol());
    m_acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    m_acceptor_.bind(endpoint);
    m_acceptor_.listen();
    start_accept();
    io_service_thread_ = new IOServiceThread(&m_ioservice_);
    io_service_thread_->start();
    write_to_outputs("<span style=\"color:green\">SERVER</span>:: <span style=\"color:green\">Server started successfully!</span>\n");
}

// Stops the server and all its services
void ProxyServer::stop() {
    write_to_outputs("<span style=\"color:green\">SERVER</span>:: Stopping sever...");
    req_num = 0;
    req_bytes = 0;
    cache_hits = 0;
    cache_hit_size = 0;
    emit new_req(req_num);
    emit add_req_bytes(req_bytes);
    emit cache_hit(cache_hits);
    emit add_cache_bytes(cache_hit_size);
    m_ioservice_.stop();
    io_service_thread_->terminate();
    io_service_thread_->wait();
    delete io_service_thread_;
    delete cache_manager_;
    if (m_acceptor_.is_open())
        m_acceptor_.close();
    m_connections_.clear();
    write_to_outputs("<span style=\"color:green\">SERVER</span>:: <span style =\"color:red\"><i>Server stopped!</i></span><br>");
}

//===========================
// ROUTINES
//===========================

//++++++++++++++++++++++++++ HTTP ROUTINE

// Begin HTTP routine by checking if file is in cache and handling either outcome
void ProxyServer::do_http_routine(con_handle_t con_handle, string const& host) {
    string request_path = get_req_path(con_handle->request_, host);
    string cache_filename = replace_all(host, ".", "_") + (request_path == "/" ? "/index.html" : request_path);
    if (cache_filename[cache_filename.size() - 1] == '/') cache_filename += "index.html";
    int result = cache_manager_->check_cache(cache_filename);
    if (result != 1) { // Cache miss
        write_to_outputs("<span style=\"color:green\">SERVER</span>:: <span style=\"color:red\">Cache miss</span>\n");
        if (result == 0) cache_manager_->delete_file(cache_filename);
        auto buff = std::make_shared<string>(formulate_request(con_handle, "GET " + request_path + " HTTP/1.1\r\n"));
        auto handler = boost::bind(&ProxyServer::read_http_header, this, con_handle, cache_filename, boost::asio::placeholders::error);
        boost::asio::async_write(con_handle->response_socket_, boost::asio::buffer(*buff), handler);
        write_to_log(con_handle->connect_time_ + ", " + get_current_date_and_time() + ", false, " + con_handle->remote_host_ + "\n");
    }
    else if (result == 1) {
        emit cache_hit(++cache_hits);
        write_to_outputs("<span style=\"color:green\">SERVER</span>:: <span style=\"color:green\">Cache hit</span>\n");
        string type = cache_manager_->get_file_type();
        string encoding_type = convert_to_encoding(cache_manager_->get_file_encoding());
        string transfer = convert_to_transfer_encoding(cache_manager_->get_transfer_encoding());
        string content_length;

        vector<unsigned char> proxy_res = cache_manager_->consume_binary();
        emit add_cache_bytes(cache_hit_size += proxy_res.size());

        if (transfer == "")
            content_length = "Content-Length: " + std::to_string(proxy_res.size()) + "\r\n";
        con_handle->response_ = "HTTP/1.1 200 OK\r\n" + content_length +
                "Content-Type: " + type + "\r\n" +
                encoding_type + transfer + "\r\n";
        size_t s = proxy_res.size();
        for(size_t i = 0; i < proxy_res.size(); ++i) con_handle->response_ += proxy_res[i];

        auto handler = boost::bind(&ProxyServer::close_http_connection, this, con_handle, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, time_since_epoch());
        boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(con_handle->response_, con_handle->response_ .size()),
                                 boost::asio::transfer_exactly(con_handle->response_.size()), handler);
        write_to_log(con_handle->connect_time_ + ", " + get_current_date_and_time() + ", true, " + con_handle->remote_host_ + "\n");
    }
}

// Formulate proper request to send to remote host
string ProxyServer::formulate_request(con_handle_t con_handle, string const& top_line) {
    string request = top_line;
    vector<string> vec = split_string(con_handle->request_, '\n');
    for(uint16_t i = 1; i < vec.size(); ++i) {
        request += vec[i] + "\n";
    }
    request += "\r\n";
    return request;
}

// Read the response header from the socket
void ProxyServer::read_http_header(con_handle_t con_handle, string const& cache_filename, boost::system::error_code const& e) {
    if (!e) {
        auto handler = boost::bind(&ProxyServer::read_http_response, this, con_handle, cache_filename, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        boost::asio::async_read_until(con_handle->response_socket_, con_handle->response_buffer_, "\r\n\r\n", handler);
    }
    else {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
        if (con_handle != m_connections_.end()) {
            close_connection(con_handle);
        }
    }
}

// Read the rest of the response
void ProxyServer::read_http_response(con_handle_t con_handle, string const& cache_filename, boost::system::error_code const& e, size_t bytes_transferred) {
    if (!e) {
        string temp = con_handle->response_;
        temp += response_to_str_buff(con_handle, bytes_transferred);
        con_handle->response_ = temp;
        string encode_t = get_transfer_encoding(con_handle->response_);
        double time = time_since_epoch();
        vector<string> t_vec = split_string(
                    con_handle->response_.substr(0, con_handle->response_.find("\r\n")),
                    ' ');
        if (t_vec.size() <= 1) {
            close_connection(con_handle);
        }
        else if (encode_t != "" && t_vec[1] == "200" && bytes_transferred != 0) {
            string delim = "\r\n0\r\n";
            auto handler = boost::bind(&ProxyServer::write_http_response, this, con_handle, cache_filename, time, 0);
            boost::asio::async_read_until(con_handle->response_socket_, con_handle->response_buffer_, delim, handler);
        }
        else if (encode_t == "" && t_vec[1] == "200") {
            size_t buff_size = get_http_size(con_handle->response_);
            size_t temp = buff_size - con_handle->response_.substr(con_handle->response_.find("\r\n\r\n") + 4, con_handle->response_.size()).size();
            if (buff_size > temp) buff_size = temp;
            auto handler = boost::bind(&ProxyServer::write_http_response, this, con_handle, cache_filename, time, buff_size);
            boost::asio::async_read(con_handle->response_socket_, con_handle->response_buffer_,
                                    boost::asio::transfer_exactly(buff_size), handler);
        }
        else {
            write_http_response(con_handle, cache_filename, time);
        }
    }
    else {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
        close_connection(con_handle);
    }
}

// Write the HTTP response to the requester
void ProxyServer::write_http_response(con_handle_t con_handle, string const& cache_filename, double time_read, size_t buff_size) {
    try {
        string temp = con_handle->response_;
        string temp_res = response_to_str_buff(con_handle, buff_size);
        for (size_t i = 0; i < temp_res.size(); ++i) temp += temp_res[i];
        con_handle->response_ = temp;
        vector<string> t_vec = split_string(con_handle->response_.substr(0, con_handle->response_.find("\r\n")), ' ');
        double time = time_since_epoch();
        if (debugging_)
            write_to_outputs("<span style=\"color:blue\">DEBUGGING:: </span> Recieved " + std::to_string(con_handle->response_.size()) +
                             " bytes in " + std::to_string(time_since_epoch() - time_read) +
                             " seconds from " + con_handle->response_socket_.remote_endpoint().address().to_string() + "\n");

        auto handler = boost::bind(&ProxyServer::close_http_connection, this, con_handle, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, time);
        boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(con_handle->response_, con_handle->response_.size()), handler);
        if (t_vec.size() > 1 && t_vec[1] == "200") {
            string head = temp.substr(0, temp.find("\r\n\r\n"));
            string body = temp.substr(temp.find("\r\n\r\n") + 4, temp.size());
            vector<unsigned char> binary_file(body.size());
            for (size_t i = 0; i < body.size(); ++i) binary_file[i] = body[i];
            string type = get_type(temp);
            string encode_c = get_content_encoding(con_handle->response_);
            string encode_t = get_transfer_encoding(con_handle->response_);
            cache_manager_->add_file_to_cache(cache_filename, binary_file, type, encode_c, encode_t, get_max_age(temp), get_age(temp));
        }
    }
    catch(boost::system::error_code const& e) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
        close_connection(con_handle);
    }
}

// Properly handle closing connections once write is complete
void ProxyServer::close_http_connection(con_handle_t con_handle, boost::system::error_code const& e, size_t bytes_transferred, double start_time) {
    if (debugging_)
        write_to_outputs("<span style=\"color:blue\">DEBUGGING:: </span> Wrote " + std::to_string(bytes_transferred) +
                         " bytes in " + std::to_string(time_since_epoch() - start_time) +
                         " seconds to " + con_handle->request_socket_.remote_endpoint().address().to_string() + "\n");
    if (e)
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
    close_connection(con_handle);
}

//++++++++++++++++++++++++++ END HTTP ROUTINE

//++++++++++++++++++++++++++ CONNECT ROUTINE

// Begin CONNECT routine by responding to requester with a success message
void ProxyServer::do_connect_routine(con_handle_t con_handle) {
    auto buff = std::make_shared<string>("HTTP/1.1 200 Connection Established\r\n"
                                         "Proxy-agent: QT-Proxy/1.1\r\n\r\n");
    auto handler = boost::bind(&ProxyServer::e2e_connection_handler, this, con_handle, boost::asio::placeholders::error);
    boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(*buff), handler);
}

// Handle bidirectional read and write
void ProxyServer::e2e_connection_handler(con_handle_t con_handle, boost::system::error_code const& e) {
    if (!e && con_handle->request_socket_.is_open() && con_handle->response_socket_.is_open()) {
        auto handler2 = boost::bind(&ProxyServer::write_connect_response, this, con_handle,
                                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        con_handle->response_socket_.async_read_some(boost::asio::buffer(con_handle->response_data_, MAX_BUFF_SIZE), handler2);

        auto handler = boost::bind(&ProxyServer::write_connect_request, this, con_handle,
                                   boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        con_handle->request_socket_.async_read_some(boost::asio::buffer(con_handle->request_data_, MAX_BUFF_SIZE), handler);
    }
    else {
        close_connection(con_handle);
    }
}

// Handle reading from requester
void ProxyServer::read_connect_request(con_handle_t con_handle, boost::system::error_code const& e) {
    if (!e && con_handle->request_socket_.is_open()) {
        auto handler = boost::bind(&ProxyServer::write_connect_request, this, con_handle,
                                   boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        con_handle->request_socket_.async_read_some(boost::asio::buffer(con_handle->request_data_, MAX_BUFF_SIZE), handler);
    }
}

// Handle writing request to remote host
void ProxyServer::write_connect_request(con_handle_t con_handle, boost::system::error_code const& e, size_t bytes_transferred) {
    if (!e && bytes_transferred != 0) {
        auto handler = boost::bind(&ProxyServer::read_connect_request, this, con_handle, boost::asio::placeholders::error);
        boost::asio::async_write(con_handle->response_socket_, boost::asio::buffer(con_handle->request_data_, bytes_transferred), handler);
    }
    else if ((e == boost::asio::error::eof || e == boost::asio::error::connection_reset)
             && bytes_transferred != 0 && con_handle->response_socket_.is_open()) {
        auto handler = boost::bind(&ProxyServer::write_and_close_request, this, con_handle, boost::asio::placeholders::error);
        boost::asio::async_write(con_handle->response_socket_, boost::asio::buffer(con_handle->request_data_, bytes_transferred), handler);
    }
    else if (e) {
        if (e != boost::asio::error::eof && e != boost::asio::error::connection_reset)
            write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
        if (con_handle->response_socket_.is_open()) {
            auto handler = boost::bind(&ProxyServer::write_and_close_request, this, con_handle, boost::asio::placeholders::error);
            boost::asio::async_write(con_handle->response_socket_, boost::asio::buffer(con_handle->request_data_, bytes_transferred), handler);
        }
    }
}

// Handle reading response
void ProxyServer::read_connect_response(con_handle_t con_handle, boost::system::error_code const& e) {
    if (!e && con_handle->response_socket_.is_open()) {
        auto handler = boost::bind(&ProxyServer::write_connect_response, this, con_handle,
                                   boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        con_handle->response_socket_.async_read_some(boost::asio::buffer(con_handle->response_data_, MAX_BUFF_SIZE), handler);
    }
}

// Handle writing response to requester
void ProxyServer::write_connect_response(con_handle_t con_handle, boost::system::error_code const& e, size_t bytes_transferred) {
    if (!e && bytes_transferred != 0) {
        auto handler = boost::bind(&ProxyServer::read_connect_response, this, con_handle, boost::asio::placeholders::error);
        boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(con_handle->response_data_, bytes_transferred), handler);
    }
    else if ((e == boost::asio::error::eof || e == boost::asio::error::connection_reset)
             && bytes_transferred != 0 && con_handle->request_socket_.is_open()) {
        auto handler = boost::bind(&ProxyServer::write_and_close_response, this, con_handle, boost::asio::placeholders::error);
        boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(con_handle->response_data_, bytes_transferred), handler);
    }
    else if (e) {
        if (e != boost::asio::error::eof || e == boost::asio::error::connection_reset)
            write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
        if (con_handle->request_socket_.is_open()) {
            auto handler = boost::bind(&ProxyServer::write_and_close_response, this, con_handle, boost::asio::placeholders::error);
            boost::asio::async_write(con_handle->request_socket_, boost::asio::buffer(con_handle->response_data_, bytes_transferred), handler);
        }
    }
}

// Both of the below functions do the same thing on different sockets
// Once both are finished writing close the sockets
void ProxyServer::write_and_close_request(con_handle_t con_handle, boost::system::error_code const& e) {
    if (e)
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
    try {
        if (debugging_)
            write_to_outputs("<span style=\"color:blue\">DEBUGGING:: </span> Closing Connection! to " + con_handle->request_socket_.remote_endpoint().address().to_string() + "\n");
        con_handle->request_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        con_handle->request_socket_.close();
        if (!con_handle->request_socket_.is_open() && !con_handle->response_socket_.is_open()) {
            write_to_log(con_handle->connect_time_ + ", " + get_current_date_and_time() + ", false, " + con_handle->remote_host_ + "\n");
            m_connections_.erase(con_handle);
        }
    }
    catch (boost::system::error_code const& e2) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e2.message() + " at line #" + std::to_string(__LINE__) + "\n");
    }
    catch (std::exception const& e) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + string(e.what()) + " at line #" + std::to_string(__LINE__) + "\n");
    }
    catch (...) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>Unhandled exception at line #" + std::to_string(__LINE__) + "\n");
    }
}

void ProxyServer::write_and_close_response(con_handle_t con_handle, boost::system::error_code const& e) {
    if (e)
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e.message() + " at line #" + std::to_string(__LINE__) + "\n");
    try {
        if (debugging_)
            write_to_outputs("<span style=\"color:blue\">DEBUGGING:: </span> Closing Connection! to " + con_handle->response_socket_.remote_endpoint().address().to_string() + "\n");
        con_handle->response_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        con_handle->response_socket_.close();
        if (!con_handle->request_socket_.is_open() && !con_handle->response_socket_.is_open()) {
            write_to_log(con_handle->connect_time_ + ", " + get_current_date_and_time() + ", false, " + con_handle->remote_host_ + "\n");
            m_connections_.erase(con_handle);
        }
    }
    catch (boost::system::error_code const& e2) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + e2.message() + " at line #" + std::to_string(__LINE__) + "\n");
    }
    catch (std::exception const& e) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>" + string(e.what()) + " at line #" + std::to_string(__LINE__) + "\n");
    }
    catch (...) {
        write_to_outputs("<span style=\"color:red\">!!ERROR!! </span>Unhandled exception at line #" + std::to_string(__LINE__) + "\n");
    }
}

//++++++++++++++++++++++++++ END CONNECT ROUTINE
