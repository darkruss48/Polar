QT       += core gui uitools

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

QMAKE_CFLAGS_RELEASE -= -MD
QMAKE_CFLAGS_DEBUG   -= -MDd
QMAKE_CXXFLAGS_RELEASE -= -MD
QMAKE_CXXFLAGS_DEBUG   -= -MDd

# optimisation globale (LTO)
QMAKE_CXXFLAGS_RELEASE += /GL
QMAKE_CFLAGS_RELEASE     += /GL

# optimisation de l'éditeur de liens : supprimer code non utilisé et combiner identiques
QMAKE_LFLAGS_RELEASE += /LTCG /OPT:REF /OPT:ICF

# fonction-level linking
QMAKE_CXXFLAGS_RELEASE += /Gy
QMAKE_CFLAGS_RELEASE     += /Gy
QMAKE_CXXFLAGS_RELEASE += /O1         # optimise pour la taille (au lieu de /O2)
QMAKE_CFLAGS_RELEASE   += /O1






# JE VEUX UN STANDALONE --> RAJOUTER CETTE LIGNE POUR LINKER DE MANIERE STATIQUE ET NON DYNAMIQUE
CONFIG += static
QT += core
QT += network
QT += charts
RC_ICONS = appico.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RC_ICONS = appico.ico

RESOURCES += resources.qrc

SOURCES += \
    functb.cpp \
    gameplatform.cpp \
    leaderboard.cpp \
    main.cpp \
    mainwindow.cpp \
    render.cpp \
    updater.cpp \
    appsettings.cpp

HEADERS += \
    functb.h \
    gameplatform.h \
    leaderboard.h \
    mainwindow.h \
    render.h \
    updater.h \
    appsettings.h

FORMS += \
    classement.ui \
    mainwindow.ui \
    options.ui

TRANSLATIONS += \
    Polar_fr_FR.ts \
    Polar_en_US.ts \
    Polar_es_ES.ts \
    Polar_it_IT.ts

CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
