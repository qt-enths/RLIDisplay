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

SOURCES += main.cpp \
    mainwindow.cpp \
    rlicontrolwidget.cpp \
    rlidisplaywidget.cpp \
    layers/asmfonts.cpp \
    layers/radarengine.cpp \
    layers/infoengine.cpp \
    datasources/infocontrollers.cpp \
    layers/maskengine.cpp \
    layers/controlsengine.cpp \
    layers/menuengine.cpp \
    s52/chartmanager.cpp \
    s52/s52assets.cpp \
    s52/s52chart.cpp \
    s52/s52references.cpp \
    common/triangulate.cpp \
    layers/chartengine.cpp \
    layers/chartlayers.cpp \
    layers/chartshaders.cpp \
    layers/targetengine.cpp \
    common/rlimath.cpp \
    layers/routeengine.cpp \
    datasources/nmeadata.cpp \
    datasources/nmeaprocessor.cpp \
    datasources/radardatasource.cpp \
    datasources/radarscale.cpp \
    datasources/targetdatasource.cpp \
    datasources/shipdatasource.cpp \
    datasources/boardpultcontroller.cpp \
    layers/magnifierengine.cpp

HEADERS  += mainwindow.h \
    rlicontrolwidget.h \
    rlidisplaywidget.h \
    layers/asmfonts.h \
    layers/radarengine.h \
    layers/infoengine.h \
    datasources/infocontrollers.h \
    layers/maskengine.h \
    layers/controlsengine.h \
    layers/menuengine.h \
    s52/chartmanager.h \
    s52/s52assets.h \
    s52/s52chart.h \
    s52/s52references.h \
    common/triangulate.h \
    layers/chartengine.h \
    layers/chartlayers.h \
    layers/chartshaders.h \
    datasources/apctrl.h \
    common/rlistrings.h \
    layers/targetengine.h \
    common/rlimath.h \
    layers/routeengine.h \
    datasources/xpmon_be.h \
    datasources/nmeadata.h \
    datasources/nmeaprocessor.h \
    datasources/radarscale.h \
    datasources/radardatasource.h \
    datasources/targetdatasource.h \
    datasources/shipdatasource.h \
    datasources/boardpultcontroller.h \
    layers/magnifierengine.h


FORMS    += mainwindow.ui \
    rlicontrolwidget.ui

RESOURCES += \
    icons.qrc \
    fonts.qrc \
    shaders.qrc \
    cursors.qrc

OTHER_FILES +=
