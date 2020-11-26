#
#   Copyright (C) 2015 HWPORT.COM
#   All rights reserved.
#
#   Maintainers
#     JaeHyuk Cho ( <mailto:minzkn@minzkn.com>, <mailto:minzkn@wins21.co.kr> )
#

SHELL_BASH :=$(wildcard /bin/bash)#
ifneq ($(SHELL_BASH),)
SHELL :=$(SHELL_BASH)# bash shell default using
else
SHELL ?=/bin/sh#
endif
MAKE ?=make#

# .EXPORT_ALL_VARIABLES: # DO NOT USE !
MAKEFLAGS                    ?=#
export MAKEFLAGS
export PATH

HOST_NAME :=$(shell hostname --short)#
HOST_USER :=$(shell whoami)#
HOST_ARCH :=$(shell echo "$(shell uname -m)" | sed \
    -e s/sun4u/sparc64/ \
    -e s/arm.*/arm/ \
    -e s/sa110/arm/ \
    -e s/s390x/s390/ \
    -e s/parisc64/parisc/ \
    -e s/ppc.*/powerpc/ \
    -e s/mips.*/mips/ \
)# auto detect architecture
HOST_OS :=$(shell echo "$(shell uname)" | sed \
    -e  s/Linux/linux/ \
    -e  s/Darwin/darwin/ \
)# auto detect os
HOST_VENDOR :=pc#
HOST_LIBC :=gnu#
HOST_LABEL :=$(HOST_ARCH)#
HOST_BUILD_PROFILE :=$(HOST_ARCH)-$(HOST_VENDOR)-$(HOST_OS)-$(HOST_LIBC)#

TARGET_ARCH :=$(HOST_ARCH)#
TARGET_VENDOR :=$(HOST_VENDOR)#
TARGET_OS :=$(HOST_OS)#
TARGET_LIBC :=$(HOST_LIBC)#
TARGET_LABEL :=$(TARGET_ARCH)#
TARGET_BUILD_PROFILE :=$(TARGET_ARCH)-$(TARGET_VENDOR)-$(TARGET_OS)-$(TARGET_LIBC)#

EXT_DEPEND :=.d#
EXT_C_SOURCE :=.c#
EXT_CXX_SOURCE :=.cpp#
EXT_C_HEADER :=.h#
EXT_CXX_HEADER :=.h#
EXT_OBJECT :=.o#
EXT_LINK_OBJECT :=.lo#
EXT_ARCHIVE :=.a#
EXT_SHARED :=.so#
EXT_EXEC :=#
EXT_CONFIG :=.conf#

CROSS_COMPILE :=#

ECHO :=echo#
SYMLINK :=ln -sf#
SED :=sed#
INSTALL :=install#
INSTALL_BIN :=$(INSTALL) -m0755#
INSTALL_LIB :=$(INSTALL) -m0755#
INSTALL_DIR :=$(INSTALL) -d -m0755#
INSTALL_DATA :=$(INSTALL) -m0644#
INSTALL_CONF :=$(INSTALL) -m0644#

CC := $(CROSS_COMPILE)gcc#
LD := $(CROSS_COMPILE)ld#
AR := $(CROSS_COMPILE)ar#
RM := rm -f#
COPY_FILE := cp -f#
CAT :=cat#
STRIP := $(CROSS_COMPILE)strip#

THIS_NAME :=workload#
THIS_LIBNAME :=workload#
THIS_VERSION :=0.0.1# library version
THIS_INTERFACE_VERSION :=0# library interface version

DESTDIR :=./rootfs# default staging directory
CFLAGS_COMMON :=#
CFLAGS :=#
LDFLAGS_COMMON :=#
LDFLAGS :=#
LDFLAGS_EXEC :=-rdynamic -fPIE -pie#
LDFLAGS_SHARED_COMMON :=#
LDFLAGS_SHARED_LINK :=#
LDFLAGS_SHARED :=#
ARFLAGS_COMMON :=#
ARFLAGS :=#

