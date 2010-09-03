# linux/drivers/media/video/samsung/tv20/Makefile
#
# Copyright (c) 2009 Samsung Electronics
#		http://www.samsung.com/
#
# Makefile for Samsung TVOut driver

ifeq ($(CONFIG_VIDEO_TV30_DEBUG), y)
EXTRA_CFLAGS += -DDEBUG
endif

s5p_tvout-y :=	s5p_tv_base.o		\
		s5p_mixer_ctrl.o	\
		s5p_vp_ctrl.o		\
		s5p_tvout_ctrl.o	\
		s5p_tv_fb.o		\
		s5p_tv_v4l2.o		\
		ddc.o			\
		ver_1/hdcp.o		\
		ver_1/hdmi.o		\
		ver_1/sdo.o		\
		ver_1/mixer.o		\
		ver_1/vp.o

s5p_tvout-$(CONFIG_HDMI_HPD) +=	hpd.o

s5p_tvout-$(CONFIG_HDMI_CEC) +=	cec.o	\
				ver_1/cec.o

obj-$(CONFIG_VIDEO_TV30) += s5p_tvout.o
