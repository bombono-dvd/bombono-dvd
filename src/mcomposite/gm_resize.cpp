//
// COPY_N_PASTE from
// http://www.graphicsmagick.org
// Copyright (C) 2003 GraphicsMagick Group
// Copyright (C) 2002 ImageMagick Studio
//

//
// gm_resize.cpp
// This file is part of Bombono DVD project.
//
// Copyright (c) 2007 Ilya Murav'jov
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 

#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <unistd.h>
#include <cstdio>

namespace MagickLib
{
#include <magick/api.h>
}

#include "mtech.h"

// namespace MagickLib
// {
//     typedef Image Image;
//     typedef ExceptionInfo ExceptionInfo;
// };
using namespace MagickLib;

namespace Img
{

#define AbsoluteValue(x)  ((x) < 0 ? -(x) : (x))
#define False  0
#define DegreesToRadians(x) (MagickPI*(x)/180.0)
#define IsGray(color)  \
  (((color).red == (color).green) && ((color).green == (color).blue))
#define MagickIncarnate(x)  InitializeMagick(x)
#define MagickEpsilon  1.0e-12
#define MagickPI  3.14159265358979323846264338327950288419716939937510
#define MagickSQ2PI 2.50662827463100024161235523934010416269302368164062
#define Max(x,y)  (((x) > (y)) ? (x) : (y))
#define Min(x,y)  (((x) < (y)) ? (x) : (y))
#define QuantumTick(i,span) \
  ((((i) & 0xff) == 0) || (i == ((magick_int64_t) (span)-1)))
#define RadiansToDegrees(x) (180.0*(x)/MagickPI)
#define ScaleColor5to8(x)  (((x) << 3) | ((x) >> 2))
#define ScaleColor6to8(x)  (((x) << 2) | ((x) >> 4))
#define Swap(x,y) ((x)^=(y), (y)^=(x), (x)^=(y))
#define True  1

#define MagickAllocateMemory(type,size) ((type) malloc((size_t) (size)))
#define MagickFreeMemory(memory) \
{ \
    void *_magick_mp; \
    if (memory != 0) \
      { \
        _magick_mp=memory; \
        free(_magick_mp); \
        memory=0; \
      } \
}

// typedef struct _ContributionInfo
struct ContributionInfo
{
  double
    weight;

