ifneq ("$(wildcard ../libspicboard/debug.mk)","")
include ../libspicboard/debug.mk
else
LIBSPICBOARDDIR ?= /proj/i4spic/pub/libspicboard

default: concat.elf
  
include $(LIBSPICBOARDDIR)/debug.mk
endif

