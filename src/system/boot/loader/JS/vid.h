#ifndef __VID_H
#define __VID_H


#include "list.h"

//#include <status.h>
#include "types.h"

/** Video mode types (defined to match Initium types). */
typedef enum video_mode_type {
        VIDEO_MODE_VGA = (1<<0),                 /**< VGA. */
        VIDEO_MODE_LFB = (1<<1),                 /**< Linear framebuffer. */
} video_mode_type_t;

/** Tag containing video mode information. */
typedef struct video_mode {
        list_t header;                  /**< Link to mode list. */

        video_mode_type_t type;         /**< Type of the video mode. */
        const struct video_ops *ops;    /**< Operations for the video mode. */

        /** Common information. */
        uint32_t width;                 /**< LFB pixel width/VGA number of columns. */
        uint32_t height;                /**< LFB pixel height/VGA number of rows. */
        phys_ptr_t mem_phys;            /**< Physical address of LFB/VGA memory. */
        ptr_t mem_virt;                 /**< Loader virtual address of LFB/VGA memory. */
        uint32_t mem_size;              /**< Size of LFB/VGA memory. */

        union {
                /** VGA information. */
                struct {
                        /** Cursor position information, stored in case OS wants it. */
                        uint8_t x;      /**< Cursor X position. */
                        uint8_t y;      /**< Cursor Y position. */
                };

                /** Linear framebuffer information. */
                struct {
                        uint8_t bpp;    /**< Number of bits per pixel. */
                        uint32_t pitch; /**< Number of bytes per line of the framebuffer. */
                        uint8_t red_size; /**< Size of red component of each pixel. */
                        uint8_t red_pos; /**< Bit position of the red component of each pixel. */
                        uint8_t green_size; /**< Size of green component of each pixel. */
                        uint8_t green_pos; /**< Bit position of the green component of each pixel. */
                        uint8_t blue_size; /**< Size of blue component of each pixel. */
                        uint8_t blue_pos; /**< Bit position of the blue component of each pixel. */
                };
        };
} video_mode_t;


extern video_mode_t *current_video_mode;

#endif
