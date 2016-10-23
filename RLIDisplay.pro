TARGET = RLIDisplay
TEMPLATE = app

QT       += core gui opengl concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# using GDAL library
win32:QMAKE_LIBDIR += C:/GDAL/lib
unix:QMAKE_LIBDIR += /usr/local/lib/

unix:QMAKE_CXXFLAGS += -Wno-write-strings
unix:QMAKE_CXXFLAGS += -Wno-unused-variable

win32:INCLUDEPATH += C:/GDAL/include
unix:INCLUDEPATH += /usr/local/include/

unix:LIBS += -lgdal  -lrt
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
    menuengine.cpp \
    chartmanager.cpp \
    s52assets.cpp \
    s52chart.cpp \
    s52references.cpp \
    triangulate.cpp \
    chartengine.cpp \
    chartlayers.cpp \
    chartshaders.cpp \
    radarscale.cpp \
    targetengine.cpp \
    targetdatasource.cpp \
    nmeadata.cpp \
    nmeaprocessor.cpp \
    rlimath.cpp \
    shipdatasource.cpp

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
    menuengine.h \
    chartmanager.h \
    s52assets.h \
    s52chart.h \
    s52references.h \
    triangulate.h \
    chartengine.h \
    chartlayers.h \
    chartshaders.h \
    apctrl.h \
    rlistrings.h \
    radarscale.h \
    targetengine.h \
    targetdatasource.h \
    nmeadata.h \
    nmeaprocessor.h \
    rlimath.h \
    shipdatasource.h

FORMS    += mainwindow.ui \
    rlicontrolwidget.ui

RESOURCES += \
    icons.qrc \
    fonts.qrc \
    shaders.qrc \
    cursors.qrc

OTHER_FILES +=
