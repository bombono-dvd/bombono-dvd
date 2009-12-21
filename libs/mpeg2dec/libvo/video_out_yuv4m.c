/* 
 *  video_out_yuv4m.c
 *
 *	Copyright (C) Rainer Johanni, Andrew Stevens Jan 2001
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *	
 *  mpeg2dec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  mpeg2dec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "config.h"

#ifdef LIBVO_MJPEGTOOLS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "mpeg2.h"
#include "video_out.h"
/* #include "video_out_internal.h" */

typedef struct yuv_instance_s {
    vo_instance_t vo;
/*    int prediction_index; */
/*    vo_frame_t * frame_ptr[3];
    vo_frame_t frame[3]; */
    int width;
    int height;
	int owidth;
	int oheight;
	int omargin;
	int8_t *image_data;
    int framenum;
	FILE *out_strm;
	y4m_stream_info_t *strm_info;
} yuv_instance_t;

extern vo_instance_t yuv_vo_instance;
extern vo_instance_t yuvh_vo_instance;
extern vo_instance_t yuvs_vo_instance;

static int yuv_core_setup (yuv_instance_t *this,
						   unsigned int width, unsigned int height,
                           unsigned int chroma_width, unsigned int chroma_height, 
                           vo_setup_result_t * result,
						   void (* draw_frame) (vo_instance_t * instance,
                                                uint8_t * const * base, void* id) 
                           )
{
	this->framenum = -2;
    this->vo.setup_fbuf = NULL;
    this->vo.set_fbuf   = NULL;
    this->vo.start_fbuf = NULL;
    this->vo.discard = NULL;
    result->convert = NULL;

    this->vo.draw = draw_frame;
	this->vo.close = (void (*) (vo_instance_t *)) free;

	this->height = height;
	this->width = width;
	this->out_strm = stdout;

    this->strm_info = malloc(sizeof(y4m_stream_info_t));
    if( this->strm_info == NULL )
		return 1;

    if( (height/2 != chroma_height) || (width/2 != chroma_width) ) 
    {
        fprintf(stderr,"Source is not 4:2:0. Exit.\n");
        exit(1);
    }
        

    y4m_init_stream_info(this->strm_info);
    return 0;
}

int yuv_core_setup2(vo_instance_t * instance, const struct mpeg2_info_s * info)
{
    int display_aspect_code;
    char buf[20];
    yuv_instance_t *this = (yuv_instance_t *) instance;
    y4m_ratio_t y4m_frame_rate;
    int i;

    static unsigned int frame_period[16] = {
    0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000,
    /* unofficial: xing 15 fps */
    1800000,
    /* unofficial: libmpeg3 "Unofficial economy rates" 5/10/12/15 fps */
    5400000, 2700000, 2250000, 1800000, 0, 0
    };

    /* 1 - frame rate */
    int frame_rate = 0;
    for(i=0; i<16; i++)
        if( frame_period[i] == info->sequence->frame_period )
        {
          frame_rate = i;
          break;
        }

    y4m_frame_rate = mpeg_framerate( frame_rate );
	y4m_si_set_framerate(this->strm_info, y4m_frame_rate);

	fprintf( stderr, "Frame rate code = %d\n", frame_rate );

    /* 2 - chroma - wont take other variants */
    y4m_si_set_chroma(this->strm_info, Y4M_CHROMA_420MPEG2);


    /* 3 - interlace */
    if( info->sequence->flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE )
        y4m_si_set_interlace(this->strm_info, Y4M_ILACE_NONE );
    else 
        /* because libmpeg2 sets interlace info in pictures only, 
         wont take it under consideration */
        y4m_si_set_interlace(this->strm_info, Y4M_ILACE_NONE);


    /* 4 - aspect
      we need to set up the pixel_* to y4m_si_set_sampleaspect(),
      not the whole frame aspect (like 4:3, 16:9 and so) 
    */
    y4m_frame_rate.n = info->sequence->pixel_width;
    y4m_frame_rate.d = info->sequence->pixel_height;
    display_aspect_code = mpeg_guess_mpeg_aspect_code(info->sequence->flags&SEQ_FLAG_MPEG2 ? 2 : 1,
                                                      y4m_frame_rate,
                                                      this->width, this->height);

    /*
    y4m_frame_rate.n = info->sequence->pixel_width*this->width;
    y4m_frame_rate.d = info->sequence->pixel_height*this->height;
    {
        int w = y4m_frame_rate.n;
        int h = y4m_frame_rate.d;
                     find greatest common divisor 
        while( w )
        {
            int t = w;
            w = h%w;
            h = t;
        }
        y4m_frame_rate.n /= h;
        y4m_frame_rate.d /= h;
    }
    */
    y4m_si_set_sampleaspect(this->strm_info, y4m_frame_rate);

	snprintf( buf, 19, "XM2AR%03d", display_aspect_code );
	y4m_xtag_add( y4m_si_xtags(this->strm_info), buf );

    return y4m_write_stream_header( fileno(this->out_strm), this->strm_info ) != Y4M_OK;
}

