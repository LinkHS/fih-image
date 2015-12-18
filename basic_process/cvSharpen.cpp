#define LOG_TAG "FIH/cvSharpen"
#define DEBUG_LEVEL                 0

#include "includes.h"
#include "cvSharpen.h"
#include "porting/debug.h"

using namespace cv;

/*
 * Local functions...
 */

static void     compute_luts  (SharpenParams* sharpen_params);

static void     gray_filter  (int width, uchar *src, uchar *dst, intneg *neg0,
                              intneg *neg1, intneg *neg2);
static void     graya_filter (int width, uchar *src, uchar *dst, intneg *neg0,
                              intneg *neg1, intneg *neg2);
static void     rgb_filter   (int width, uchar *src, uchar *dst, intneg *neg0,
                              intneg *neg1, intneg *neg2);
static void     rgba_filter  (int width, uchar *src, uchar *dst, intneg *neg0,
                              intneg *neg1, intneg *neg2);


/*
 * Globals...
 */

static intneg neg_lut[256];   /* Negative coefficient LUT */
static intpos pos_lut[256];   /* Positive coefficient LUT */

static void
compute_luts (SharpenParams* sharpen_params)
{
  int i;       /* Looping var */
  int fact;    /* 1 - sharpness */

  fact = 100 - sharpen_params->sharpen_percent;
  if (fact < 1)
    fact = 1;

  for (i = 0; i < 256; i ++)
  {
    pos_lut[i] = 800 * i / fact;
    neg_lut[i] = (4 + pos_lut[i] - (i << 3)) >> 3;
  }
}

/*
 * 'sharpen()' - Sharpen an image using a convolution filter.
 */

