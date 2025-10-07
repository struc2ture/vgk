CFLAGS  = -g -Werror -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable
CFLAGS += -I/opt/homebrew/include
LFLAGS  =
LFLAGS += -L/opt/homebrew/lib -lglfw

run: bin/main
	lldb bin/main -o r

run_tests: bin/tests
	lldb bin/tests -o r

bin/main: src/main.cpp
	clang $(CFLAGS) src/main.cpp src/common/common.cpp -o bin/main $(LFLAGS)

bin/tests: src/tests.cpp
	clang $(CFLAGS) src/tests.cpp src/common/common.cpp -o bin/tests $(LFLAGS)
