#!/bin/sh

TEMPDIR=`mktemp -d /tmp/sendscan.XXXXXXXXXX`
CWD=$PWD

MODE=`echo $1 | tr A-Z a-z`
RES=$2
ADF=`echo $3 | tr A-Z a-z`
RCPT=$4

cd $TEMPDIR

if [ "X$ADF" == "Xadf" ] ; then
	scanimage -y 297.0 -x 210.0 --batch --format=tiff --mode $MODE --resolution $RES --source ADF
	tiffcp out*.tif output.tif
	tiff2pdf -z output.tif > output.pdf
else
	scanimage -y 297.0 -x 210.0 --batch --format=tiff --mode $MODE --resolution $RES --batch-count 1
	tiff2pdf -z out1.tif > output.pdf
fi

echo | mutt -s "Scanned Document" -a output.pdf $RCPT

ls -lh

rm -f out*.tif output.pdf

cd $CWD
rmdir $TEMPDIR
