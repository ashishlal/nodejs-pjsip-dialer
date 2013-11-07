#!/bin/bash
PJSIP_DIR="/home/ashish/Downloads/pjproject-2.1.0"
PJSIP_INCL="$PJSIP_DIR/pjsip/include"
PJLIB_INCL="$PJSIP_DIR/pjlib/include"
PJNATH_INCL="$PJSIP_DIR/pjnath/include"
PJLIB_UTIL_INCL="$PJSIP_DIR/pjlib-util/include"
PJMEDIA_INCL="$PJSIP_DIR/pjmedia/include"
rm -rf *.o
rm -rf *.a
g++ -std=c++0x -c -Wall -fPIC PJSIPDll.cc -I$PJSIP_INCL -I$PJLIB_INCL -I$PJNATH_INCL -I$PJLIB_UTIL_INCL -I$PJMEDIA_INCL
ar rvs libPJSIP.a PJSIPDll.o
