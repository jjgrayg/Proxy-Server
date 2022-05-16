////////////////////////////////////////////////////////
///
///     "servercontroller.h"
///
///     Class declaration for ServerController
///     This is the GUI and handler for starting and
///     stopping the proxy server
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QMainWindow>
#include "ui_servercontroller.h"
#include "proxyserver.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ServerController; }
QT_END_NAMESPACE

class ServerController : public QMainWindow {
public:
    ServerController(QWidget *parent = nullptr);
    ~ServerController();

private:
    Q_OBJECT
    unsigned int get_port_num();
    unsigned int get_cache_size_in_bytes();
    bool get_debugging() { return debugging_input_->isChecked(); }
    bool get_logging() { return logging_input_->isChecked(); }

    Ui::ServerController *ui;
    ProxyServer *srv_;
    QDebug *debug_;

    QPushButton *server_control_;
    QCheckBox *debugging_input_, *logging_input_;
    QLineEdit *port_num_input_, *cache_size_input;
    QTextEdit *output_console_;
    QLabel *req_num_, *req_bytes_, *cache_hits_, *cache_bytes_;

private slots:

    void start_server();
    void stop_server();
    void write_to_console(QString);
    void increment_cache_hits(size_t);
    void increment_num_requests(size_t);
    void add_cache_bytes(size_t);
    void add_request_bytes(size_t);
};
#endif // SERVERCONTROLLER_H
