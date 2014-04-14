/*
 * kinect sensor device, gspca driver
 *
 * Copyright (C) 2014  Alexander Sosna <Alexander Sosna>
 *
 * Based on the kinect video driver from:
 * Antonio Ospite <ospite@studenti.unina.it> in 2011
 * and OpenKinect project and libfreenect
 * http://openkinect.org
 *
 * Special thanks to Antonio Ospite 
 * who made the video driver and encouraged me with my work. 
 * 
 * Thanks to openkinect.org / libfreenect for your work!
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
 /*
  * This driver does _NOT_ work with the current versin of gspa_main, 
  * it needs a patcht version which gives access to the second isoc entpoint.
 */
 
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define MODULE_NAME "kinect_depth"

#include "gspca.h"

#define CTRL_TIMEOUT 500
#define OUTPUT_BUFFER_SIZE 1024  /* 0x400 ;) */
#define INPUT_BUFFER_SIZE   512  /* 0x200 ;) */

#define SEND_MAGIC_0 0x47
#define SEND_MAGIC_1 0x4d
#define RECEIVE_MAGIC_0 0x52
#define RECEIVE_MAGIC_1 0x42
#define STREAM_FLAG 0x70
#define PKT_SIZE 1760


MODULE_AUTHOR("Alexander Sosna <alexander@xxor.de>");
MODULE_DESCRIPTION("GSPCA/Kinect Sensor Device USB Driver for Depth Data");
MODULE_LICENSE("GPL");

struct pkt_hdr {
	uint8_t magic[2];
	uint8_t pad;
	uint8_t flag;
	uint8_t unk1;
	uint8_t seq;
	uint8_t unk2;
	uint8_t unk3;
	uint32_t timestamp;
};

struct cam_hdr {
	uint8_t magic[2];
	uint16_t len;
	uint16_t cmd;
	uint16_t tag;
};

/* specific webcam descriptor */
struct sd {
	struct gspca_dev gspca_dev; /* !! must be the first item */
	uint16_t cam_tag;           /* a sequence number for packets */
	uint8_t stream_flag;       /* to identify different stream types */
	uint8_t obuf[OUTPUT_BUFFER_SIZE]; /*ob for control commands */
	uint8_t ibuf[INPUT_BUFFER_SIZE];  /* ib for control commands */
};

#define MODE_640x480   0x0001
#define FORMAT_Y10B    0x0040
#define FPS_HIGH       0x0100

static const struct v4l2_pix_format video_camera_mode[] = {
		{640, 480, V4L2_PIX_FMT_Y10BPACK, V4L2_FIELD_NONE,
	 .bytesperline = 640 * 10 / 8,
	 .sizeimage =  640 * 480 * 10 / 8,
	 .colorspace = V4L2_COLORSPACE_SRGB,
	 .priv = MODE_640x480 | FORMAT_Y10B | FPS_HIGH},
};

static int kinect_write(struct usb_device *udev, uint8_t *data,
			uint16_t wLength)
{
	return usb_control_msg(udev,
			      usb_sndctrlpipe(udev, 0),
			      0x00,
			      USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			      0, 0, data, wLength, CTRL_TIMEOUT);
}

static int kinect_read(struct usb_device *udev, uint8_t *data, uint16_t wLength)
{
	return usb_control_msg(udev,
			      usb_rcvctrlpipe(udev, 0),
			      0x00,
			      USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			      0, 0, data, wLength, CTRL_TIMEOUT);
}

