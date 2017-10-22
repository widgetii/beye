This repository was created from original CVS repository of BEYE program in https://sourceforge.net/projects/beye/

TODO:
    1) replace hand-written configure to Autoconf tools
    2) make tests in Linux32/64, Darwin, Win32/64, Raspbian, NetBSD 
    3) find bugs which lead to crashes

Building Beye:

    o   If you are using a Git checkout, you need to run the
        bootstrap script in order to generate configure. This is not
        necessary for official tarballs.

    o   Run configure the make. Useful configure flags are:

        --enable-ncurses: support for the ncurses library
        --enable-slang: support for the SLang library
        --enable-iconv: support for the Iconv library


