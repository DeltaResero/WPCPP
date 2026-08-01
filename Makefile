# Wii Pi Calculation Project Plus - Makefile
#---------------------------------------------------------------------------------
# Clear the implicit built-in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
.SECONDARY:

#---------------------------------------------------------------------------------
# Check if DEVKITPPC is set up correctly
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:= WPCPP
BUILD		:= build
SOURCES		:= src
INCLUDES	:= src
LIBOGC_INC	:= $(DEVKITPRO)/libogc/include
LIBOGC_LIB	:= $(DEVKITPRO)/libogc/lib/wii
PORTLIBS	:= $(DEVKITPRO)/portlibs/wii

#---------------------------------------------------------------------------------
# Compiler and tools
#---------------------------------------------------------------------------------
CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc
CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++
STRIP = $(DEVKITPPC)/bin/powerpc-eabi-strip

#---------------------------------------------------------------------------------
# Options for code generation
#---------------------------------------------------------------------------------
# MACHDEP already carries -DGEKKO, so this does not repeat it
CFLAGS		:= -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS	:= $(CFLAGS)
# Assign with '=' so that $@ is read when a target is linked. With ':=' it is read
# here instead, where no target exists yet, and the map file comes out named '.map'
LDFLAGS		= $(MACHDEP) -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# Any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
# Order matters for static archives. The linker takes them left to right and only
# keeps what is unresolved so far, so gmpxx has to come before the gmp it calls into
LIBS		:= -lwiiuse -lbte -logc -lm -lgmpxx -lgmp

#---------------------------------------------------------------------------------
# GMP configuration
#---------------------------------------------------------------------------------
GMP_DIR		:= third_party/gmp
GMP_INSTALL_PFX	:= $(abspath $(CURDIR)/$(GMP_DIR)/build-output)
GMP_LIB		:= $(GMP_INSTALL_PFX)/lib/libgmp.a

#---------------------------------------------------------------------------------
# List of directories containing libraries
#---------------------------------------------------------------------------------
LIBDIRS		:= $(PORTLIBS) $(GMP_INSTALL_PFX)


#---------------------------------------------------------------------------------
# No real need to edit anything past this point
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export OUTPUT	:= $(CURDIR)/$(TARGET)
export VPATH	:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:= $(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# Automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))

#---------------------------------------------------------------------------------
# Use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
    export LD := $(CC)
else
    export LD := $(CXX)
endif

export OFILES    := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# Build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:= $(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
	           $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
	           -I$(LIBOGC_INC) \
	           -I$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# Build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:= $(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
	           -L$(LIBOGC_LIB)

.PHONY: $(BUILD) clean distclean all run gmp

#---------------------------------------------------------------------------------
all: gmp $(BUILD)

gmp: $(GMP_LIB)

# GMP comes from its git mirror, and that mirror carries no configure script.
# Only the release tarballs ship one. It has to be generated here first, which
# is what GMP's own .bootstrap does, and that needs autoconf, automake, libtool
# and m4 on the machine doing the building
$(GMP_LIB):
	@echo "Building GMP library..."
	@if [ ! -f "$(GMP_DIR)/.bootstrap" ]; then \
		echo "ERROR: $(GMP_DIR) is empty, so GMP cannot be built."; \
		echo "Fetch it with: git submodule update --init --recursive"; \
		exit 1; \
	fi
	@cd $(GMP_DIR) && \
	if [ ! -f "configure" ]; then \
		echo "Generating GMP's configure script..."; \
		./.bootstrap; \
	fi
	@cd $(GMP_DIR) && \
	if [ ! -f "Makefile" ]; then \
		ac_cv_func_obstack_vprintf=no ./configure \
			--prefix=$(GMP_INSTALL_PFX) \
			--host=powerpc-eabi \
			--enable-cxx \
			--disable-shared \
			"ABI=32" \
			"CFLAGS=-g -O2 -mrvl -mcpu=750 -meabi -mhard-float" \
			"CXXFLAGS=-g -O2"; \
	fi
	@$(MAKE) -j$(shell nproc) -C $(GMP_DIR)
	@$(MAKE) -C $(GMP_DIR) install
	@touch $(GMP_LIB)
	@echo "GMP library built successfully"

#---------------------------------------------------------------------------------
$(BUILD): $(GMP_LIB)
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# Clean rules, now including distclean
#---------------------------------------------------------------------------------
clean:
	@echo "Cleaning project files..."
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol $(OUTPUT).elf.map

distclean: clean
	@echo "Cleaning GMP build artifacts..."
	@if [ -f "$(GMP_DIR)/Makefile" ]; then $(MAKE) -C $(GMP_DIR) distclean; fi
	@rm -rf $(GMP_DIR)/build-output

#---------------------------------------------------------------------------------
run:
	wiiload $(TARGET).dol

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# Main Targets - After creation, strip the ELF file and convert to DOL
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
	@echo "Linking ELF file..."
	$(LD) -o $@ $(OFILES) $(LIBPATHS) $(LDFLAGS) $(LIBS)
	@echo "Stripping ELF file..."
	$(STRIP) $@

#---------------------------------------------------------------------------------
-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
