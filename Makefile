CFLAGS  = -g -Werror -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable
CFLAGS += -I/opt/homebrew/include -I/usr/local/include
LFLAGS  =
LFLAGS += -L/opt/homebrew/lib -lglfw
LFLAGS += -L/usr/local/lib -lvulkan

export VK_ICD_FILENAMES = /usr/local/share/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH = /usr/local/share/vulkan/explicit_layer.d
export DYLD_LIBRARY_PATH = /usr/local/lib:$$DYLD_LIBRARY_PATH

run: bin/main
	lldb bin/main -o r

bin/main: src/main.cpp src/vgk.cpp src/vgk.hpp
	clang $(CFLAGS) src/main.cpp src/common/common.cpp src/vgk.cpp -o bin/main $(LFLAGS)
