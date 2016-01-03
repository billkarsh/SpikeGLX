
RESOURCES += \
    $$PWD/CommonResources.qrc \
    $$PWD/QLED.qrc

win32 {
    RESOURCES += $$PWD/WindowsResources.qrc
    RC_FILE   += $$PWD/WindowsResources/WinResources.rc
}

macx {
    ICON = $$PWD/MacResources/SpikeGL.icns
}


