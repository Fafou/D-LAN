# -------------------------------------------------
# Project created by QtCreator 2009-10-04T02:24:09
# -------------------------------------------------
QT += testlib
QT -= gui
TARGET = Tests

CONFIG += link_prl

include(../common.pri)
include(../../Libs/protobuf.pri)

LIBS += -L"../output/$$FOLDER" -lCommon
POST_TARGETDEPS += ../output/$$FOLDER/libCommon.a

INCLUDEPATH += . \
   .. \
   ../..

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    Tests.cpp \
    ../../Protos/common.pb.cc \
    ../../Protos/core_settings.pb.cc
HEADERS += Tests.h