  long
    pixel;
}; //ContributionInfo;

// typedef struct _FilterInfo
struct FilterInfo
{
  double
    (*function)(const double,const double),
    support;
}; //FilterInfo;

static double J1(double x)
{
  double
    p,
    q;

  register long
    i;

  static const double
    Pone[] =
    {
       0.581199354001606143928050809e+21,
      -0.6672106568924916298020941484e+20,
       0.2316433580634002297931815435e+19,
      -0.3588817569910106050743641413e+17,
       0.2908795263834775409737601689e+15,
      -0.1322983480332126453125473247e+13,
       0.3413234182301700539091292655e+10,
      -0.4695753530642995859767162166e+7,
       0.270112271089232341485679099e+4
    },
    Qone[] =
    {
      0.11623987080032122878585294e+22,
      0.1185770712190320999837113348e+20,
      0.6092061398917521746105196863e+17,
      0.2081661221307607351240184229e+15,
      0.5243710262167649715406728642e+12,
      0.1013863514358673989967045588e+10,
      0.1501793594998585505921097578e+7,
      0.1606931573481487801970916749e+4,
      0.1e+1
    };

  p=Pone[8];
  q=Qone[8];
  for (i=7; i >= 0; i--)
  {
    p=p*x*x+Pone[i];
    q=q*x*x+Qone[i];
  }
  return(p/q);
}

static double P1(double x)
{
  double
    p,
    q;

  register long
    i;

  static const double
    Pone[] =
    {
      0.352246649133679798341724373e+5,
      0.62758845247161281269005675e+5,
      0.313539631109159574238669888e+5,
      0.49854832060594338434500455e+4,
      0.2111529182853962382105718e+3,
      0.12571716929145341558495e+1
    },
    Qone[] =
    {
      0.352246649133679798068390431e+5,
      0.626943469593560511888833731e+5,
      0.312404063819041039923015703e+5,
      0.4930396490181088979386097e+4,
      0.2030775189134759322293574e+3,
      0.1e+1
    };

  p=Pone[5];
  q=Qone[5];
  for (i=4; i >= 0; i--)
  {
    p=p*(8.0/x)*(8.0/x)+Pone[i];
    q=q*(8.0/x)*(8.0/x)+Qone[i];
  }
  return(p/q);
}

static double Q1(double x)
{
  double
    p,
    q;

  register long
    i;

  static const double
    Pone[] =
    {
      0.3511751914303552822533318e+3,
      0.7210391804904475039280863e+3,
      0.4259873011654442389886993e+3,
      0.831898957673850827325226e+2,
      0.45681716295512267064405e+1,
      0.3532840052740123642735e-1
    },
    Qone[] =
    {
      0.74917374171809127714519505e+4,
      0.154141773392650970499848051e+5,
      0.91522317015169922705904727e+4,
      0.18111867005523513506724158e+4,
      0.1038187585462133728776636e+3,
      0.1e+1
    };

  p=Pone[5];
  q=Qone[5];
  for (i=4; i >= 0; i--)
  {
    p=p*(8.0/x)*(8.0/x)+Pone[i];
    q=q*(8.0/x)*(8.0/x)+Qone[i];
  }
  return(p/q);
}


static double BesselOrderOne(double x)
{
  double
    p,
    q;

  if (x == 0.0)
    return(0.0);
  p=x;
  if (x < 0.0)
    x=(-x);
  if (x < 8.0)
    return(p*J1(x));
  q=sqrt(2.0/(MagickPI*x))*(P1(x)*(1.0/sqrt(2.0)*(sin(x)-cos(x)))-8.0/x*Q1(x)*
    (-1.0/sqrt(2.0)*(sin(x)+cos(x))));
  if (p < 0.0)
    q=(-q);
  return(q);
}

static double Bessel(const double x,const double ) // support)
{
  if (x == 0.0)
    return(MagickPI/4.0);
  return(BesselOrderOne(MagickPI*x)/(2.0*x));
}

static double Sinc(const double x,const double ) // support)
{
  if (x == 0.0)
    return(1.0);
  return(sin(MagickPI*x)/(MagickPI*x));
}

static double Blackman(const double x,const double ) // support)
{
  return(0.42+0.5*cos(MagickPI*x)+0.08*cos(2*MagickPI*x));
}

static double BlackmanBessel(const double x,const double support)
{
  return(Blackman(x/support,support)*Bessel(x,support));
}

static double BlackmanSinc(const double x,const double support)
{
  return(Blackman(x/support,support)*Sinc(x,support));
}

static double Box(const double x,const double ) // support)
{
  if (x < -0.5)
    return(0.0);
  if (x < 0.5)
    return(1.0);
  return(0.0);
}

static double Catrom(const double x,const double ) // support)
{
  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return(0.5*(4.0+x*(8.0+x*(5.0+x))));
  if (x < 0.0)
    return(0.5*(2.0+x*x*(-5.0-3.0*x)));
  if (x < 1.0)
    return(0.5*(2.0+x*x*(-5.0+3.0*x)));
  if (x < 2.0)
    return(0.5*(4.0+x*(-8.0+x*(5.0-x))));
  return(0.0);
}

static double Cubic(const double x,const double ) // support)
{
  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return((2.0+x)*(2.0+x)*(2.0+x)/6.0);
  if (x < 0.0)
    return((4.0+x*x*(-6.0-3.0*x))/6.0);
  if (x < 1.0)
    return((4.0+x*x*(-6.0+3.0*x))/6.0);
  if (x < 2.0)
    return((2.0-x)*(2.0-x)*(2.0-x)/6.0);
  return(0.0);
}

static double Gaussian(const double x,const double ) // support)
{
  return(exp(-2.0*x*x)*sqrt(2.0/MagickPI));
}

static double Hanning(const double x,const double ) // support)
{
  return(0.5+0.5*cos(MagickPI*x));
}

static double Hamming(const double x,const double ) // support)
{
  return(0.54+0.46*cos(MagickPI*x));
}

static double Hermite(const double x,const double ) // support)
{
  if (x < -1.0)
    return(0.0);
  if (x < 0.0)
    return((2.0*(-x)-3.0)*(-x)*(-x)+1.0);
  if (x < 1.0)
    return((2.0*x-3.0)*x*x+1.0);
  return(0.0);
}

static double Lanczos(const double x,const double support)
{
  if (x < -3.0)
    return(0.0);
  if (x < 0.0)
    return(Sinc(-x,support)*Sinc(-x/3.0,support));
  if (x < 3.0)
    return(Sinc(x,support)*Sinc(x/3.0,support));
  return(0.0);
}

static double Mitchell(const double x,const double ) // support)
{
#define B   (1.0/3.0)
#define C   (1.0/3.0)
#define P0  ((  6.0- 2.0*B       )/6.0)
#define P2  ((-18.0+12.0*B+ 6.0*C)/6.0)
#define P3  (( 12.0- 9.0*B- 6.0*C)/6.0)
#define Q0  ((       8.0*B+24.0*C)/6.0)
#define Q1  ((     -12.0*B-48.0*C)/6.0)
#define Q2  ((       6.0*B+30.0*C)/6.0)
#define Q3  ((     - 1.0*B- 6.0*C)/6.0)

  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return(Q0-x*(Q1-x*(Q2-x*Q3)));
  if (x < 0.0)
    return(P0+x*x*(P2-x*P3));
  if (x < 1.0)
    return(P0+x*x*(P2+x*P3));
  if (x < 2.0)
    return(Q0+x*(Q1+x*(Q2+x*Q3)));
  return(0.0);
}

static double Quadratic(const double x,const double ) // support)
{
  if (x < -1.5)
    return(0.0);
  if (x < -0.5)
    return(0.5*(x+1.5)*(x+1.5));
  if (x < 0.5)
    return(0.75-x*x);
  if (x < 1.5)
    return(0.5*(x-1.5)*(x-1.5));
  return(0.0);
}

static double Triangle(const double x,const double ) // support)
{
  if (x < -1.0)
    return(0.0);
  if (x < 0.0)
    return(1.0+x);
  if (x < 1.0)
    return(1.0-x);
  return(0.0);
}

/////////////////////////////////////////////////////////

static unsigned int HorizontalFilter(const Image *source,Image *destination,
  const double x_factor,const FilterInfo *filter_info,const double blur,
  ContributionInfo *contribution,const size_t span,unsigned long *quantum,
  ExceptionInfo *exception)
{
#define ResizeImageText  "  Resize image...  "

  double
    center,
    density,
    scale,
    support;

  DoublePixelPacket
    pixel,
    zero;

  long
    j,
    n,
    start,
    stop,
    y;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *source_indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Apply filter to resize horizontally from source to destination.
  */
  scale=blur*Max(1.0/x_factor,1.0);
  support=scale*filter_info->support;
  destination->storage_class=source->storage_class;
  if (support > 0.5)
    destination->storage_class=DirectClass;
  else
    {
      /*
        Reduce to point sampling.
      */
      support=0.5+MagickEpsilon;
      scale=1.0;
    }
  scale=1.0/scale;
  memset(&zero,0,sizeof(DoublePixelPacket));
  for (x=0; x < (long) destination->columns; x++)
  {
    center=(double) (x+0.5)/x_factor;
    start=(long) Max(center-support+0.5,0);
    stop=(long) Min(center+support+0.5,source->columns);
    density=0.0;
    for (n=0; n < (stop-start); n++)
    {
      contribution[n].pixel=start+n;
      contribution[n].weight=
        filter_info->function(scale*(start+n-center+0.5),filter_info->support);
      density+=contribution[n].weight;
    }
    if ((density != 0.0) && (density != 1.0))
      {
        /*
          Normalize.
        */
        density=1.0/density;
        for (i=0; i < n; i++)
          contribution[i].weight*=density;
      }
    p=AcquireImagePixels(source,contribution[0].pixel,0,
      contribution[n-1].pixel-contribution[0].pixel+1,source->rows,exception);
    q=SetImagePixels(destination,x,0,1,destination->rows);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    source_indexes=GetIndexes(source);
    indexes=GetIndexes(destination);
    for (y=0; y < (long) destination->rows; y++)
    {
      pixel=zero;
      for (i=0; i < n; i++)
      {
        j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
          (contribution[i].pixel-contribution[0].pixel);
        pixel.red+=contribution[i].weight*(p+j)->red;
        pixel.green+=contribution[i].weight*(p+j)->green;
        pixel.blue+=contribution[i].weight*(p+j)->blue;
	if ((source->matte) || (source->colorspace == CMYKColorspace))
          pixel.opacity+=contribution[i].weight*(p+j)->opacity;
      }
      if ((indexes != (IndexPacket *) NULL) &&
          (source_indexes != (IndexPacket *) NULL))
        {
          i=Min(Max((long) (center+0.5),start),stop-1);
          j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
            (contribution[i-start].pixel-contribution[0].pixel);
          indexes[y]=source_indexes[j];
        }
      q->red=(Quantum) ((pixel.red < 0) ? 0 :
        (pixel.red > MaxRGB) ? MaxRGB : pixel.red+0.5);
      q->green=(Quantum) ((pixel.green < 0) ? 0 :
        (pixel.green > MaxRGB) ? MaxRGB : pixel.green+0.5);
      q->blue=(Quantum) ((pixel.blue < 0) ? 0 :
        (pixel.blue > MaxRGB) ? MaxRGB : pixel.blue+0.5);
      if ((destination->matte) || (destination->colorspace == CMYKColorspace))
        q->opacity=(Quantum) ((pixel.opacity < 0) ? 0 :
          (pixel.opacity > MaxRGB) ? MaxRGB : pixel.opacity+0.5);
      q++;
    }
    if (!SyncImagePixels(destination))
      break;
    if (QuantumTick(*quantum,span))
      if (!MagickMonitor(ResizeImageText,*quantum,span,exception))
        break;
    (*quantum)++;
  }
  return(x == (long) destination->columns);
}

static unsigned int VerticalFilter(const Image *source,Image *destination,
  const double y_factor,const FilterInfo *filter_info,const double blur,
  ContributionInfo *contribution,const size_t span,unsigned long *quantum,
  ExceptionInfo *exception)
{
  double
    center,
    density,
    scale,
    support;

  DoublePixelPacket
    pixel,
    zero;

  long
    j,
    n,
    start,
    stop,
    x;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *source_indexes;

  register long
    i,
    y;

  register PixelPacket
    *q;

  /*
    Apply filter to resize vertically from source to destination.
  */
  scale=blur*Max(1.0/y_factor,1.0);
  support=scale*filter_info->support;
  destination->storage_class=source->storage_class;
  if (support > 0.5)
    destination->storage_class=DirectClass;
  else
    {
      /*
        Reduce to point sampling.
      */
      support=0.5+MagickEpsilon;
      scale=1.0;
    }
  scale=1.0/scale;
  memset(&zero,0,sizeof(DoublePixelPacket));
  for (y=0; y < (long) destination->rows; y++)
  {
    center=(double) (y+0.5)/y_factor;
    start=(long) Max(center-support+0.5,0);
    stop=(long) Min(center+support+0.5,source->rows);
    density=0.0;
    for (n=0; n < (stop-start); n++)
    {
      contribution[n].pixel=start+n;
      contribution[n].weight=
        filter_info->function(scale*(start+n-center+0.5),filter_info->support);
      density+=contribution[n].weight;
    }
    if ((density != 0.0) && (density != 1.0))
      {
        /*
          Normalize.
        */
        density=1.0/density;
        for (i=0; i < n; i++)
          contribution[i].weight*=density;
      }
    p=AcquireImagePixels(source,0,contribution[0].pixel,source->columns,
      contribution[n-1].pixel-contribution[0].pixel+1,exception);
    q=SetImagePixels(destination,0,y,destination->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    source_indexes=GetIndexes(source);
    indexes=GetIndexes(destination);
    for (x=0; x < (long) destination->columns; x++)
    {
      pixel=zero;
      for (i=0; i < n; i++)
      {
        j=(long) ((contribution[i].pixel-contribution[0].pixel)*
          source->columns+x);
        pixel.red+=contribution[i].weight*(p+j)->red;
        pixel.green+=contribution[i].weight*(p+j)->green;
        pixel.blue+=contribution[i].weight*(p+j)->blue;
        if ((source->matte) || (source->colorspace == CMYKColorspace))
          pixel.opacity+=contribution[i].weight*(p+j)->opacity;
      }
      if ((indexes != (IndexPacket *) NULL) &&
          (source_indexes != (IndexPacket *) NULL))
        {
          i=Min(Max((long) (center+0.5),start),stop-1);
          j=(long) ((contribution[i-start].pixel-contribution[0].pixel)*
            source->columns+x);
          indexes[x]=source_indexes[j];
        }
      q->red=(Quantum) ((pixel.red < 0) ? 0 :
        (pixel.red > MaxRGB) ? MaxRGB : pixel.red+0.5);
      q->green=(Quantum) ((pixel.green < 0) ? 0 :
        (pixel.green > MaxRGB) ? MaxRGB : pixel.green+0.5);
      q->blue=(Quantum) ((pixel.blue < 0) ? 0 :
        (pixel.blue > MaxRGB) ? MaxRGB : pixel.blue+0.5);
      if ((destination->matte) || (destination->colorspace == CMYKColorspace))
        q->opacity=(Quantum) ((pixel.opacity < 0) ? 0 :
          (pixel.opacity > MaxRGB) ? MaxRGB : pixel.opacity+0.5);
      q++;
    }
    if (!SyncImagePixels(destination))
      break;
    if (QuantumTick(*quantum,span))
      if (!MagickMonitor(ResizeImageText,*quantum,span,exception))
        break;
    (*quantum)++;
  }
  return(y == (long) destination->rows);
}

Image *ResizeImage(const Image *image,const unsigned long columns,
  const unsigned long rows,const FilterTypes filter,const double blur,
  ExceptionInfo *exception)
{
  ContributionInfo
    *contribution;

  double
    support,
    x_factor,
    x_support,
    y_factor,
    y_support;

  Image
    *source_image,
    *resize_image;

  register long
    i;

  static const FilterInfo
    filters[SincFilter+1] =
    {
      { Box, 0.0 },
      { Box, 0.0 },
      { Box, 0.5 },
      { Triangle, 1.0 },
      { Hermite, 1.0 },
      { Hanning, 1.0 },
      { Hamming, 1.0 },
      { Blackman, 1.0 },
      { Gaussian, 1.25 },
      { Quadratic, 1.5 },
      { Cubic, 2.0 },
      { Catrom, 2.0 },
      { Mitchell, 2.0 },
      { Lanczos, 3.0 },
      { BlackmanBessel, 3.2383 },
      { BlackmanSinc, 4.0 }
    };

  size_t
    span;

  unsigned int
    status;

  unsigned long
    quantum;

  /*
    Initialize resize image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  assert((filter >= 0) && (filter <= SincFilter));

  if ((columns == 0) || (rows == 0))
    // Муравьев
    //ThrowImageException(ImageError,UnableToResizeImage,
    //MagickMsg(CorruptImageError,NegativeOrZeroImageSize));
    Error("Unable to resize image");
    
  if ((columns == image->columns) && (rows == image->rows) && (blur == 1.0))
    return(CloneImage(image,0,0,True,exception));
  resize_image=CloneImage(image,columns,rows,True,exception);
  if (resize_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Allocate filter contribution info.
  */
  x_factor=(double) resize_image->columns/image->columns;
  y_factor=(double) resize_image->rows/image->rows;
  i=(long) DefaultResizeFilter;
  if (image->filter != UndefinedFilter)
    i=(long) image->filter;
  else
    if ((image->storage_class == PseudoClass) || image->matte ||
        ((x_factor*y_factor) > 1.0))
      i=(long) MitchellFilter;

//   LogMagickEvent(TransformEvent,GetMagickModule(),
//     "Resizing image of size %lux%lu to %lux%lu using %s filter",
//     image->columns,image->rows,columns,rows,
//     ResizeFilterToString((FilterTypes)i));

  x_support=blur*Max(1.0/x_factor,1.0)*filters[i].support;
  y_support=blur*Max(1.0/y_factor,1.0)*filters[i].support;
  support=Max(x_support,y_support);
  if (support < filters[i].support)
    support=filters[i].support;

  contribution=MagickAllocateMemory(ContributionInfo *,
    (size_t) (2.0*Max(support,0.5)+3)*sizeof(ContributionInfo));
  if (contribution == (ContributionInfo *) NULL)
    {
      DestroyImage(resize_image);
      //ThrowImageException3(ResourceLimitError,MemoryAllocationFailed,
      //  UnableToResizeImage)
      Error("Unable to resize image: memory allocation failed");
    }

  /*
    Resize image.
  */
  quantum=0;
  if (((double) columns*(image->rows+rows)) >
      ((double) rows*(image->columns+columns)))
    {
      source_image=CloneImage(resize_image,columns,image->rows,True,exception);
      if (source_image == (Image *) NULL)
        {
          MagickFreeMemory(contribution);
          DestroyImage(resize_image);
          return((Image *) NULL);
        }
      span=source_image->columns+resize_image->rows;
      status=HorizontalFilter(image,source_image,x_factor,&filters[i],blur,
        contribution,span,&quantum,exception);
      status|=VerticalFilter(source_image,resize_image,y_factor,&filters[i],
        blur,contribution,span,&quantum,exception);
    }
  else
    {
      source_image=CloneImage(resize_image,image->columns,rows,True,exception);
      if (source_image == (Image *) NULL)
        {
          MagickFreeMemory(contribution);
          DestroyImage(resize_image);
          return((Image *) NULL);
        }
      span=resize_image->columns+source_image->rows;
      status=VerticalFilter(image,source_image,y_factor,&filters[i],blur,
        contribution,span,&quantum,exception);
      status|=HorizontalFilter(source_image,resize_image,x_factor,&filters[i],
        blur,contribution,span,&quantum,exception);
    }
  /*
    Free allocated memory.
  */
  MagickFreeMemory(contribution);
  DestroyImage(source_image);
  if (status == False)
    {
      DestroyImage(resize_image);
      //ThrowImageException3(ResourceLimitError,MemoryAllocationFailed,
      //  UnableToResizeImage)
      Error("Unable to resize image: memory allocation failed");
    }
  resize_image->is_grayscale=image->is_grayscale;
  return(resize_image);
}

// C_LINKAGE MagickLib::Image* Img_ZoomImage(const MagickLib::Image *image,const unsigned long columns,
//   const unsigned long rows,MagickLib::ExceptionInfo *exception)
Image* ZoomImage(const Image *image,const unsigned long columns,
  const unsigned long rows, ExceptionInfo *exception)
{
  Image
    *zoom_image;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  zoom_image=Img::ResizeImage(image,columns,rows,image->filter,image->blur,
    exception);
  return(zoom_image);
}


} // namespace Img

