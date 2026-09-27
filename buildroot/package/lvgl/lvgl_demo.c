/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <linux/fb.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "lvgl.h"
#include "demos/lv_demos.h"

static struct fb_var_screeninfo var;
static struct fb_fix_screeninfo fix;
static unsigned char *pixels;
static volatile sig_atomic_t running = 1;

static void stop(int signo)
{
    (void)signo;
    running = 0;
}

static uint32_t channel(uint8_t value, struct fb_bitfield field)
{
    if (!field.length) return 0;
    return ((uint32_t)value >> (8 - field.length)) << field.offset;
}

static void flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colors)
{
    const unsigned bytes = var.bits_per_pixel / 8;
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++, colors++) {
            if (x < 0 || y < 0 || x >= (int)var.xres || y >= (int)var.yres)
                continue;
            lv_color32_t rgb;
            rgb.full = lv_color_to32(*colors);
            uint32_t pixel = channel(rgb.ch.red, var.red) |
                             channel(rgb.ch.green, var.green) |
                             channel(rgb.ch.blue, var.blue) |
                             channel(255, var.transp);
            size_t offset = (size_t)(y + var.yoffset) * fix.line_length +
                            (size_t)(x + var.xoffset) * bytes;
            /* Both SG200X toolchains target little-endian CPUs. */
            for (unsigned b = 0; b < bytes; b++)
                pixels[offset + b] = (unsigned char)(pixel >> (8 * b));
        }
    }
    lv_disp_flush_ready(drv);
}

static int valid_field(struct fb_bitfield field)
{
    return field.length <= 8 && field.offset <= var.bits_per_pixel &&
           field.length <= var.bits_per_pixel - field.offset && !field.msb_right;
}

static uint64_t milliseconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int main(int argc, char **argv)
{
    const char *demo = argc > 1 ? argv[1] : "widgets";
    const char *device = argc > 2 ? argv[2] : "/dev/fb0";
    if (argc > 3 || (strcmp(demo, "widgets") && strcmp(demo, "benchmark") &&
                     strcmp(demo, "stress"))) {
        fprintf(stderr, "Usage: %s [widgets|benchmark|stress] [/dev/fbN]\n", argv[0]);
        return EXIT_FAILURE;
    }
    int fd = open(device, O_RDWR);
    if (fd < 0) { perror(device); return EXIT_FAILURE; }
    int result = EXIT_FAILURE;
    lv_color_t *buffer = NULL;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0 ||
        ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        perror("framebuffer info");
        goto close_fb;
    }
    unsigned bytes = var.bits_per_pixel / 8;
    if ((var.bits_per_pixel != 16 && var.bits_per_pixel != 24 &&
         var.bits_per_pixel != 32) || fix.type != FB_TYPE_PACKED_PIXELS ||
        fix.visual != FB_VISUAL_TRUECOLOR || var.nonstd || var.grayscale ||
        !valid_field(var.red) || !valid_field(var.green) ||
        !valid_field(var.blue) || !valid_field(var.transp) ||
        !var.red.length || !var.green.length || !var.blue.length ||
        !var.xres || !var.yres || var.xres > 32767 || var.yres > 32767 ||
        ((uint64_t)var.xoffset + var.xres) * bytes > fix.line_length ||
        ((uint64_t)var.yoffset + var.yres) * fix.line_length > fix.smem_len) {
        fprintf(stderr, "Unsupported framebuffer layout (need packed 16/24/32-bit truecolor)\n");
        goto close_fb;
    }
    pixels = mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pixels == MAP_FAILED) { perror("mmap"); goto close_fb; }
    buffer = calloc((size_t)var.xres * 40, sizeof(*buffer));
    if (!buffer) { perror("draw buffer"); goto unmap_fb; }
    lv_init();
    static lv_disp_draw_buf_t draw;
    static lv_disp_drv_t display;
    lv_disp_draw_buf_init(&draw, buffer, NULL, var.xres * 40);
    lv_disp_drv_init(&display);
    display.hor_res = var.xres;
    display.ver_res = var.yres;
    display.draw_buf = &draw;
    display.flush_cb = flush;
    if (!lv_disp_drv_register(&display)) {
        fprintf(stderr, "Cannot register LVGL display\n");
        goto unmap_fb;
    }
    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    printf("LVGL %s: %ux%u, %u bpp on %s (Ctrl+C to exit)\n",
           demo, var.xres, var.yres, var.bits_per_pixel, device);
    if (!strcmp(demo, "benchmark")) lv_demo_benchmark();
    else if (!strcmp(demo, "stress")) lv_demo_stress();
    else lv_demo_widgets();
    uint64_t previous = milliseconds();
    while (running) {
        uint64_t now = milliseconds();
        lv_tick_inc((uint32_t)(now - previous));
        previous = now;
        uint32_t delay = lv_timer_handler();
        if (delay > 20) delay = 20;
        if (delay < 1) delay = 1;
        struct timespec pause = {0, (long)delay * 1000000};
        nanosleep(&pause, NULL);
    }
    result = EXIT_SUCCESS;
unmap_fb:
    free(buffer);
    munmap(pixels, fix.smem_len);
close_fb:
    close(fd);
    return result;
}
