#!/bin/sh

RCDIR=./rcs/
INPROGRESSDIR=./rcs/running
TTYRECDIR=./rcs/ttyrecs/$1
DEFAULT_RC=../settings/init.txt
PLAYERNAME=$1
SCRIPTSDIR=./rcs/${PLAYERNAME}.scripts

mkdir -p $RCDIR
mkdir -p $INPROGRESSDIR
mkdir -p $TTYRECDIR
mkdir -p $SCRIPTSDIR

if [ ! -f ${RCDIR}/${PLAYERNAME}.rc ]; then
    cp ${DEFAULT_RC} ${RCDIR}/${PLAYERNAME}.rc
fi
