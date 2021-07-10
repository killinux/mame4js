all:  linenoise_example linenoise_utf8_example linenoise_cpp_example

linenoise_example: linenoise.h linenoise.c example.c
	$(CC) -Wall -W -O0 -g4 --source-map-base http://121.5.59.140/mame/mame/ -o $@ linenoise.c example.c

linenoise_utf8_example: linenoise.c utf8.c example.c
	$(CC) -DNO_COMPLETION -DUSE_UTF8 -Wall -W -O0 -g4 --source-map-base http://121.5.59.140/mame/mame/ -o $@ linenoise.c utf8.c example.c

linenoise_cpp_example: linenoise.h linenoise.c
	g++ -Wall -W -O0 -g4 --source-map-base http://121.5.59.140/mame/mame/ -o $@ linenoise.c example.c

clean:
	rm -f linenoise_example linenoise_utf8_example linenoise_cpp_example *.o