CFLAGS_COMMON +=-O2#
#CFLAGS_COMMON +=-g#
CFLAGS_COMMON +=-pipe#
CFLAGS_COMMON +=-fPIC#
#CFLAGS_COMMON +=-fomit-frame-pointer# backtrace() daes not work !
CFLAGS_COMMON +=-fno-omit-frame-pointer# backtrace() will work normally.
CFLAGS_COMMON_strict +=-ansi#
CFLAGS_COMMON_strict +=-Wall -W#
CFLAGS_COMMON_strict +=-Wshadow#
CFLAGS_COMMON_strict +=-Wcast-qual#
CFLAGS_COMMON_strict +=-Wcast-align#
CFLAGS_COMMON_strict +=-Wpointer-arith#
CFLAGS_COMMON_strict +=-Wbad-function-cast#
CFLAGS_COMMON_strict +=-Wstrict-prototypes#
CFLAGS_COMMON_strict +=-Wmissing-prototypes#
CFLAGS_COMMON_strict +=-Wmissing-declarations#
CFLAGS_COMMON_strict +=-Wnested-externs#
CFLAGS_COMMON_strict +=-Winline#
CFLAGS_COMMON_strict +=-Wwrite-strings#
CFLAGS_COMMON_strict +=-Wchar-subscripts#
CFLAGS_COMMON_strict +=-Wformat#
CFLAGS_COMMON_strict +=-Wformat-security#
CFLAGS_COMMON_strict +=-Wimplicit#
CFLAGS_COMMON_strict +=-Wmain#
CFLAGS_COMMON_strict +=-Wmissing-braces#
CFLAGS_COMMON_strict +=-Wnested-externs#
CFLAGS_COMMON_strict +=-Wparentheses#
CFLAGS_COMMON_strict +=-Wredundant-decls#
CFLAGS_COMMON_strict +=-Wreturn-type#
CFLAGS_COMMON_strict +=-Wsequence-point#
CFLAGS_COMMON_strict +=-Wsign-compare#
CFLAGS_COMMON_strict +=-Wswitch#
CFLAGS_COMMON_strict +=-Wuninitialized#
CFLAGS_COMMON_strict +=-Wunknown-pragmas#
CFLAGS_COMMON_strict +=-Wcomment#
CFLAGS_COMMON_strict +=-Wundef#
CFLAGS_COMMON_strict +=-Wunused#
#CFLAGS_COMMON_strict +=-Wunreachable-code#
CFLAGS_COMMON_strict +=-Wconversion#
#CFLAGS_COMMON_strict +=-Wpadded#
CFLAGS_COMMON += $(CFLAGS_COMMON_strict)#
CFLAGS_COMMON +=-I./source -I./include -I.#
CFLAGS_COMMON +=-D_REENTRANT# thread safety (optional)
CFLAGS_COMMON +=-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64# enable 64-bits file i/o compatibility (optional)
CFLAGS_COMMON +=-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0# glibc run-time compatibility compile (optional)

CFLAGS_COMMON +=-Ddef_workload_use_sqlite3=0
CFLAGS_COMMON +=-Ddef_workload_use_mysql_connector_c=0

LDFLAGS_SHARED_COMMON +=-L.#
LDFLAGS_SHARED_LINK +=-lpthread#
LDFLAGS +=-s#

ARFLAGS_COMMON +=rcs#

TARGET :=$(foreach s_this_name,$(EXT_LINK_OBJECT) $(EXT_ARCHIVE) $(EXT_SHARED),lib$(THIS_LIBNAME)$(s_this_name))# library
TARGET +=$(THIS_NAME)_static$(EXT_EXEC)# static executable shared object
TARGET +=$(THIS_NAME)_shared$(EXT_EXEC)# shared executable shared object
TARGET +=$(THIS_NAME)$(EXT_EXEC)# executable shared object

# default make goal
.PHONY: all world rebuild install
all world: __build_all
rebuild: clean all

install: all
	@$(ECHO) "[**] installing (DESTDIR=\"$(DESTDIR)\")"
	@$(INSTALL_DIR) "$(DESTDIR)/"
	@$(INSTALL_DIR) "$(DESTDIR)/usr/"
#	@$(INSTALL_DIR) "$(DESTDIR)/usr/include/"
#	@$(INSTALL_DATA) "./include/workload.h" "$(DESTDIR)/usr/include/"
#	@$(INSTALL_DIR) "$(DESTDIR)/usr/lib/"
#	@$(INSTALL_LIB) "lib$(THIS_LIBNAME)$(EXT_LINK_OBJECT)" "$(DESTDIR)/usr/lib/"
#	@$(INSTALL_LIB) "lib$(THIS_LIBNAME)$(EXT_ARCHIVE)" "$(DESTDIR)/usr/lib/"
#	@$(INSTALL_LIB) "lib$(THIS_LIBNAME)$(EXT_SHARED).$(THIS_VERSION)" "$(DESTDIR)/usr/lib/"
#	@$(SYMLINK) "lib$(THIS_LIBNAME)$(EXT_SHARED).$(THIS_VERSION)" "$(DESTDIR)/usr/lib/lib$(THIS_LIBNAME)$(EXT_SHARED).$(THIS_INTERFACE_VERSION)"
#	@$(SYMLINK) "lib$(THIS_LIBNAME)$(EXT_SHARED).$(THIS_VERSION)" "$(DESTDIR)/usr/lib/lib$(THIS_LIBNAME)$(EXT_SHARED)"
	@$(INSTALL_DIR) "$(DESTDIR)/usr/bin/"
	@$(INSTALL_BIN) "$(THIS_NAME)" "$(DESTDIR)/usr/bin/"
	@$(INSTALL_DIR) "$(DESTDIR)/etc/"
	@$(INSTALL_CONF) "workload.xml" "$(DESTDIR)/etc/workload.xml"
	@$(ECHO) "[**] installed (DESTDIR=\"$(DESTDIR)\")"