void
cvSharpen (Mat& mSrc,            /* I - Source image region */ 
         Mat& mDst,              /* O - Destination image region */
         SharpenParams* params)  /* I - Sharpen Parameters */
{
  uchar       *src_rows[4];    /* Source pixel rows */
  uchar       *src_ptr;        /* Current source pixel */
  uchar       *dst_row;        /* Destination pixel row */
  intneg       *neg_rows[4];    /* Negative coefficient rows */
  intneg       *neg_ptr;        /* Current negative coefficient */
  int          i;              /* Looping vars */
  int          y;              /* Current location in image */
  int          row;            /* Current row in src_rows */
  int          count;          /* Current number of filled src_rows */
  int          width;          /* Byte width of the image */
  int          x1;             /* Selection bounds */
  int          y1;
  int          y2;
  int          sel_width;      /* Selection width */
  int          sel_height;     /* Selection height */
  int          img_bpp;        /* Bytes-per-pixel in image */
  void          (*filter)(int, uchar *, uchar *, intneg *, intneg *, intneg *);
  
  filter = NULL;
		
	if( params == NULL )
	{
		SharpenParams sharpen_params = { 40, NULL };
		params = &sharpen_params;
	}
	/*
	 * Check for drawable selection
	 */
	if( params->roi == NULL )
	{
		x1 = 0;
		y1 = 0;
		sel_width = mSrc.cols;
		sel_height = mSrc.rows;
	}
	else
	{
		x1 = params->roi->x;
		y1 = params->roi->y;
		sel_width = params->roi->w;
		sel_height = params->roi->h;
	}
	
  y2 = y1 + sel_height;

  img_bpp = mSrc.channels();
  width = sel_width * img_bpp;
	
  mDst.create(y2, sel_width, mSrc.type());
	
  /*
   * Setup for filter...
   */

  compute_luts (params);

  for (row = 0; row < 4; row ++)
  {
		//src_rows[row] = g_new (uchar, width);
		src_rows[row] = (uchar *)malloc(width*1);
		//neg_rows[row] = g_new (intneg, width);
		neg_rows[row] = (intneg *)malloc(width*sizeof(intneg));

		if( src_rows[row] == NULL || neg_rows[row] == NULL )
		{
FIH_LOGD("Error: malloc error\r\n");
			return ;
		}
  }

  //dst_row = g_new (uchar, width);
	dst_row = (uchar *)malloc(width*1);
	if( dst_row == NULL )
	{
FIH_LOGD("Error: malloc error\r\n");
		return ;
	}
	
  /*
   * Pre-load the first row for the filter...
   */

  //gimp_pixel_rgn_get_row (&src_rgn, src_rows[0], x1, y1, sel_width);
	memcpy (src_rows[0], &(mSrc.ptr<uchar>(y1)[x1]), width);
	
  for (i = width, src_ptr = src_rows[0], neg_ptr = neg_rows[0];
       i > 0;
       i --, src_ptr ++, neg_ptr ++)
    *neg_ptr = neg_lut[*src_ptr];

  row   = 1;
  count = 1;
	
  /*
   * Select the filter...
   */

  switch (img_bpp)
  {
    case 1 :
      filter = gray_filter;
      break;
    case 2 :
      filter = graya_filter;
      break;
    case 3 :
      filter = rgb_filter;
      break;
    case 4 :
      filter = rgba_filter;
      break;
  };

  /*
   * Sharpen...
   */

  for (y = y1; y < y2; y ++)
	{
	  /*
	   * Load the next pixel row...
	   */

	  if ((y + 1) < y2)
    {
      /*
       * Check to see if our src_rows[] array is overflowing yet...
       */

      if (count >= 3)
        count --;

      /*
       * Grab the next row...
       */

      //gimp_pixel_rgn_get_row (&src_rgn, src_rows[row], x1, y + 1, sel_width);
			memcpy (src_rows[row], &(mSrc.ptr<uchar>(y+1)[x1]), width);
      for (i = width, src_ptr = src_rows[row], neg_ptr = neg_rows[row];
           i > 0;
           i --, src_ptr ++, neg_ptr ++)
        *neg_ptr = neg_lut[*src_ptr];

      count ++;
      row = (row + 1) & 3;
    }
	  else
    {
      /*
       * No more pixels at the bottom...  Drop the oldest samples...
       */

      count --;
    }

	  /*
	   * Now sharpen pixels and save the results...
	   */

	  if (count == 3)
    {
      (* filter) (sel_width, src_rows[(row + 2) & 3], dst_row,
                  neg_rows[(row + 1) & 3] + img_bpp,
                  neg_rows[(row + 2) & 3] + img_bpp,
                  neg_rows[(row + 3) & 3] + img_bpp);

      /*
       * Set the row...
       */

      //gimp_pixel_rgn_set_row (&dst_rgn, dst_row, x1, y, sel_width);
			memcpy (&(mDst.ptr<uchar>(y)[x1]), dst_row, width);
    }
	  else if (count == 2)
    {
      if (y == y1)      /* first row */
        //gimp_pixel_rgn_set_row (&dst_rgn, src_rows[0], x1, y, sel_width);
			  memcpy (&(mDst.ptr<uchar>(y)[x1]), src_rows[0], width);
      else                  /* last row  */
        //gimp_pixel_rgn_set_row (&dst_rgn, src_rows[(sel_height - 1) & 3], x1, y, sel_width);
			  memcpy (&(mDst.ptr<uchar>(y)[x1]), src_rows[(sel_height - 1) & 3], width);
    }
	}

  /*
   * OK, we're done.  Free all memory used...
   */

  for (row = 0; row < 4; row ++)
  {
    free(src_rows[row]);
    free(neg_rows[row]);
  }

  free(dst_row);
}

/*
 * 'gray_filter()' - Sharpen grayscale pixels.
 */

static void
gray_filter (int    width,     /* I - Width of line in pixels */
             uchar *src,       /* I - Source line */
             uchar *dst,       /* O - Destination line */
             intneg *neg0,      /* I - Top negative coefficient line */
             intneg *neg1,      /* I - Middle negative coefficient line */
             intneg *neg2)      /* I - Bottom negative coefficient line */
{
  intpos pixel;         /* New pixel value */

  *dst++ = *src++;
  width -= 2;

  while (width > 0)
  {
    pixel = (pos_lut[*src++] - neg0[-1] - neg0[0] - neg0[1] -
             neg1[-1] - neg1[1] -
             neg2[-1] - neg2[0] - neg2[1]);
    pixel = (pixel + 4) >> 3;
    *dst++ = saturate_cast<uchar> (pixel);

    neg0 ++;
    neg1 ++;
    neg2 ++;
    width --;
  }

  *dst++ = *src++;
}

