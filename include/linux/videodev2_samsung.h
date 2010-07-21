/*
 * Video for Linux Two header file for samsung
 *
 * Copyright (C) 2009, Dongsoo Nathaniel Kim<dongsoo45.kim@samsung.com>
 *
 * This header file contains several v4l2 APIs to be proposed to v4l2
 * community and until bein accepted, will be used restrictly in Samsung's
 * camera interface driver FIMC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __LINUX_VIDEODEV2_SAMSUNG_H
#define __LINUX_VIDEODEV2_SAMSUNG_H

/* Values for 'capabilities' field */
/* Object detection device */
#define V4L2_CAP_OBJ_RECOGNITION	0x10000000
/* strobe control */
#define V4L2_CAP_STROBE			0x20000000


#define V4L2_CID_FOCUS_MODE		(V4L2_CID_CAMERA_CLASS_BASE+17)
/* Focus Methods */
enum v4l2_focus_mode {
	V4L2_FOCUS_MODE_AUTO		= 0,
	V4L2_FOCUS_MODE_MACRO		= 1,
	V4L2_FOCUS_MODE_MANUAL		= 2,
	V4L2_FOCUS_MODE_LASTP		= 2,
};

#define V4L2_CID_ZOOM_MODE		(V4L2_CID_CAMERA_CLASS_BASE+18)
/* Zoom Methods */
enum v4l2_zoom_mode {
	V4L2_ZOOM_MODE_CONTINUOUS	= 0,
	V4L2_ZOOM_MODE_OPTICAL		= 1,
	V4L2_ZOOM_MODE_DIGITAL		= 2,
	V4L2_ZOOM_MODE_LASTP		= 2,
};

/* Exposure Methods */
#define V4L2_CID_PHOTOMETRY		(V4L2_CID_CAMERA_CLASS_BASE+19)
enum v4l2_photometry_mode {
	V4L2_PHOTOMETRY_MULTISEG	= 0, /*Multi Segment*/
	V4L2_PHOTOMETRY_CWA		= 1, /*Centre Weighted Average*/
	V4L2_PHOTOMETRY_SPOT		= 2,
	V4L2_PHOTOMETRY_AFSPOT		= 3, /*Spot metering on focused point*/
	V4L2_PHOTOMETRY_LASTP		= V4L2_PHOTOMETRY_AFSPOT,
};

/* Manual exposure control items menu type: iris, shutter, iso */
#define V4L2_CID_CAM_APERTURE	(V4L2_CID_CAMERA_CLASS_BASE+20)
#define V4L2_CID_CAM_SHUTTER	(V4L2_CID_CAMERA_CLASS_BASE+21)
#define V4L2_CID_CAM_ISO	(V4L2_CID_CAMERA_CLASS_BASE+22)

/* Following CIDs are menu type */
#define V4L2_CID_SCENEMODE	(V4L2_CID_CAMERA_CLASS_BASE+23)
#define V4L2_CID_CAM_STABILIZE	(V4L2_CID_CAMERA_CLASS_BASE+24)
#define V4L2_CID_CAM_MULTISHOT	(V4L2_CID_CAMERA_CLASS_BASE+25)

/* Control dynamic range */
#define V4L2_CID_CAM_DR		(V4L2_CID_CAMERA_CLASS_BASE+26)

/* White balance preset control */
#define V4L2_CID_WHITE_BALANCE_PRESET	(V4L2_CID_CAMERA_CLASS_BASE+27)

/* CID extensions */
#define V4L2_CID_ROTATION		(V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PADDR_Y		(V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_PADDR_CB		(V4L2_CID_PRIVATE_BASE + 2)
#define V4L2_CID_PADDR_CR		(V4L2_CID_PRIVATE_BASE + 3)
#define V4L2_CID_PADDR_CBCR		(V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_CID_OVERLAY_AUTO		(V4L2_CID_PRIVATE_BASE + 5)
#define V4L2_CID_OVERLAY_VADDR0		(V4L2_CID_PRIVATE_BASE + 6)
#define V4L2_CID_OVERLAY_VADDR1		(V4L2_CID_PRIVATE_BASE + 7)
#define V4L2_CID_OVERLAY_VADDR2		(V4L2_CID_PRIVATE_BASE + 8)
#define V4L2_CID_OVLY_MODE		(V4L2_CID_PRIVATE_BASE + 9)
#define V4L2_CID_DST_INFO		(V4L2_CID_PRIVATE_BASE + 10)

/*      Pixel format FOURCC depth  Description  */
/* 12  Y/CbCr 4:2:0 64x32 macroblocks */
#define V4L2_PIX_FMT_NV12T    v4l2_fourcc('T', 'V', '1', '2')


/*
 *  * V4L2 extention for digital camera
 *   */
