////////////////////////////////////////////////////////
///
///     "ioServicThread.h"
///
///     Class declaration for IOServiceThread
///     This class runs the Boost io_service on a
///     separate thread from the GUI. This allows
///     the Boost and QT libraries to run concurrently
///     without issue.
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#ifndef IOSERVICETHREAD_H
#define IOSERVICETHREAD_H

#include <QThread>
#include <QTextEdit>
#include <QMutex>
#include <boost/asio.hpp>
#include <string>

class IOServiceThread : public QThread {
public:
    IOServiceThread(boost::asio::io_service* service) : QThread(), io_service_(service) {}
    void run() override;

private:
    Q_OBJECT
    boost::asio::io_service *io_service_;
};
#endif // IOSERVICETHREAD_H
