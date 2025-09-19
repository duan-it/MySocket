# Socket 学习项目 Makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -Iinclude
SRCDIR = src
INCDIR = include
TESTDIR = tests
EXAMPLEDIR = examples
OBJDIR = obj
BINDIR = bin

# 源文件
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# 测试文件
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.c=$(OBJDIR)/%.o)
TEST_BINARIES = $(TEST_SOURCES:$(TESTDIR)/%.c=$(BINDIR)/%)

# 示例文件
EXAMPLE_SOURCES = $(wildcard $(EXAMPLEDIR)/*.c)
EXAMPLE_BINARIES = $(EXAMPLE_SOURCES:$(EXAMPLEDIR)/%.c=$(BINDIR)/%)

# 主要目标
all: directories libmysocket tests examples

# 创建目录
directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

# 编译静态库
libmysocket: $(OBJECTS)
	ar rcs $(BINDIR)/libmysocket.a $(OBJECTS)
	@echo "静态库 libmysocket.a 创建完成"

# 编译目标文件
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(TESTDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(EXAMPLEDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 编译测试程序
tests: $(TEST_BINARIES)

$(BINDIR)/%: $(OBJDIR)/%.o libmysocket
	$(CC) $(CFLAGS) $< -L$(BINDIR) -lmysocket -o $@

# 编译示例程序
examples: $(EXAMPLE_BINARIES)

$(BINDIR)/%: $(OBJDIR)/%.o libmysocket
	$(CC) $(CFLAGS) $< -L$(BINDIR) -lmysocket -o $@

# 运行测试
test: tests
	@echo "运行测试..."
	@for test in $(TEST_BINARIES); do \
		echo "运行 $$test"; \
		./$$test; \
	done

# 清理
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "清理完成"

# 安装（可选）
install: all
	@echo "安装功能待实现"

.PHONY: all directories libmysocket tests examples test clean install