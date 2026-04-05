QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    interface/mainwindow.cpp \
    libs/geo/geo.cpp \
    libs/json/json.cpp \
    libs/svg/svg.cpp \
    main.cpp \
    transport-catalogue/json_reader.cpp \
    transport-catalogue/map_renderer.cpp \
    transport-catalogue/request_handler.cpp \
    transport-catalogue/transport_catalogue.cpp

HEADERS += \
    interface/mainwindow.h \
    libs/geo/geo.h \
    libs/json/json.h \
    libs/svg/svg.h \
    transport-catalogue/domain.h \
    transport-catalogue/json_reader.h \
    transport-catalogue/map_renderer.h \
    transport-catalogue/request_handler.h \
    transport-catalogue/transport_catalogue.h

FORMS += \
    interface/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=
