////////////////////////////////////////////////////////
///
///     "servercontroller.cpp"
///
///     Implementation of functions defined in
///     servercontroller.h
///     Initializes GUI
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#include "servercontroller.h"
#include "ui_servercontroller.h"

// Default constructor that initializes member vars
ServerController::ServerController(QWidget *parent) : QMainWindow(parent), ui(new Ui::ServerController) {
    ui->setupUi(this);
    output_console_ = findChild<QTextEdit*>("ConsoleOutputBroswer");
    output_console_->setReadOnly(true);
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> Initializing control panel..."));
    server_control_ = findChild<QPushButton*>("ServerControl");
    debugging_input_ = findChild<QCheckBox*>("DebuggingInput");
    logging_input_ = findChild<QCheckBox*>("LoggingInput");
    port_num_input_ = findChild<QLineEdit*>("PortNumInput");
    cache_size_input = findChild<QLineEdit*>("CacheSizeInput");

    req_num_ = findChild<QLabel*>("req_num");
    req_bytes_ = findChild<QLabel*>("bytes_num");
    cache_hits_ = findChild<QLabel*>("cache_hits");
    cache_bytes_ = findChild<QLabel*>("cache_bytes_num");

    logging_input_->setCheckState(Qt::Checked);
    connect(server_control_, SIGNAL(clicked()), this, SLOT(start_server()));

    findChild<QLabel*>("OptionIcon")->pixmap().scaled(Qt::KeepAspectRatio, Qt::FastTransformation);
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> Finished initializing"));
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> Welcome to Proxy Server 1.0!"));
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> This server works on the back of two libraries"));
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> &emsp; - Boost, for the server and its ASIO libraries"));
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> &emsp; - QT, for the GUI and threading"));
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> &emsp; - SQLite, for tracking the cache"));
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> Use it for anything that's not illegal"));
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> Find the source on Github at the link below:"));
    write_to_console(QString::fromStdString("<span style='color:grey'>[" + get_current_date_and_time() + "]</span> ~ <span style='color:blue;'> MAIN:: </span> https://github.com/jjgrayg/Proxy-Server<br>"));
}

// Destructor for GUI
ServerController::~ServerController() {
    delete ui;
}

// Handle starting the server
void ServerController::start_server() {
    unsigned int port_num = get_port_num();
    unsigned int cache_size = get_cache_size_in_bytes();
    bool debugging = get_debugging();
    bool logging = get_logging();

    server_control_->disconnect();
    connect(server_control_, SIGNAL(clicked()), this, SLOT(stop_server()));
    server_control_->setText("Stop Server");
    debugging_input_->setEnabled(false);
    logging_input_->setEnabled(false);
    port_num_input_->setEnabled(false);
    cache_size_input->setEnabled(false);
    srv_ = new ProxyServer(cache_size, port_num, logging, debugging, "QT Server");
    connect(srv_, SIGNAL(ready_write_console(QString)), this, SLOT(write_to_console(QString)));
    connect(srv_, SIGNAL(new_req(size_t)), this, SLOT(increment_num_requests(size_t)));
    connect(srv_, SIGNAL(cache_hit(size_t)), this, SLOT(increment_cache_hits(size_t)));
    connect(srv_, SIGNAL(add_cache_bytes(size_t)), this, SLOT(add_cache_bytes(size_t)));
    connect(srv_, SIGNAL(add_req_bytes(size_t)), this, SLOT(add_request_bytes(size_t)));
    try {
        srv_->start();
    }
    catch (boost::system::error_code e) {
        stop_server();
        write_to_console(QString::fromStdString("<span><span style=\"color:grey\">[" + get_current_date_and_time() + "]</span> ~ <span style=\"color:green\">SERVER</span>:: <span style=\"color:red\">!!ERROR!! </span>" + e.message() + " line #" + std::to_string(__LINE__) + "\n"));
        qDebug() << QString::fromStdString(e.message() + " line #" + std::to_string(__LINE__) + "\n");
    }
}

// Handle stopping the server
void ServerController::stop_server() {
    try {
        srv_->stop();
    }
    catch (boost::system::error_code e) {
        write_to_console(QString::fromStdString("<span><span style=\"color:grey\">[" + get_current_date_and_time() + "]</span> ~ <span style=\"color:green\">SERVER</span>:: <span style=\"color:red\">!!ERROR!! </span>" + e.message() + " line #" + std::to_string(__LINE__) + "\n"));
        qDebug() << QString::fromStdString(e.message() + " line #" + std::to_string(__LINE__) + "\n");
    }
    server_control_->disconnect();
    connect(server_control_, SIGNAL(clicked()), this, SLOT(start_server()));
    server_control_->setText("Start Server");
    debugging_input_->setEnabled(true);
    logging_input_->setEnabled(true);
    port_num_input_->setEnabled(true);
    cache_size_input->setEnabled(true);
}

// Read portnum input from GUI
unsigned int ServerController::get_port_num() {
    return std::stoul(port_num_input_->text().toStdString());
}

// Read cache size from GUI
unsigned int ServerController::get_cache_size_in_bytes() {
    return std::stoul(cache_size_input->text().toStdString()) * 1024 * 1024;
}

// SLOT to handle appending message to console
void ServerController::write_to_console(QString to_insert) {
    output_console_->append(to_insert);
}

// SLOT to handle setting cache hits
void ServerController::increment_cache_hits(size_t size) {
    cache_hits_->setText(QString::number(size));
}

// SLOT to handle setting number of requests
void ServerController::increment_num_requests(size_t size) {
    req_num_->setText(QString::number(size));
}

// SLOT to handle setting cache hit bytes
void ServerController::add_cache_bytes(size_t size) {
    cache_bytes_->setText(QString::number(size));
}

// SLOT to handle setting request bytes
void ServerController::add_request_bytes(size_t size) {
    req_bytes_->setText(QString::number(size));
}

