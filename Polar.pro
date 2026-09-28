QT += core gui uitools
lessThan(QT_MAJOR_VERSION, 6): error("Polar requires Qt 6")

QT += widgets

CONFIG += c++20

# Compiler-specific size/link-time optimizations; do not pass MSVC flags to GCC/Clang.
win32-msvc {
    QMAKE_CXXFLAGS_RELEASE += /GL /Gy /O1
    QMAKE_LFLAGS_RELEASE += /LTCG /OPT:REF /OPT:ICF
}
QT += network charts
RC_ICONS = resources/images/appico.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RC_ICONS = resources/images/appico.ico

RESOURCES += resources.qrc

SOURCES += \
    src/network/functb.cpp \
    src/core/gameplatform.cpp \
    src/ui/leaderboard.cpp \
    src/app/main.cpp \
    src/ui/mainwindow.cpp \
    src/ui/render.cpp \
    src/network/updater.cpp \
    src/core/appsettings.cpp

HEADERS += \
    src/network/functb.h \
    src/core/gameplatform.h \
    src/ui/leaderboard.h \
    src/ui/mainwindow.h \
    src/ui/render.h \
    src/network/updater.h \
    src/core/appsettings.h

FORMS += \
    ui/classement.ui \
    ui/mainwindow.ui \
    ui/options.ui

TRANSLATIONS += \
    translations/Polar_fr_FR.ts \
    translations/Polar_en_US.ts \
    translations/Polar_es_ES.ts \
    translations/Polar_it_IT.ts

CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += src/core src/network src/ui

HEADERS += src/core/wtdata.h src/core/performanceanalysis.h src/network/wtapi.h src/ui/editionpicker.h
SOURCES += src/core/wtdata.cpp src/core/performanceanalysis.cpp src/network/wtapi.cpp src/ui/editionpicker.cpp
HEADERS += src/ui/raceanalysisdialog.h
SOURCES += src/ui/raceanalysisdialog.cpp
