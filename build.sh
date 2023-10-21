#gcc src/global_state.c src/main.c -Llib -l:libraylib.a -lm -Iinclude -Isrc -o main
gcc src/*.c -Llib -l:libraylib.a -lm -l:libyaml-0.so.2 -Iinclude -Isrc -o main
