build\program.exe: build\main.o build\Stack.o
		g++ -o build\Stack.exe $^
build\main.o: source\main.cpp
		g++ -c -o build\main.o source\main.cpp
build\Stack.o: source\Stack.cpp
		g++ -c -o build\Stack.o source\Stack.cpp