#!/bin/sh

TAXSOLVEDIR=${PWD}/`dirname $0`

if [ ! -d ${TAXSOLVEDIR}/../bin ] ; then
   mkdir ${TAXSOLVEDIR}/../bin
fi

cd ${TAXSOLVEDIR}/Gui_gtk/
   make 

cd  "${TAXSOLVEDIR}"
make 