# clean project
.PHONY: distclean clean
distclean clean:
	@$(ECHO) "[**] $(@)"
	@$(RM) -f $(wildcard *$(EXT_OBJECT) *$(EXT_DEPEND) *$(EXT_LINK_OBJECT) *$(EXT_ARCHIVE) *$(EXT_SHARED) *$(EXT_SHARED).* *$(EXT_CONFIG) $(TARGET))

# cross build target
X86_PC_TOOLCHAIN_PATHNAME_LIST :=/opt/i686-pc-linux-gnu/bin#
X86_PC_TOOLCHAIN_PATHNAME_LIST +=/usr/local/i686-pc-linux-gnu/bin#
X86_SYNOLOGY_TOOLCHAIN_PATHNAME_LIST :=/opt/i686-pc-linux-gnu/bin#
X86_SYNOLOGY_TOOLCHAIN_PATHNAME_LIST +=/usr/local/i686-pc-linux-gnu/bin#
.PHONY: synology x86-pc-linux-gnu x86-synology-linux-gnu i686-pc-linux-gnu i686-synology-linux-gnu
synology: i686-synology-linux-gnu
x86-pc-linux-gnu: i686-pc-linux-gnu
i686-pc-linux-gnu:
	@$(ECHO) "[**] building profile $(@)"
	@PATH=${PATH}$(foreach s_this_pathname,$(wildcard $(X86_PC_TOOLCHAIN_PATHNAME_LIST)),:$(s_this_pathname)) \
	$(MAKE) \
	--no-print-directory \
	CROSS_COMPILE="i686-pc-linux-gnu-" \
	CFLAGS="" \
	LDFLAGS="" \
	LDFLAGS_SHARED="" \
	TARGET_ARCH="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\1/")" \
	TARGET_VENDOR="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\2/")" \
	TARGET_OS="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\3/")" \
	TARGET_LIBC="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\4/")" \
	__build_all
x86-synology-linux-gnu: i686-synology-linux-gnu
i686-synology-linux-gnu:
	@$(ECHO) "[**] building profile $(@)"
	@PATH=${PATH}$(foreach s_this_pathname,$(wildcard $(X86_SYNOLOGY_TOOLCHAIN_PATHNAME_LIST)),:$(s_this_pathname)) \
	$(MAKE) \
	--no-print-directory \
	CROSS_COMPILE="i686-pc-linux-gnu-" \
	CFLAGS="" \
	LDFLAGS="" \
	LDFLAGS_SHARED="" \
	TARGET_ARCH="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\1/")" \
	TARGET_VENDOR="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\2/")" \
	TARGET_OS="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\3/")" \
	TARGET_LIBC="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\4/")" \
	__build_all

X86_64_PC_TOOLCHAIN_PATHNAME_LIST :=#
.PHONY: x86_64-pc-linux-gnu
x86_64-pc-linux-gnu:
	@$(ECHO) "[**] building profile $(@)"
	@PATH=${PATH}$(foreach s_this_pathname,$(wildcard $(X86_64_PC_TOOLCHAIN_PATHNAME_LIST)),:$(s_this_pathname)) \
	$(MAKE) \
	--no-print-directory \
	CROSS_COMPILE="" \
	CFLAGS="" \
	LDFLAGS="" \
	LDFLAGS_SHARED="" \
	TARGET_ARCH="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\1/")" \
	TARGET_VENDOR="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\2/")" \
	TARGET_OS="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\3/")" \
	TARGET_LIBC="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\4/")" \
	__build_all

