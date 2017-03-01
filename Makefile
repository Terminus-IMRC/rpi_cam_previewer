all: rpi_cam_previewer

CFLAGS := -I/opt/vc/include -pipe -Wall -Wextra -g -O2
LDLIBS := -L/opt/vc/lib

rpi_cam_previewer: rpi_cam_previewer.o
rpi_cam_previewer: LDLIBS += -lbcm_host -lvcos -lmmal_core -lmmal_util -lmmal_vc_client

.PHONY: clean
clean:
	$(RM) rpi_cam_previewer
	$(RM) rpi_cam_previewer.o
