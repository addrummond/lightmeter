#!/bin/sh

cd ../bitmaps
python process_bitmaps.py output
cd ../menus

python process_strings.py strings_english &&
gcc -c -DTEST -I../ menu_strings.c -o menu_strings_test.out &&
gcc -c -DTEST -I../ menu_strings_table.c -o menu_strings_table_test.out &&
gcc -c -DTEST -I../ ../bitmaps/bitmaps.c -o bitmaps.out &&
gcc bitmaps.out menu_strings_table_test.out menu_strings_test.out -o mtest