MIPS_SMP86XX_TOOLCHAIN_PATHNAME_LIST :=/opt/mips-4.3/bin#
.PHONY: smp8671 mipsel-smp86xx-linux-gnu
smp8671: mipsel-smp86xx-linux-gnu
mipsel-smp86xx-linux-gnu:
	@$(ECHO) "[**] building profile $(@)"
	@PATH=${PATH}$(foreach s_this_pathname,$(wildcard $(MIPS_SMP86XX_TOOLCHAIN_PATHNAME_LIST)),:$(s_this_pathname)) \
	$(MAKE) \
	--no-print-directory \
	CROSS_COMPILE="mips-linux-gnu-" \
	CFLAGS="-EL -Wa,-mips32r2 -Wa,-mfix7000 -mtune=24kf" \
	LDFLAGS="-EL" \
	LDFLAGS_SHARED="" \
	TARGET_ARCH="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\1/")" \
	TARGET_VENDOR="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\2/")" \
	TARGET_OS="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\3/")" \
	TARGET_LIBC="$(shell echo "$(@)" | $(SED) -e "s/^\(.*\)-\(.*\)-\(.*\)-\(.*\)/\4/")" \
	__build_all

# real build depend
.PHONY: __build_all
__build_all: $(TARGET)
	@$(ECHO) "[**] build complete ($(^))"
	$(if $(THIS_VERSION),@$(ECHO) "   - THIS_VERSION=\"$(THIS_VERSION)\"")
	$(if $(TARGET_BUILD_PROFILE),@$(ECHO) "   - TARGET_BUILD_PROFILE=\"$(TARGET_BUILD_PROFILE)\"")
	$(if $(THIS_INTERFACE_VERSION),@$(ECHO) "   - THIS_INTERFACE_VERSION=\"$(THIS_INTERFACE_VERSION)\"")
	$(if $(CROSS_COMPILE),@$(ECHO) "   - CROSS_COMPILE=\"$(CROSS_COMPILE)\"")
	$(if $(strip $(CFLAGS_COMMON) $(CFLAGS)),@$(ECHO) "   - CFLAGS=\"$(strip $(CFLAGS_COMMON) $(CFLAGS))\"")
	$(if $(strip $(LDFLAGS_COMMON) $(LDFLAGS)),@$(ECHO) "   - LDFLAGS=\"$(strip $(LDFLAGS_COMMON) $(LDFLAGS))\"")
	$(if $(strip $(LDFLAGS_EXEC)),@$(ECHO) "   - LDFLAGS_EXEC=\"$(strip $(LDFLAGS_EXEC))\"")
	$(if $(strip $(LDFLAGS_SHARED_COMMON) $(LDFLAGS_SHARED)),@$(ECHO) "   - LDFLAGS_SHARED=\"$(strip $(LDFLAGS_SHARED_COMMON) $(LDFLAGS_SHARED))\"")
	$(if $(strip $(ARFLAGS_COMMON) $(ARFLAGS)),@$(ECHO) "   - LDFLAGS_SHARED=\"$(strip $(ARFLAGS_COMMON) $(ARFLAGS))\"")

# exec link (-fPIE -pie => shared object build)
MAIN_SOURCE_LIST :=$(THIS_NAME)$(EXT_C_SOURCE)#
#MAIN_SOURCE_LIST :=$(notdir $(wildcard ./source/*$(EXT_C_SOURCE)))# auto detect source
MAIN_OBJECTS :=$(MAIN_SOURCE_LIST:%$(EXT_C_SOURCE)=%$(EXT_OBJECT))# auto generate object by source
$(THIS_NAME)$(EXT_EXEC): $(THIS_NAME)_static$(EXT_EXEC)
	@$(ECHO) "[CP] $(notdir $(@)) <= $(notdir $(<))"
	@$(COPY_FILE) "$(<)" "$(@)"
$(THIS_NAME)_static$(EXT_EXEC): $(MAIN_OBJECTS) lib$(THIS_LIBNAME)$(EXT_ARCHIVE)
	@$(ECHO) "[LD] $(notdir $(@)) <= $(notdir $(^)) (LDFLAGS=\"$(strip $(LDFLAGS_SHARED_COMMON) $(LDFLAGS_SHARED) $(LDFLAGS_COMMON) $(LDFLAGS) $(LDFLAGS_EXEC))\")"
	@$(CC) $(LDFLAGS_SHARED_COMMON) $(LDFLAGS_SHARED) $(LDFLAGS_COMMON) $(LDFLAGS) $(LDFLAGS_EXEC) -o "$(@)" $(^) $(LDFLAGS_SHARED_LINK)
	@$(STRIP) --remove-section=.comment --remove-section=.note $(@) # strong strip (optional)
