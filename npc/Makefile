# 定义顶层模块名和约束文件路径
TOPNAME = top
NXDC_FILES = constr/top.nxdc
INC_PATH ?=

# 1. 自动抓取 vsrc 和 csrc 目录下的所有源文件 (解决找不到 scpu.v 的问题)
VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")

# 2. NVBoard 路径和依赖配置
BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

# 自动调用脚本生成引脚绑定 C++ 文件 (auto_bind.cpp)
SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	mkdir -p $(BUILD_DIR)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

CSRCS += $(SRC_AUTO_BIND)

# 引入 NVBoard 官方编译规则
include $(NVBOARD_HOME)/scripts/nvboard.mk

# C++ 头文件和链接选项 (加载 SDL2 用于图形界面)
INCFLAGS = $(addprefix -I, $(INC_PATH))
CFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""
LDFLAGS += -lSDL2 -lSDL2_image

# 3. 核心编译与仿真规则
sim: $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@rm -rf $(OBJ_DIR)
	@mkdir -p $(BUILD_DIR)
	verilator -Wall --cc --exe --build --trace \
		--top-module $(TOPNAME) \
		-I$(abspath ./vsrc) \
		$(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE) \
		$(addprefix -CFLAGS , $(CFLAGS)) \
		$(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) -o $(abspath $(BIN))
	@# 自动将机器码文件拷贝到可执行文件同目录，避免相对路径导致读取失败
	@cp vsrc/inst.txt $(BUILD_DIR)/ 2>/dev/null || cp inst.txt $(BUILD_DIR)/ 2>/dev/null || true
	@$(BIN)

wave:
	@gtkwave waveform.vcd &

clean:
	@rm -rf $(BUILD_DIR) obj_dir waveform.vcd

include ../Makefile
