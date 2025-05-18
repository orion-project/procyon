# These values are updated by make_version.py

APP_VER_MAJOR=1
APP_VER_MINOR=0
APP_VER_PATCH=0
APP_VER_CODENAME=dev
APP_VER_YEAR=2024
APP_VER=1.0.0-dev

DEFINES += "APP_VER_MAJOR=$$APP_VER_MAJOR"
DEFINES += "APP_VER_MINOR=$$APP_VER_MINOR"
DEFINES += "APP_VER_PATCH=$$APP_VER_PATCH"
DEFINES += "APP_VER_CODENAME=\"\\\"$$APP_VER_CODENAME\\\"\""
DEFINES += "APP_VER_YEAR=$$APP_VER_YEAR"
DEFINES += "APP_VER=\"\\\"$$APP_VER\\\"\""

win32 {
    DEFINES += "BUILDDATE=\"\\\"$$system(date /T)\\\"\""
    DEFINES += "BUILDTIME=\"\\\"$$system(time /T)\\\"\""
}
else {
    DEFINES += "BUILDDATE=\"\\\"$$system(date '+%F')\\\"\""
    DEFINES += "BUILDTIME=\"\\\"$$system(date '+%T')\\\"\""
}
