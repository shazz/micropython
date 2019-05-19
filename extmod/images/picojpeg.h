//------------------------------------------------------------------------------
// picojpeg - Public domain, Rich Geldreich <richgel99@gmail.com>
// Oct. 16, 2018 - Made reentrant, added scanline output, added RGB565 output
// Changes from Scott Wagner <scott.wagner@promethean-design.com>
//------------------------------------------------------------------------------
#ifndef PICOJPEG_H
#define PICOJPEG_H

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
enum
{
   PJPG_OK = 0,
   PJPG_NO_MORE_BLOCKS = 1,
   PJPG_BAD_DHT_COUNTS,
   PJPG_BAD_DHT_INDEX,
   PJPG_BAD_DHT_MARKER,
   PJPG_BAD_DQT_MARKER,
   PJPG_BAD_DQT_TABLE,
   PJPG_BAD_PRECISION,
   PJPG_BAD_HEIGHT,
   PJPG_BAD_WIDTH,
   PJPG_TOO_MANY_COMPONENTS,
   PJPG_BAD_SOF_LENGTH,
   PJPG_BAD_VARIABLE_MARKER,
   PJPG_BAD_DRI_LENGTH,
   PJPG_BAD_SOS_LENGTH,
   PJPG_BAD_SOS_COMP_ID,
   PJPG_W_EXTRA_BYTES_BEFORE_MARKER,
   PJPG_NO_ARITHMITIC_SUPPORT,
   PJPG_UNEXPECTED_MARKER,
   PJPG_NOT_JPEG,
   PJPG_UNSUPPORTED_MARKER,
   PJPG_BAD_DQT_LENGTH,
   PJPG_TOO_MANY_BLOCKS,
   PJPG_UNDEFINED_QUANT_TABLE,
   PJPG_UNDEFINED_HUFF_TABLE,
   PJPG_NOT_SINGLE_SCAN,
   PJPG_UNSUPPORTED_COLORSPACE,
   PJPG_UNSUPPORTED_SAMP_FACTORS,
   PJPG_DECODE_ERROR,
   PJPG_BAD_RESTART_MARKER,
   PJPG_ASSERTION_ERROR,
   PJPG_BAD_SOS_SPECTRAL,
   PJPG_BAD_SOS_SUCCESSIVE,
   PJPG_STREAM_READ_ERROR,
   PJPG_NOTENOUGHMEM,
   PJPG_UNSUPPORTED_COMP_IDENT,
   PJPG_UNSUPPORTED_QUANT_TABLE,
   PJPG_UNSUPPORTED_MODE,        // picojpeg doesn't support progressive JPEG's
};  

// Scan types
typedef enum
{
   PJPG_GRAYSCALE,
   PJPG_YH1V1,
   PJPG_YH2V1,
   PJPG_YH1V2,
   PJPG_YH2V2
} pjpeg_scan_type_t;

// Output types
typedef enum
{
   PJPG_GRAY8,
   PJPG_RGB888,
   PJPG_RGB565,
   PJPG_REDUCED_GRAY8,
   PJPG_REDUCED_RGB888,
   PJPG_REDUCED_RGB565,
} pjpeg_output_type_t;