/* Strobe flash light */
enum v4l2_strobe_control {
	/* turn off the flash light */
	V4L2_STROBE_CONTROL_OFF		= 0,
	/* turn on the flash light */
	V4L2_STROBE_CONTROL_ON		= 1,
	/* act guide light before splash */
	V4L2_STROBE_CONTROL_AFGUIDE	= 2,
	/* charge the flash light */
	V4L2_STROBE_CONTROL_CHARGE	= 3,
};

enum v4l2_strobe_conf {
	V4L2_STROBE_OFF			= 0,	/* Always off */
	V4L2_STROBE_ON			= 1,	/* Always splashes */
	/* Auto control presets */
	V4L2_STROBE_AUTO		= 2,
	V4L2_STROBE_REDEYE_REDUCTION	= 3,
	V4L2_STROBE_SLOW_SYNC		= 4,
	V4L2_STROBE_FRONT_CURTAIN	= 5,
	V4L2_STROBE_REAR_CURTAIN	= 6,
	/* Extra manual control presets */
	/* keep turned on until turning off */
	V4L2_STROBE_PERMANENT		= 7,
	V4L2_STROBE_EXTERNAL		= 8,
};

enum v4l2_strobe_status {
	V4L2_STROBE_STATUS_OFF		= 0,
	/* while processing configurations */
	V4L2_STROBE_STATUS_BUSY		= 1,
	V4L2_STROBE_STATUS_ERR		= 2,
	V4L2_STROBE_STATUS_CHARGING	= 3,
	V4L2_STROBE_STATUS_CHARGED	= 4,
};

/* capabilities field */
/* No strobe supported */
#define V4L2_STROBE_CAP_NONE		0x0000
/* Always flash off mode */
#define V4L2_STROBE_CAP_OFF		0x0001
/* Always use flash light mode */
#define V4L2_STROBE_CAP_ON		0x0002
/* Flashlight works automatic */
#define V4L2_STROBE_CAP_AUTO		0x0004
/* Red-eye reduction */
#define V4L2_STROBE_CAP_REDEYE		0x0008
/* Slow sync */
#define V4L2_STROBE_CAP_SLOWSYNC	0x0010
/* Front curtain */
#define V4L2_STROBE_CAP_FRONT_CURTAIN	0x0020
/* Rear curtain */
#define V4L2_STROBE_CAP_REAR_CURTAIN	0x0040
/* keep turned on until turning off */
#define V4L2_STROBE_CAP_PERMANENT	0x0080
/* use external strobe */
#define V4L2_STROBE_CAP_EXTERNAL	0x0100

/* Set mode and Get status */
struct v4l2_strobe {
	/* off/on/charge:0/1/2 */
	enum	v4l2_strobe_control control;
	/* supported strobe capabilities */
	__u32	capabilities;
	enum	v4l2_strobe_conf mode;
	enum 	v4l2_strobe_status status;	/* read only */
	/* default is 0 and range of value varies from each models */
	__u32	flash_ev;
	__u32	reserved[4];
};

#define VIDIOC_S_STROBE     _IOWR('V', 83, struct v4l2_strobe)
#define VIDIOC_G_STROBE     _IOR('V', 84, struct v4l2_strobe)

/* Object recognition and collateral actions */
enum v4l2_recog_mode {
	V4L2_RECOGNITION_MODE_OFF	= 0,
	V4L2_RECOGNITION_MODE_ON	= 1,
	V4L2_RECOGNITION_MODE_LOCK	= 2,
};

enum v4l2_recog_action {
	V4L2_RECOGNITION_ACTION_NONE	= 0,	/* only recognition */
	V4L2_RECOGNITION_ACTION_BLINK	= 1,	/* Capture on blinking */
	V4L2_RECOGNITION_ACTION_SMILE	= 2,	/* Capture on smiling */
};

enum v4l2_recog_pattern {
	V4L2_RECOG_PATTERN_FACE		= 0, /* Face */
	V4L2_RECOG_PATTERN_HUMAN	= 1, /* Human */
	V4L2_RECOG_PATTERN_CHAR		= 2, /* Character */
};

struct v4l2_recog_rect {
	enum	v4l2_recog_pattern  p;	/* detected pattern */
	struct	v4l2_rect  o;	/* detected area */
	__u32	reserved[4];
};

struct v4l2_recog_data {
	__u8	detect_cnt;		/* detected object counter */
	struct	v4l2_rect	o;	/* detected area */
	__u32	reserved[4];
};

struct v4l2_recognition {
	enum v4l2_recog_mode	mode;

	/* Which pattern to detect */
	enum v4l2_recog_pattern  pattern;

	/* How many object to detect */
	__u8 	obj_num;

	/* select detected object */
	__u32	detect_idx;

	/* read only :Get object coordination */
	struct v4l2_recog_data	data;

	enum v4l2_recog_action	action;
	__u32	reserved[4];
};

#define VIDIOC_S_RECOGNITION	_IOWR('V', 85, struct v4l2_recognition)
#define VIDIOC_G_RECOGNITION	_IOR('V', 86, struct v4l2_recognition)

#endif /* __LINUX_VIDEODEV2_SAMSUNG_H */