static void yuv_draw_frame (vo_instance_t * instance,
			    uint8_t * const * base, void* id)
{
	int res;
	yuv_instance_t *this = (yuv_instance_t *) instance;
	size_t n = 0;
	y4m_frame_info_t info;
	
	if (++(this->framenum) < 0 )
		return;
	y4m_init_frame_info(&info);
	res = y4m_write_frame_header(fileno(this->out_strm), this->strm_info, &info ); 
  	n += fwrite(base[0],
				1, this->width * this->height,  this->out_strm);
  	n += fwrite(base[1],
				1, this->width * this->height/4,  this->out_strm);
  	n += fwrite(base[2],
				1, this->width * this->height/4,  this->out_strm);
	fflush(this->out_strm);
 	if( res != Y4M_OK || (int)(n)!=this->width * this->height * 3/2)
  	{
    	fprintf(stderr,"Write of YUV output failed %d not written\n", this->width * this->height *3/2 - n);
   		exit(1);
  	}

}

static int vo_yuv_setup( vo_instance_t *_this, unsigned int width,
                         unsigned int height, unsigned int chroma_width,
                         unsigned int chroma_height, vo_setup_result_t * result)
{
	yuv_instance_t *this = (yuv_instance_t *)_this;
	int res = yuv_core_setup( this, width, height,  
							  chroma_width, chroma_height,
                              result,
							  yuv_draw_frame );
    if( res )
        return 1;
    this->oheight = height;
    y4m_si_set_height(this->strm_info, height);
    this->owidth  = width;
    y4m_si_set_width(this->strm_info, width);

    this->vo.setup2 = yuv_core_setup2;
    return 0;
}

vo_instance_t * vo_yuv_open(void)
{
	yuv_instance_t *instance = malloc(sizeof(yuv_instance_t));
	if( instance == NULL )
		return NULL;

	instance->vo.setup = vo_yuv_setup;
	return (vo_instance_t *) instance;
}

static void yuvh_draw_frame (vo_instance_t * instance,
			    uint8_t * const * base, void* id)
{
	int res;
	uint8_t *dst;
	uint8_t * const *src = base;
	yuv_instance_t *this = (yuv_instance_t *) instance;	
	int iw = this->owidth;
	int ih = this->oheight;
	int is = this->omargin;
	int i, j;
	size_t n;
	y4m_frame_info_t info;

	if (++(this->framenum) < 0 )
		return;

	y4m_init_frame_info(&info);
	dst = (uint8_t *)this->image_data;
	for(j=0;j<ih;j++)
		for(i=is;i<iw+is;i++)
			*(dst++) = (src[0][2*j*this->width+2*i]+src[0][2*j*this->width+2*i+1])>>1;

	dst = (uint8_t *)this->image_data + iw * ih;
	for(j=0;j<ih/2;j++)
		for(i=is/2;i<(iw+is)/2;i++)
			*(dst++) = (src[1][2*j*this->width/2+2*i]+src[1][2*j*this->width/2+2*i+1])>>1;

	dst = (uint8_t *)this->image_data + iw * ih * 5 / 4;
	for(j=0;j<ih/2;j++)
		for(i=is/2;i<(iw+is)/2;i++)
			*(dst++) = (src[2][2*j*this->width/2+2*i]+src[2][2*j*this->width/2+2*i+1])>>1;


	res = y4m_write_frame_header(fileno(this->out_strm), this->strm_info, &info);
	n = fwrite(this->image_data,1,iw * ih * 3/2,this->out_strm);
	fflush(this->out_strm);
	if( res |= Y4M_OK || (int)(n)!=iw * ih * 3/2)
	{
		fprintf(stderr,"Write of output failed\n");
		exit(1);
	}
	
 
}


