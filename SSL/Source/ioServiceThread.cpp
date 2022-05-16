////////////////////////////////////////////////////////
///
///     "ioServiceThread.cpp"
///
///     Implementation of function defined in
///     ioServiceThread.h
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#include "ioServiceThread.h"

// Starts the IO Service associated with the thread
void IOServiceThread::run() {
    io_service_->run();
}
