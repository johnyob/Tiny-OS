# Cross compiler for risc v
TOOLCHAIN = riscv64-unknown-linux-gnu

CC = $(TOOLCHAIN)-gcc

OPT_C_FLAGS = -O0 -g -ggdb3
WARN_C_FLAGS = 

C_FLAGS = $(WARN_C_FLAGS) $(OPT_C_FLAGS)
C_FLAGS += -ffreestanding -nostdlib -nodefaultlibs
C_FLAGS += -mcmodel=medany -march=rv64imafdc -mabi=lp64d

# Source directories
SRC = ../src
HEAD = ../include

C_OBJECTS = $(shell find $(SRC) -name "*.c")
ASM_OBJECTS = $(shell find $(SRC) -name "*.S")
HEADERS = $(shell find $(HEAD) -name "*.h")

OBJECT_DIR = object
KERNEL_OUT = kernel

OBJECTS = $(patsubst $(SRC)/%.S, $(OBJECT_DIR)/%.o, $(ASM_OBJECTS))
OBJECTS += $(patsubst $(SRC)/%.c, $(OBJECT_DIR)/%.o, $(C_OBJECTS))

QEMU = qemu-system-riscv64
MACH = virt
MEM = 128M

QEMU_OPTS = -machine $(MACH) -m $(MEM) -nographic -serial mon:stdio -kernel $(KERNEL_OUT) -bios none
DEBUG_OPTS = -gdb tcp::1234 -S

build: $(OBJECTS) $(HEADERS)
		echo $(OBJCTS)
		$(CC) $(C_FLAGS) -T linker.ld -o $(KERNEL_OUT) $(OBJECTS)

$(OBJECT_DIR)/%.o: $(SRC)/%.S
				mkdir -p $(@D)
				$(CC) $(C_FLAGS) -I$(HEAD) -c $< -o $@

$(OBJECT_DIR)/%.o: $(SRC)/%.c
				mkdir -p $(@D)
				$(CC) $(C_FLAGS) -I$(HEAD) -c $< -o $@ 

clean:
	rm -rf $(OBJECT_DIR)
	rm $(KERNEL_OUT)

run: build
	$(QEMU) $(QEMU_OPTS)

debug: build
	$(QEMU) $(QEMU_OPTS) $(DEBUG_OPTS)