static int  vo_yuvh_setup( vo_instance_t *_this, unsigned int width,
                         unsigned int height, unsigned int chroma_width,
                         unsigned int chroma_height, vo_setup_result_t * result)
{
    yuv_instance_t *this = (yuv_instance_t *)_this;
    int res = yuv_core_setup( this, width, height,  
                              chroma_width, chroma_height,
                              result,
                              yuvh_draw_frame );
    if( res )
        return 1;
    /*this->strm_info->height = this->oheight = height/2;
    this->strm_info->width = this->owidth = (width/32)*16; */
    this->oheight = height/2;
    this->owidth = (width/32)*16;
    y4m_si_set_height(this->strm_info, this->oheight);
    y4m_si_set_width(this->strm_info,  this->owidth);

	this->omargin = (width - 2*this->owidth)/4;
    this->image_data = (int8_t *)malloc(this->owidth*this->oheight*3/2);
    if( this->image_data == NULL )
        return 1;

    this->vo.setup2 = yuv_core_setup2;
    return 0;
}


vo_instance_t * vo_yuvh_open(void)
{
	yuv_instance_t *instance = malloc(sizeof(yuv_instance_t));

	if( instance == NULL )
		return NULL;

	instance->vo.setup = vo_yuvh_setup;
	return (vo_instance_t *) instance;
}

static void yuvs_draw_frame (vo_instance_t * instance,
			    uint8_t * const * base, void* id)
{
	int res;
	uint8_t *dst;
	uint8_t * const *src = base;
	uint8_t *srcp;
	yuv_instance_t *this = (yuv_instance_t *) instance;	
	int iw = this->owidth;
	int ih = this->oheight;
	int is = this->omargin;
	int i, j;
	size_t n;
	y4m_frame_info_t info;

	if (++(this->framenum) < 0 )
		return;

	y4m_init_frame_info( &info);

	dst = (uint8_t *)this->image_data;
	for(j=0;j<ih;j++)
		for(i=is;i<iw+is;i+=2)
		{
			srcp = &src[0][j*this->width+i*3/2];
			dst[0] = (2*srcp[0]+srcp[1])/3;
			dst[1] = (2*srcp[2]+srcp[1])/3;
			dst += 2;
		}

	dst = (uint8_t *)this->image_data + iw * ih;
	for(j=0;j<ih/2;j++)
		for(i=is/2;i<(iw+is)/2;i+=2)
		{
			srcp = &src[1][j*this->width/2+i*3/2];
			dst[0] = (2*srcp[0]+srcp[1])/3;
			dst[1] = (2*srcp[2]+srcp[1])/3;
			dst += 2;
		}
	dst = (uint8_t *)this->image_data + iw * ih * 5 / 4;
	for(j=0;j<ih/2;j++)
		for(i=is/2;i<(iw+is)/2;i+=2)
		{
			srcp = &src[2][j*this->width/2+i*3/2];
			dst[0] = (2*srcp[0]+srcp[1])/3;
			dst[1] = (2*srcp[2]+srcp[1])/3;
			dst += 2;
		}

	res = y4m_write_frame_header(fileno(this->out_strm), this->strm_info, &info);
	n = fwrite(this->image_data,1,iw * ih * 3/2,this->out_strm);
	fflush(this->out_strm);
	if(res != Y4M_OK || (int)(n)!=iw * ih * 3/2)
	{
		fprintf(stderr,"Write of output failed\n");
		exit(1);
	}
 
}

static int vo_yuvs_setup( vo_instance_t *_this, unsigned int width,
                         unsigned int height, unsigned int chroma_width,
                         unsigned int chroma_height, vo_setup_result_t * result)
{
	yuv_instance_t *this = (yuv_instance_t *)_this;
    int res = yuv_core_setup( this, width, height,  
                              chroma_width, chroma_height,
                              result,
							  yuvs_draw_frame );
    if( res )
        return 1;

	/*this->strm_info->height = this->oheight = height;
	this->strm_info->width = this->owidth = (width*2/(3*16))*16; */
    this->oheight = height;
    this->owidth = (width*2/(3*16))*16;

    y4m_si_set_height(this->strm_info, this->oheight);
    y4m_si_set_width(this->strm_info,  this->owidth);

	this->omargin = (width*2/3 - this->owidth)/2;
	if( this->owidth != 480 )
	{
		fprintf( stderr, "Warning: Output width is %d which is not suitable for SVCD!\n", this->owidth);
	}

	this->image_data = (int8_t *)malloc(this->owidth*this->oheight*3/2);
	if( this->image_data == NULL )
		return 1;
    this->vo.setup2 = yuv_core_setup2;
    return 0;
}


vo_instance_t * vo_yuvs_open(void)
{
	yuv_instance_t *instance = malloc(sizeof(yuv_instance_t));

	if( instance == NULL )
		return NULL;

	instance->vo.setup = vo_yuvs_setup;
	return (vo_instance_t *) instance;
}


#endif