static int send_cmd(struct gspca_dev *gspca_dev, uint16_t cmd, void *cmdbuf,
		unsigned int cmd_len, void *replybuf, unsigned int reply_len)
{
	struct sd *sd = (struct sd *) gspca_dev;
	struct usb_device *udev = gspca_dev->dev;
	int res, actual_len;
	uint8_t *obuf = sd->obuf;
	uint8_t *ibuf = sd->ibuf;
	struct cam_hdr *chdr = (void *)obuf;
	struct cam_hdr *rhdr = (void *)ibuf;

	if (cmd_len & 1 || cmd_len > (OUTPUT_BUFFER_SIZE - sizeof(*chdr))){
		pr_err("send_cmd: Invalid command length (0x%x)\n", cmd_len);
		return -1;
	}

	chdr->magic[0] = SEND_MAGIC_0;
	chdr->magic[1] = SEND_MAGIC_1;
	chdr->cmd = cpu_to_le16(cmd);
	chdr->tag = cpu_to_le16(sd->cam_tag);
	chdr->len = cpu_to_le16(cmd_len / 2);

	memcpy(obuf+sizeof(*chdr), cmdbuf, cmd_len);

	res = kinect_write(udev, obuf, cmd_len + sizeof(*chdr));
	PDEBUG(D_USBO, "Control cmd=%04x tag=%04x len=%04x: %d", cmd,
		sd->cam_tag, cmd_len, res);
	if (res < 0) {
		pr_err("send_cmd: Output control transfer failed (%d)\n", res);
		return res;
	}

	do {
		actual_len = kinect_read(udev, ibuf, INPUT_BUFFER_SIZE);
	} while (actual_len == 0);
	PDEBUG(D_USBO, "Control reply: %d", res);
	if (actual_len < sizeof(*rhdr)) {
		pr_err("send_cmd: Input control transfer failed (%d)\n", res);
		return res;
	}
	actual_len -= sizeof(*rhdr);

	if (rhdr->magic[0] != 0x52 || rhdr->magic[1] != 0x42) {
		pr_err("send_cmd: Bad magic %02x %02x\n",
		       rhdr->magic[0], rhdr->magic[1]);
		return -1;
	}
	if (rhdr->cmd != chdr->cmd) {
		pr_err("send_cmd: Bad cmd %02x != %02x\n",
		       rhdr->cmd, chdr->cmd);
		return -1;
	}
	if (rhdr->tag != chdr->tag) {
		pr_err("send_cmd: Bad tag %04x != %04x\n",
		       rhdr->tag, chdr->tag);
		return -1;
	}
	if (cpu_to_le16(rhdr->len) != (actual_len/2)) {
		pr_err("send_cmd: Bad len %04x != %04x\n",
		       cpu_to_le16(rhdr->len), (int)(actual_len/2));
		return -1;
	}

	if (actual_len > reply_len) {
		pr_warn("send_cmd: Data buffer is %d bytes long, but got %d bytes\n",
			reply_len, actual_len);
		memcpy(replybuf, ibuf+sizeof(*rhdr), reply_len);
	} else {
		memcpy(replybuf, ibuf+sizeof(*rhdr), actual_len);
	}

	sd->cam_tag++;

	return actual_len;
}

static int write_register(struct gspca_dev *gspca_dev, uint16_t reg,
			uint16_t data)
{
	uint16_t reply[2];
	uint16_t cmd[2];
	int res;

	cmd[0] = cpu_to_le16(reg);
	cmd[1] = cpu_to_le16(data);

	PDEBUG(D_USBO, "Write Reg 0x%04x <= 0x%02x", reg, data);
	res = send_cmd(gspca_dev, 0x03, cmd, 4, reply, 4);
	if (res < 0)
		return res;
	if (res != 2) {
		pr_warn("send_cmd returned %d [%04x %04x], 0000 expected\n",
			res, reply[0], reply[1]);
	}
	return 0;
}

/* this function is called at probe time */
static int sd_config(struct gspca_dev *gspca_dev,
		     const struct usb_device_id *id)
{
	struct sd *sd = (struct sd *) gspca_dev;
	struct cam *cam;

	sd->cam_tag = 0;

	/* video has stream flag = 0x80 
	 * depth has stream flag = 0x70 */
	sd->stream_flag = STREAM_FLAG;

	cam = &gspca_dev->cam;

	cam->cam_mode = video_camera_mode;
	cam->nmodes = ARRAY_SIZE(video_camera_mode);

#if 0
	/* Setting those values is not needed for video stream 
	 * pkt_size = 1920 for video
	 * pkt_size = 1760 for depth */
	cam->npkt = 15;
	gspca_dev->pkt_size = PKT_SIZE;
#endif

	return 0;
}

