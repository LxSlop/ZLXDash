.SUFFIXES:
ifeq ($(strip $(DEVKITXENON)),)
$(error "Please set DEVKITXENON in your environment. export DEVKITXENON=<path to>devkitPPC")
endif

include $(DEVKITXENON)/rules

TARGET	:=	ZLXDash
BUILD	:=	build
SOURCES	:=	source
ZLX	?=	$(abspath $(dir $(abspath $(firstword $(MAKEFILE_LIST))))../ZLX-Library)

CFLAGS	=	-g -Os -Wall $(MACHDEP) $(INCLUDE) -I$(ZLX)/zlx
CXXFLAGS	=	$(CFLAGS)
LDFLAGS	=	-g $(MACHDEP) -Wl,-Map,$(notdir $@).map
LIBS	:=	$(ZLX)/zlx.a -lxtaf -lext2fs -lfat -lntfs -lpng -lz -lxenon -lm

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(CURDIR)/$(SOURCES)
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(notdir $(wildcard $(SOURCES)/*.c))
CPPFILES	:=	$(notdir $(wildcard $(SOURCES)/*.cpp))
export LD	:=	$(CXX)
export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE	:=	-I$(CURDIR)/$(SOURCES) -I$(LIBXENON_INC)
export LIBPATHS	:=	-L$(LIBXENON_LIB)

.PHONY: all $(BUILD) zlx clean

all: $(BUILD)

# ZLX is built as a library; it is the only thing shared with the ZLX-Library tree
zlx:
	@$(MAKE) --no-print-directory -C $(ZLX) -f Makefile_lib

$(BUILD): zlx
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).elf32

else

$(OUTPUT).elf32: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

-include $(OFILES:.o=.d)

endif
