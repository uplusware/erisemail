#!/bin/sh
find . -name "*.[Hh]" -o -name "*.[hH][pP][pP]" -o -name "*.[Cc]" -o -name "*.[Cc][Pp][Pp]" > cscope.files
cscope -bkq -i cscope.files
ctags -R --c++-kinds=+p --fields=+iaS --extra=+q
