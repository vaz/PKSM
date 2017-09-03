#pragma once

#include "common.h"
#include "quirc/quirc.h"

bool camera_get_mode();
void camera_init();
void camera_exit();
void camera_picture();
void camera_draw_qr();