/* this function is called at probe and resume time */
static int sd_init(struct gspca_dev *gspca_dev)
{
	PDEBUG(D_PROBE, "Kinect Camera device.");

	return 0;
}

static int sd_start(struct gspca_dev *gspca_dev)
{
	/* Disable auto-cycle of projector */
	write_register(gspca_dev, 0x105, 0x00);
	/* reset depth stream */
	write_register(gspca_dev, 0x06, 0x00);
	/* Depth Stream Format 0x03: 11 bit stream | 0x02: 10 bit */
	write_register(gspca_dev, 0x12, 0x02);
	/* Depth Stream Resolution 1: standard (640x480) */
	write_register(gspca_dev, 0x13, 0x01); 
	/* Depth Framerate / 0x1e (30): 30 fps */
	write_register(gspca_dev, 0x14, 0x1e);
	/* Depth Stream Control  / 2: Open Depth Stream */
	write_register(gspca_dev, 0x06, 0x02);	
	/* disable depth hflip / LSB = 0: Smoothing Disabled */
	write_register(gspca_dev, 0x17, 0x00);	

	return 0;
}

static void sd_stopN(struct gspca_dev *gspca_dev)
{
	/* reset depth stream */
	write_register(gspca_dev, 0x06, 0x00);
}

static void sd_pkt_scan(struct gspca_dev *gspca_dev, u8 *__data, int len)
{
	struct sd *sd = (struct sd *) gspca_dev;

	struct pkt_hdr *hdr = (void *)__data;
	uint8_t *data = __data + sizeof(*hdr);
	int datalen = len - sizeof(*hdr);
	/* frame:   |new||current||end of frame|
	 * for video 0x81 0x82 and 0x85 
	 * for depth 0x71 0x72 and 0x75 */
	uint8_t sof = sd->stream_flag | 1;
	uint8_t mof = sd->stream_flag | 2;
	uint8_t eof = sd->stream_flag | 5;

	if (len < 12)
		return;

	if (hdr->magic[0] != RECEIVE_MAGIC_0 || hdr->magic[1] != RECEIVE_MAGIC_1) {
		pr_warn("[Stream %02x] Invalid magic %02x%02x\n",
			sd->stream_flag, hdr->magic[0], hdr->magic[1]);
		return;
	}

	if (hdr->flag == sof)
		gspca_frame_add(gspca_dev, FIRST_PACKET, data, datalen);

	else if (hdr->flag == mof)
		gspca_frame_add(gspca_dev, INTER_PACKET, data, datalen);

	else if (hdr->flag == eof)
		gspca_frame_add(gspca_dev, LAST_PACKET, data, datalen);

	else
		pr_warn("Packet type not recognized...\n");
}

/* sub-driver description */
static const struct sd_desc sd_desc = {
	.name      = MODULE_NAME,
	.config    = sd_config,
	.init      = sd_init,
	.start     = sd_start,
	.stopN     = sd_stopN,
	.pkt_scan  = sd_pkt_scan,
	/*
	.get_streamparm = sd_get_streamparm,
	.set_streamparm = sd_set_streamparm,
	*/
};

/* -- module initialisation -- */
static const struct usb_device_id device_table[] = {
	{USB_DEVICE(0x045e, 0x02ae)},
	{USB_DEVICE(0x045e, 0x02bf)},
	{}
};

MODULE_DEVICE_TABLE(usb, device_table);

/* -- device connect -- */
static int sd_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	return gspca_dev_probe(intf, id, &sd_desc, sizeof(struct sd),
				THIS_MODULE);
}

static struct usb_driver sd_driver = {
	.name       = MODULE_NAME,
	.id_table   = device_table,
	.probe      = sd_probe,
	.disconnect = gspca_disconnect,
#ifdef CONFIG_PM
	.suspend    = gspca_suspend,
	.resume     = gspca_resume,
	.reset_resume = gspca_resume,
#endif
};

module_usb_driver(sd_driver);
