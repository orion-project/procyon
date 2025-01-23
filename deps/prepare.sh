#!/bin/bash

HUNSPELL_VER=v1.7.0
HOEDOWN_VER=3.0.7

# https://misc.flogisoft.com/bash/tip_colors_and_formatting
COLOR_HEADER=$"\033[95m"
COLOR_OKBLUE=$"\033[94m"
COLOR_OKGREEN=$"\033[92m"
COLOR_WARNING=$"\033[93m"
COLOR_FAIL=$"\033[91m"
COLOR_BOLD=$"\033[1m"
COLOR_UNDERLINE=$"\033[4m"
COLOR_RESET=$"\033[0m"

print_header() {
    echo
    echo -e "${COLOR_HEADER}$1${COLOR_RESET}"
}

print_error() {
    echo -e "${COLOR_FAIL}$1${COLOR_RESET}"
}

print_done() {
    echo
    echo -e "${COLOR_OKGREEN}Done${COLOR_RESET}"
    echo
}

exit_if_error() {
    if [ $? -ne 0 ]; then
        print_error "$1"
        exit
    fi
}

clone_hunspell() {
    print_header "Preparing hunspell repo..."
    git clone https://github.com/hunspell/hunspell
    exit_if_error "Failed to clone hunspell repo"

    cd hunspell
    git checkout $HUNSPELL_VER
    exit_if_error "Failed to switch to hunspell $HUNSPELL_VER"
    cd ..
}

download_dicts() {
    print_header "Downloading hunspell disctionaries..."
    LIBRE=libreoffice-6.3.0.4
    DICTS_DIR=../bin/dicts
    if [ ! -f $LIBRE.zip ]; then
        curl https://codeload.github.com/LibreOffice/dictionaries/zip/$LIBRE > $LIBRE.zip
    fi
    if [ ! -f $DICTS_DIR ]; then mkdir $DICTS_DIR; fi
    unzip -j $LIBRE.zip dictionaries-$LIBRE/en/en_US.* -d $DICTS_DIR
    #unzip -j $LIBRE.zip dictionaries-$LIBRE/ru_RU/ru_RU.* -d $DICTS_DIR
    unzip -j $LIBRE.zip dictionaries-$LIBRE/ru_RU/ru_RU.* -d .
    python3 convert_ru_dict.py
}

clone_hoedown() {
    print_header "Preparing hoedown repo..."
    git clone https://github.com/hoedown/hoedown
    exit_if_error "Failed to clone hoedown repo"
    cd hoedown
    git checkout $HOEDOWN_VER
    exit_if_error "Failed to switch to hoedown $HOEDOWN_VER"
    cd ..
}

clone_hunspell
download_dicts
clone_hoedown
print_done
