//#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
//#error "Only <libgimp/gimp.h> can be included directly."
//#endif

#ifndef __GIMP_DRAWABLE_H__
#define __GIMP_DRAWABLE_H__

#include "hope.h"
//#include "opencv.hpp"

struct _HopeDrawable
{
  //int32    drawable_id;   /* drawable ID */
  uint     width;         /* width of drawable */
  uint     height;        /* height of drawable */
  uint     bpp;           /* bytes per pixel of drawable */
  uint     ntile_rows;    /* # of tile rows */
  uint     ntile_cols;    /* # of tile columns */
  //GimpTile *tiles;         /* the normal tiles */
  //GimpTile *shadow_tiles;  /* the shadow tiles */
};

typedef struct _HopeDrawable HopeDrawable;

#endif
