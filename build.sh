#gcc src/global_state.c src/main.c -Llib -l:libraylib.a -lm -Iinclude -Isrc -o main
gcc -o main src/*.c src/data_handling/*.c -Wall -ggdb -Llib -l:libraylib.a -lm -Iinclude -Isrc -Isrc/data_handling
