rm upload/challenge01/challenge01.tar.gz
cp nush.c upload/challenge01/
cp svec.c upload/challenge01/
cp svec.h upload/challenge01/
cp Makefile upload/challenge01/
cp tokenize.c upload/challenge01/
cp tokenize.h upload/challenge01/
cd upload/challenge01
rm challenge01.tar.gz
gtar -czvf  challenge01.tar.gz --exclude="._[^/]*" .
rm *.c
rm *.h
rm Makefile

echo "Files compressed!"
