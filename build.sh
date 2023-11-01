#gcc src/global_state.c src/main.c -Llib -l:libraylib.a -lm -Iinclude -Isrc -o main
g++ -o main src/*.cpp -Wall -g -rdynamic -Llib -l:libraylib.a -lm -Iinclude -Isrc
