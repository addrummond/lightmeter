#!/bin/sh

python process_strings.py strings_english &&
gcc -c -DTEST -I../ -I./ -I../bitmaps menu_strings.c -o menu_strings_test.out &&
gcc -c -DTEST -I../ -I./ -I../bitmaps menu_strings_table.c -o menu_strings_table_test.out &&
gcc -c -DTEST -I../ -I../bitmaps ../bitmaps/bitmaps.c -o bitmaps.out &&
gcc bitmaps.out menu_strings_table_test.out menu_strings_test.out -o mtest
