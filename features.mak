ifdef support_waveout
definitions += WIN32_WAVEOUT_SUPPORT
$(platform)_libraries += winmm
endif

ifdef support_oss
definitions += OSS_SUPPORT
endif

ifdef support_alsa
definitions += ALSA_SUPPORT
$(platform)_libraries += asound
endif
