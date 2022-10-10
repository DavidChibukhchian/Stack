build\program.exe: build\main.o build\stack.o
		g++ -o build\Stack.exe $^
build\main.o: source\main.cpp
		g++ -c -o build\main.o source\main.cpp
build\stack.o: source\Stack.cpp
		g++ -c -o build\stack.o source\stack.cpp
