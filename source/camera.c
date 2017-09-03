#include "camera.h"

u8* cameraBuffer;
struct quirc* context;
bool qr_mode;

bool camera_get_mode() {
	return qr_mode;
}

void camera_init() {
	camInit();
	CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A);
	CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A);

	CAMU_SetNoiseFilter(SELECT_OUT1, true);
	CAMU_SetAutoExposure(SELECT_OUT1, true);
	CAMU_SetAutoWhiteBalance(SELECT_OUT1, true);

	CAMU_SetTrimming(PORT_CAM1, false);

	cameraBuffer = malloc(sizeof(u16) * 400 * 240);

	context = quirc_new();
	quirc_resize(context, 400, 240);
	
	qr_mode = true;
}

void camera_exit() {
	CAMU_Activate(SELECT_NONE);
	camExit();
	quirc_destroy(context);
	free(cameraBuffer);
	
	qr_mode = false;
}

void camera_picture() {
	u32 transfer_size;
	Handle cam_handle = 0;
	CAMU_GetMaxBytes(&transfer_size, 400, 240);
	CAMU_SetTransferBytes(PORT_CAM1, transfer_size, 400, 240);
	CAMU_Activate(SELECT_OUT1);
	CAMU_ClearBuffer(PORT_CAM1);
	CAMU_StartCapture(PORT_CAM1);
	CAMU_SetReceiving(&cam_handle, cameraBuffer, PORT_CAM1, 400 * 240 * 2, transfer_size);
	svcWaitSynchronization(cam_handle, U64_MAX);
	CAMU_StopCapture(PORT_CAM1);
	svcCloseHandle(cam_handle);
	CAMU_Activate(PORT_NONE);
}

void camera_draw_qr() {
    pp2d_free_texture(TEXTURE_QR);
    camera_picture();
    u32 *rgba8_buf = malloc(240 * 400 * sizeof(u32));
    for (int i = 0; i < 240 * 400; i++)
    {
        rgba8_buf[i] = RGB565_TO_RGBA8(cameraBuffer[i]);
        u8 *byte_pointer = (u8*)&rgba8_buf[i];
        u8 r = *(byte_pointer+3);
        u8 b = *(byte_pointer+2);
        u8 g = *(byte_pointer+1);
        u8 a = *(byte_pointer);
        rgba8_buf[i] = RGBA8(r, g, b, a);
    }
    pp2d_load_texture_memory(TEXTURE_QR, rgba8_buf, 400, 240);
    pp2d_draw_texture(TEXTURE_QR, 0, 0);
    free(rgba8_buf);

    int w;
    int h;

    u8 *image = (u8*) quirc_begin(context, &w, &h);

    for (ssize_t x = 0; x < w; x++)
    {
        for (ssize_t y = 0; y < h; y++)
        {
            u16 px = cameraBuffer[y * 400 + x];
            image[y * w + x] = (u8)(((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
        }
    }

    quirc_end(context);

    if (quirc_count(context) > 0)
    {
        struct quirc_code code;
        struct quirc_data data;
        quirc_extract(context, 0, &code);
        if (!quirc_decode(&code, &data))
        {
            qr_mode = false;
        }
    }
}