# Binary EYE

This repository was created from original [CVS
repository](https://sourceforge.net/projects/beye/) of BEYE program

[![Build Status](https://travis-ci.org/widgetii/beye.svg?branch=master)](https://travis-ci.org/widgetii/beye)

![Screenshot from Wikipedia](https://upload.wikimedia.org/wikipedia/commons/b/b6/Biew_ss.png)

TODO:

    - [X] replace hand-written configure to Autoconf tools
    - [ ] make tests in Linux32/64, Darwin, Win32/64, Raspbian, NetBSD 
    - [ ] find bugs which lead to crashes

Building Beye:

    o   If you are using a Git checkout, you need to run the
        bootstrap script in order to generate configure. This is not
        necessary for official tarballs.

    o   Run configure the make. Useful configure flags are:

        --enable-ncurses: support for the ncurses library
        --enable-slang: support for the SLang library
        --enable-iconv: support for the Iconv library

