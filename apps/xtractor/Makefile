binary_name := xtractor
path_step := ../..
source_dirs := .

source_files.windows := xtractor.rc
source_files.mingw := $(source_files.windows)

text_files := text/text

libraries.common = analysis async \
                   binary binary_compression binary_format \
                   debug \
                   formats_archived formats_chiptune formats_image formats_packed \
                   io \
                   l10n_stub \
                   parameters platform platform_application platform_version \
                   strings \
                   tools
libraries.3rdparty = lhasa lzma unrar zlib

libraries.boost = filesystem program_options system
windows_libraries := advapi32 oldnames shell32

include $(path_step)/makefile.mak
include $(path_step)/make/package.mak

install_linux: install_linux_binary

install_linux_binary:
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)

install_windows: install_mingw install_runtime

install_mingw:
	$(call copyfile_cmd,$(target),$(DESTDIR))

dingux_dir = local/apps/$(binary_name)

install_dingux:
	install -D $(target) $(DESTDIR)/$(dingux_dir)/$(notdir $(target))

include $(path_step)/3rdparty/install.mak