/*
 * 'graya_filter()' - Sharpen grayscale+alpha pixels.
 */

static void
graya_filter (int   width,     /* I - Width of line in pixels */
              uchar *src,      /* I - Source line */
              uchar *dst,      /* O - Destination line */
              intneg *neg0,     /* I - Top negative coefficient line */
              intneg *neg1,     /* I - Middle negative coefficient line */
              intneg *neg2)     /* I - Bottom negative coefficient line */
{
  intpos pixel;         /* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-2] - neg0[0] - neg0[2] -
               neg1[-2] - neg1[2] -
               neg2[-2] - neg2[0] - neg2[2]);
      pixel = (pixel + 4) >> 3;
      *dst++ = saturate_cast<uchar> (pixel);

      *dst++ = *src++;
      neg0 += 2;
      neg1 += 2;
      neg2 += 2;
      width --;
    }

  *dst++ = *src++;
  *dst++ = *src++;
}

/*
 * 'rgb_filter()' - Sharpen RGB pixels.
 */

static void
rgb_filter (int    width,      /* I - Width of line in pixels */
            uchar *src,        /* I - Source line */
            uchar *dst,        /* O - Destination line */
            intneg *neg0,       /* I - Top negative coefficient line */
            intneg *neg1,       /* I - Middle negative coefficient line */
            intneg *neg2)       /* I - Bottom negative coefficient line */
{
  intpos pixel;         /* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-3] - neg0[0] - neg0[3] -
               neg1[-3] - neg1[3] -
               neg2[-3] - neg2[0] - neg2[3]);
      pixel = (pixel + 4) >> 3;
      *dst++ = saturate_cast<uchar> (pixel);

      pixel = (pos_lut[*src++] - neg0[-2] - neg0[1] - neg0[4] -
               neg1[-2] - neg1[4] -
               neg2[-2] - neg2[1] - neg2[4]);
      pixel = (pixel + 4) >> 3;
      *dst++ = saturate_cast<uchar> (pixel);

      pixel = (pos_lut[*src++] - neg0[-1] - neg0[2] - neg0[5] -
               neg1[-1] - neg1[5] -
               neg2[-1] - neg2[2] - neg2[5]);
      pixel = (pixel + 4) >> 3;
      *dst++ = saturate_cast<uchar> (pixel);

      neg0 += 3;
      neg1 += 3;
      neg2 += 3;
      width --;
    }

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
}

/*
 * 'rgba_filter()' - Sharpen RGBA pixels.
 */

static void
rgba_filter (int   width,      /* I - Width of line in pixels */
             uchar *src,       /* I - Source line */
             uchar *dst,       /* O - Destination line */
             intneg *neg0,      /* I - Top negative coefficient line */
             intneg *neg1,      /* I - Middle negative coefficient line */
             intneg *neg2)      /* I - Bottom negative coefficient line */
{
  intpos pixel;         /* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-4] - neg0[0] - neg0[4] -
               neg1[-4] - neg1[4] -
               neg2[-4] - neg2[0] - neg2[4]);
      pixel = (pixel + 4) >> 3;
      *dst++ = saturate_cast<uchar> (pixel);

      pixel = (pos_lut[*src++] - neg0[-3] - neg0[1] - neg0[5] -
               neg1[-3] - neg1[5] -
               neg2[-3] - neg2[1] - neg2[5]);
      pixel = (pixel + 4) >> 3;
      *dst++ = saturate_cast<uchar> (pixel);

      pixel = (pos_lut[*src++] - neg0[-2] - neg0[2] - neg0[6] -
               neg1[-2] - neg1[6] -
               neg2[-2] - neg2[2] - neg2[6]);
      pixel = (pixel + 4) >> 3;
      *dst++ = saturate_cast<uchar> (pixel);

      *dst++ = *src++;

      neg0 += 4;
      neg1 += 4;
      neg2 += 4;
      width --;
    }

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
}

