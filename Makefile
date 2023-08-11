# SPDX-License-Identifier: BSD-2-Clause
# Author: Erik Inkinen <erik.inkinen@erikinkinen.fi>

ARCH ?= x86_64

CC = clang
CFLAGS = -I efi/include -I src -target $(ARCH)-pc-win32-coff -fno-stack-protector -fshort-wchar -mno-red-zone 
LD = lld-link
LDFLAGS = -subsystem:efi_application -nodefaultlib -dll

TARGETS = src/main.o
TARGET_APP = erikboot.efi

all: $(TARGET_APP)

clean:
	@echo -e "\e[1;31m- CLEANING\e[0m"
	@rm -f $(TARGETS) $(TARGET_APP) $(subst .efi,.lib,$(TARGET_APP))

$(TARGET_APP): $(TARGETS)
	@echo -e "\e[1;32m- LINKING $@\e[0m"
	@$(LD) $(LDFLAGS) -entry:efi_main $< -out:$@

%.o: %.c
	@echo -e "\e[32m- COMPILING $@\e[0m"
	@$(CC) $(INCDIR) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
