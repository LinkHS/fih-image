#ifndef __GIMP_PIXEL_RGN_H__
#define __GIMP_PIXEL_RGN_H__

#include "TypeDefines.h"

struct _HopePixelRgn
{
  //uchar       *data;          /* pointer to region data */
  //GimpDrawable *drawable;      /* pointer to drawable */
  int          bpp;           /* bytes per pixel */
  int          rowstride;     /* bytes per pixel row */
  int          x, y;          /* origin */
  int          w, h;          /* width and height of region */
  uint         dirty : 1;     /* will this region be dirtied? */
  uint         shadow : 1;    /* will this region use the shadow or normal tiles */
  //int          process_count; /* used internally */
};

typedef struct _HopePixelRgn HopePixelRgn;

#endif
