#define STBIW_WINDOWS_UTF8

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*! \brief Incoming arguments */
enum {
    ARG_PROGRAM, /**< Program name                      */
    ARG_PALETTE, /**< Palette file                      */
    ARG_PICTURE, /**< Target file                       */
    ARG_COLOR,   /**< Marking color as 0xXXXXXX         */
    ARG_OUTPUT,  /**< Output filename                   */
    
    ARGS_COUNT   /**< <automatic> Number of arguments   */
};

static const uint32_t color_channels = 4; /**< Number of channels used */

uint32_t * palette_data = NULL; /**< Palette's pixel data     */
uint32_t   palette_size = 0;    /**< Palette's size in pixels */

uint32_t * picture_data   = NULL; /**< Picture's pixel data     */
uint32_t   picture_width  = 0;    /**< Picture's width size     */
uint32_t   picture_height = 0;    /**< Picture's height size    */

uint32_t   color_marking = 0;     /**< Bad pixel marking color  */

const char * output_filename = NULL; /**< Output image filename */

/*! \brief Memory cleanup */
static void _free(void) __attribute__((destructor));
static void _free(void) {
    stbi_image_free(palette_data);
    stbi_image_free(picture_data);
}

/*! \brief Checks the correctness of the incoming arguments
 * \param[in] argc Number of arguments
 * \param[in] argv List of arguments */
static void _args_check(int argc, char * argv[]) {
    if (argc != ARGS_COUNT) {
        fprintf(
            stderr, 
            "Wrong usage, needs:\n"
            "%s <palette file> <picture file> <marking color>\n",
            argv[0]
        );
        exit(EXIT_FAILURE);
    }
}

/*! \brief Sets flags to STBI subsystem */
static void _stbi_prepare(void) {
    stbi_set_flip_vertically_on_load(true);
    stbi_flip_vertically_on_write   (true);
}

/*! \brief Loads the palette into global variable
 * \param[in] filename Palette's image filename */
static void _palette_load(const char * filename) {
    uint32_t width;
    uint32_t height;
    uint32_t chan;
    
    palette_data = (uint32_t *)stbi_load(
        filename, 
        &width, 
        &height, 
        &chan, 
        color_channels
    );
    if (!palette_data) {
        fprintf(
            stderr, 
            "Cannot load palette file \"%s\",\n"
            "requires to be STBI supported image format\n",
            filename
        );
        exit(EXIT_FAILURE);
    }
    
    palette_size = width * height;
}

/*! \brief Loads the picture into global variable
 * \param[in] filename Picture's image filename */
static void _picture_load(const char * filename) {
    uint32_t chan;
    picture_data = (uint32_t *)stbi_load(
        filename, 
        &picture_width, 
        &picture_height, 
        &chan, 
        color_channels
    );
    if (!picture_data) {
        fprintf(
            stderr, 
            "Cannot load picture file \"%s\",\n"
            "requires to be STBI supported image format\n",
            filename
        );
        exit(EXIT_FAILURE);
    }
}

/*! \brief Converts string definition of the marking color into the uint32_t
 * \param[in] string String color's definition */
static void _color_load(const char * string) {
    sscanf(string, "%x", &color_marking);
    // By some reason the G and B channels are swapped on saving,
    // let's swap them now to solve it.
    color_marking =  color_marking & 0xff0000ff       | // R  A
                    (color_marking & 0x00ff0000) >> 8 | //  G
                    (color_marking & 0x0000ff00) << 8;  //   B
}

/*! \brief Saves the output filename
 * \param[in] filename Output's image filename */
static void _output_load(const char * filename) {
    output_filename = filename;
}

/*! \brief Applies transferred arguments to global variables
 * \param[in] argv List of arguments */
static void _args_apply(char * argv[]) {
    _stbi_prepare();
    
    _palette_load(argv[ARG_PALETTE]);
    _picture_load(argv[ARG_PICTURE]);
    _color_load  (argv[ARG_COLOR  ]);
    _output_load (argv[ARG_OUTPUT ]);
}

/*! \brief Checks if the selected pixel color is contained in the palette
 * \param[in] pixel Color
 * \return True if the selected color is contained in the palette */
static bool _pixel_isinpalette(uint32_t pixel) {
    uint32_t * pdata = palette_data;
    
    for (uint32_t i = 0; i < palette_size; i++) {
        if (*pdata == pixel)
            return true;
        
        pdata++;
    }
    
    return false;
}

/*! \brief Replaces the target pixel with the marking color
 * \param[in] ptr Pointer to the image's pixel */
static void _pixel_replace(uint32_t * ptr) {
    *ptr = color_marking;
}

/*! \brief Processes the image pixels */
static void _image_process(void) {
    uint32_t * pdata = picture_data;
    
    for (uint32_t y = 0; y < picture_height; y++)
    for (uint32_t x = 0; x < picture_width;  x++) {
        if (!_pixel_isinpalette(*pdata))
            _pixel_replace(pdata);
            
        pdata++;
    }
}

/*! \brief Saves the resulting picture to the disk */
static void _result_save(void) {
    if (stbi_write_png(
            output_filename,
            picture_width,
            picture_height,
            color_channels,
            picture_data,
            picture_width * color_channels
        ) == 0) {
        fprintf(
            stderr, 
            "Cannot save picture file to \"%s\",\n",
            output_filename
        );
    }
}

int main(int argc, char * argv[]) {
    _args_check(argc, argv);
    _args_apply(      argv);
    
    _image_process();
    _result_save  ();
    
    return EXIT_SUCCESS;
}
