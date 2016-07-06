TARGET = RLIDisplay
TEMPLATE = app

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# using GDAL library
win32:QMAKE_LIBDIR += C:/GDAL/lib
unix:QMAKE_LIBDIR += /usr/local/lib/

win32:INCLUDEPATH += C:/GDAL/include
unix:INCLUDEPATH += /usr/local/include/

unix:LIBS += -lgdal
win32:LIBS += -lgdal_i -lgeos_i

SOURCES += main.cpp\
        mainwindow.cpp \
    rlicontrolwidget.cpp \
    rlidisplaywidget.cpp \
    asmfonts.cpp \
    radardatasource.cpp \
    radarengine.cpp \
    infoengine.cpp \
    infocontrollers.cpp \
    rlicontrolevent.cpp \
    maskengine.cpp \
    controlsengine.cpp \
    menuengine.cpp

HEADERS  += mainwindow.h \
    rlicontrolwidget.h \
    rlidisplaywidget.h \
    asmfonts.h \
    radardatasource.h \
    radarengine.h \
    xpmon_be.h \
    infoengine.h \
    infocontrollers.h \
    rlicontrolevent.h \
    maskengine.h \
    controlsengine.h \
    menuengine.h

FORMS    += mainwindow.ui \
    rlicontrolwidget.ui

RESOURCES += \
    icons.qrc \
    fonts.qrc \
    shaders.qrc

OTHER_FILES +=
