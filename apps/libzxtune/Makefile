dynamic_name := zxtune
path_step := ../..
source_dirs := src

static_runtime=1

libraries.common = analysis \
                   binary binary_compression binary_format \
                   core core_plugins_archives_stub core_plugins_archives_stub core_plugins_players \
                   debug devices_aym devices_beeper devices_dac devices_fm devices_saa devices_z80 \
                   formats_chiptune formats_multitrack formats_packed_lha \
                   l10n_stub \
                   module module_players \
                   parameters platform_version \
                   sound strings \
                   tools

libraries.3rdparty = gme he hvl lazyusf2 lhasa mgba sidplayfp snesspc vio2sf xmp z80ex zlib

windows_libraries := oldnames

include $(path_step)/makefile.mak