$(THIS_NAME)_shared$(EXT_EXEC): $(MAIN_OBJECTS) lib$(THIS_LIBNAME)$(EXT_SHARED)
	@$(ECHO) "[LD] $(notdir $(@)) <= $(notdir $(^)) (LDFLAGS=\"$(strip $(LDFLAGS_SHARED_COMMON) $(LDFLAGS_SHARED) $(LDFLAGS_COMMON) $(LDFLAGS) $(LDFLAGS_EXEC))\")"
	@$(CC) $(LDFLAGS_SHARED_COMMON) $(LDFLAGS_SHARED) $(LDFLAGS_COMMON) $(LDFLAGS) $(LDFLAGS_EXEC) -o "$(@)" $(^) $(LDFLAGS_SHARED_LINK)
	@$(STRIP) --remove-section=.comment --remove-section=.note $(@) # strong strip (optional)
$(MAIN_OBJECTS): CFLAGS_COMMON+=-fPIE

# library link
LIBRARY_SOURCE_LIST :=$(filter-out $(MAIN_SOURCE_LIST),$(notdir $(wildcard ./source/*$(EXT_C_SOURCE))))# auto detect source
#LIBRARY_SOURCE_LIST :=$(notdir $(wildcard ./source/*$(EXT_C_SOURCE)))# auto detect source
LIBRARY_OBJECTS :=$(LIBRARY_SOURCE_LIST:%$(EXT_C_SOURCE)=%$(EXT_OBJECT))# auto generate object by source
lib$(THIS_LIBNAME)$(EXT_LINK_OBJECT): $(LIBRARY_OBJECTS)
	@$(ECHO) "[LO] $(notdir $(@)) <= $(notdir $(^)) (LDFLAGS=\"$(strip $(LDFLAGS_COMMON) $(LDFLAGS))\")"
	@$(LD) $(LDFLAGS_COMMON) $(LDFLAGS) -r -o "$(@)" $(^)
lib$(THIS_LIBNAME)$(EXT_ARCHIVE): $(LIBRARY_OBJECTS)
	@$(ECHO) "[AR] $(notdir $(@)) <= $(notdir $(^)) (ARFLAGS=\"$(strip $(ARFLAGS_COMMON) $(ARFLAGS))\")"
	@$(foreach s_this_name,$(?),$(AR) $(ARFLAGS_COMMON) $(ARFLAGS) "$(@)" "$(s_this_name)" 2>/dev/null;)
lib$(THIS_LIBNAME)$(EXT_SHARED): $(LIBRARY_OBJECTS)
	@$(ECHO) "[SO] $(notdir $(@)).$(THIS_VERSION) <= $(notdir $(^)) (soname=$(@).$(THIS_INTERFACE_VERSION), LDFLAGS=\"$(strip $(LDFLAGS_SHARED_COMMON) $(LDFLAGS_SHARED) $(LDFLAGS_COMMON) $(LDFLAGS))\")"
	@$(CC) $(LDFLAGS_SHARED_COMMON) $(LDFLAGS_SHARED) $(LDFLAGS_COMMON) $(LDFLAGS) -shared -Wl,-soname,$(@).$(THIS_INTERFACE_VERSION) -o "$(@).$(THIS_VERSION)" $(^) $(LDFLAGS_SHARED_LINK)
	@$(ECHO) "[SL] $(notdir $(@)).$(THIS_INTERFACE_VERSION) <= $(notdir $(@)).$(THIS_VERSION)"
	@$(SYMLINK) "$(@).$(THIS_VERSION)" "$(@).$(THIS_INTERFACE_VERSION)"
	@$(ECHO) "[SL] $(notdir $(@)) <= $(notdir $(@)).$(THIS_VERSION)"
	@$(SYMLINK) "$(@).$(THIS_VERSION)" "$(@)"

# common compile
CFLAGS_COMMON_for_ezxml:=$(filter-out $(CFLAGS_COMMON_strict),$(CFLAGS_COMMON))
ezxml$(EXT_OBJECT) xml$(EXT_OBJECT): CFLAGS_COMMON=$(CFLAGS_COMMON_for_ezxml)
%$(EXT_OBJECT): ./source/%$(EXT_C_SOURCE) makefile.gnu
	@$(ECHO) "[CC] $(notdir $(@)) <= $(notdir $(<))"
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS) -c -o "$(@)" "$(<)"
	@$(CC) -MMD $(CFLAGS_COMMON) $(CFLAGS) -c -o "$(@)" "$(<)" # create depend rule file (strong depend check, optional)

# include depend rules (strong depend check, optional)
override THIS_DEPEND_RULES_LIST:=$(wildcard *$(EXT_DEPEND))#
ifneq ($(THIS_DEPEND_RULES_LIST),)
sinclude $(THIS_DEPEND_RULES_LIST)
endif

.DEFAULT:
	@$(ECHO) "[!!] unknown goal ($(@))"

# End of makefile.gnu
