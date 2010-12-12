makebin_name = $(1)
makelib_name = lib$(1).a
makedyn_name = lib$(1).so
makeobj_name = $(1).o
makedir_cmd = mkdir -p $(1)
rmdir_cmd = rm -Rf $(1)
rmfiles_cmd = rm -f $(1)
showtime_cmd = date +"%x %X"

compiler=gcc
CXX_PLATFORM_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
ifdef release
LD_PLATFORM_FLAGS += -Wl,-O3,-x,--gc-sections,--relax
endif

#built-in features
support_oss = 1
support_alsa = 1
#support_sdl = 1

ifdef STATIC_BOOST_PATH
include_dirs += $(STATIC_BOOST_PATH)/include
linux_libraries_dirs += $(STATIC_BOOST_PATH)/lib
endif

#multithread release libraries
linux_libraries += $(foreach lib,$(boost_libraries),boost_$(lib)-mt)

#release libraries
linux_libraries += $(foreach lib,$(qt_libraries),Qt$(lib))
