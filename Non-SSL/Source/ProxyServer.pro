QT       += core gui uitools

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cachemanager.cpp \
    ioServiceThread.cpp \
    servercontroller.cpp \
    main.cpp \
    proxyserver.cpp \
    utilities.cpp \
    ./sqlite/sqlite3.c

HEADERS += \
    cachemanager.h \
    connection.h \
    ioServiceThread.h \
    servercontroller.h \
    proxyserver.h \
    utilities.h \
    ./sqlite/sqlite3.h

FORMS += \
    servercontroller.ui

INCLUDEPATH += \
    ./sqlite \
    C:/ProgrammingLibs/boost_1_78_0 # Change to Boost root directory


LIBS += \
    -lwsock32 \
    -lws2_32 \
    -l:libboost_thread-mgw11-mt-x64-1_78.a \ # Change to compiled Boost thread lib
    -l:libboost_filesystem-mgw11-mt-x64-1_78.a \ # Change to compiled Boost filesystem lib
    -LC:\ProgrammingLibs\boost_1_78_0\stage\lib # Change to compiled Boost libs location

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
