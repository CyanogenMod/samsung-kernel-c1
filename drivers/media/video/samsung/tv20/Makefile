#################################################
# Makefile for TVOut for Samsung
# 2009 (C) Samsung Electronics
# Author : sangpil moon <sangpil.moon@samsung.com>
#################################################

obj-$(CONFIG_VIDEO_TV20)		+= s5p_tvout.o

s5p_tvout-y	+=	ddc.o

ifeq ($(CONFIG_HDMI_HPD), y)
s5p_tvout-y	+=	hpd.o
endif

ifeq ($(CONFIG_HDMI_CEC), y)
s5p_tvout-y	+=	cec.o \
		        ver_1/cec.o
endif

ifeq ($(CONFIG_CPU_S5PV210), y)
s5p_tvout-y	+=	ver_1/hdcp.o \
			ver_1/hdmi.o  \
			ver_1/sdout.o  \
			ver_1/mixer.o  \
			ver_1/vp.o
endif

s5p_tvout-y	+=	s5p_stda_tvout_if.o \
			s5p_stda_grp.o \
			s5p_stda_video_layer.o \
			s5p_tv_v4l2.o \
			s5p_tv_base.o

ifeq ($(CONFIG_TV_FB), y)
s5p_tvout-y	+=	s5p_tv_fb.o
endif

ifeq ($(CONFIG_VIDEO_TV20_DEBUG), y)
EXTRA_CFLAGS += -DDEBUG
endif