typedef struct
{
   // Image resolution
   int m_width;
   int m_height;
   
   // Number of components (1 or 3)
   int m_comps;
   
   // Total number of minimum coded units (MCU's) per row/col.
   int m_MCUSPerRow;
   int m_MCUSPerCol;
   
   // Scan type
   pjpeg_scan_type_t m_scanType;
   
   // MCU width/height in pixels (each is either 8 or 16 depending on the scan type)
   int m_MCUWidth;
   int m_MCUHeight;

   // Selected output type, corrected for image type detected
   pjpeg_output_type_t m_outputType;

   // Handle to internal data structure
   void *m_PJHandle;

   // Line buffer for m_MCUHeight output lines
   void *m_linebuf;

   // m_pMCUBufR, m_pMCUBufG, and m_pMCUBufB are pointers to internal MCU Y or RGB pixel component buffers.
   // Each time pjpegDecodeMCU() is called successfully these buffers will be filled with 8x8 pixel blocks of Y or RGB pixels.
   // Each MCU consists of (m_MCUWidth/8)*(m_MCUHeight/8) Y/RGB blocks: 1 for greyscale/no subsampling, 2 for H1V2/H2V1, or 4 blocks for H2V2 sampling factors. 
   // Each block is a contiguous array of 64 (8x8) bytes of a single component: either Y for grayscale images, or R, G or B components for color images.
   //
   // The 8x8 pixel blocks are organized in these byte arrays like this:
   //
   // PJPG_GRAYSCALE: Each MCU is decoded to a single block of 8x8 grayscale pixels. 
   // Only the values in m_pMCUBufR are valid. Each 8 bytes is a row of pixels (raster order: left to right, top to bottom) from the 8x8 block.
   //
   // PJPG_H1V1: Each MCU contains is decoded to a single block of 8x8 RGB pixels.
   //
   // PJPG_YH2V1: Each MCU is decoded to 2 blocks, or 16x8 pixels.
   // The 2 RGB blocks are at byte offsets: 0, 64
   //
   // PJPG_YH1V2: Each MCU is decoded to 2 blocks, or 8x16 pixels. 
   // The 2 RGB blocks are at byte offsets: 0, 
   //                                       128
   //
   // PJPG_YH2V2: Each MCU is decoded to 4 blocks, or 16x16 pixels.
   // The 2x2 block array is organized at byte offsets:   0,  64, 
   //                                                   128, 192
   //
   // It is up to the caller to copy or blit these pixels from these buffers into the destination bitmap.
   unsigned char *m_pMCUBufR;
   unsigned char *m_pMCUBufG;
   unsigned char *m_pMCUBufB;
} pjpeg_image_info_t;

typedef int (jsread_t)(void *buffer, int size, void *pCallback_data);
typedef int (jswrite_t)(void *buffer, int size, void *pCallback_data);

// Initializes the decompressor. Returns 0 on success, or one of the above error codes on failure.
// jsread will be called to fill the decompressor's internal input buffer.
// If reduced output type, only the first pixel pjpeg_output_type_t output_typeof each block will
// be decoded. This mode is much faster because it skips the AC dequantization, IDCT and chroma
// upsampling of every image pixel.
// If RGB565 output type, data format is 2 bytes: |RRRRRGGG|GGGBBBBB|
unsigned char pjpeg_decode_init(pjpeg_image_info_t *pInfo, jsread_t *jsread, void *pCallback_data,
      void *storage, pjpeg_output_type_t output_type);

// Specify window of input image to decompress.  If the window size is greater than the image size,
// then the image will decompressed to the center of the window, and bordered in black.  If the window
// size is less than the image size, then the image will be cropped to fit into the window.  If upper
// or left corner offset parameter is negative, then the image in that direction will be cropped from
// the center.
// This method must be called after pjpeg_decode_init() and before decoding begins.
unsigned char pjpeg_set_window(pjpeg_image_info_t *pInfo, unsigned int width_pixels,
      unsigned int height_pixels, int left_offset_pixels, int top_offset_pixels);

// Decompresses the file's next MCU. Returns 0 on success, PJPG_NO_MORE_BLOCKS if no more blocks
// are available, or an error code.
// Must be called a total of m_MCUSPerRow*m_MCUSPerCol times to completely decompress the image.
unsigned char pjpeg_decode_mcu(pjpeg_image_info_t *pInfo);

// Decompresses file and passes scan lines to callback method jswrite.  pCallbackData is
// a client data pointer passed back to jswrite.  Note that pInfo->m_linebuf must point to
// a client-supplied buffer of size determined by pjpeg_get_line_buffer_size() after
// peg_decode_init() is called!
unsigned char pjpeg_decode_scanlines(pjpeg_image_info_t *pInfo, jswrite_t *jswrite, void *pCallback_data);

// Gets the size of the internal storage needed by pjpeg.  Client is responsible for allocating
// and managing this storage.
unsigned int pjpeg_get_storage_size(void);

// Gets the size of the line buffer storage needed for scanlines.  Allocation size is
// <image_width_pixels>*<MCU_height_lines>*<output_bytes_per_pixel>.  Client is responsible
// for allocating and managing this storage.
unsigned int pjpeg_get_line_buffer_size(pjpeg_image_info_t *pInfo);

#ifdef __cplusplus
}
#endif

#endif // PICOJPEG_H
