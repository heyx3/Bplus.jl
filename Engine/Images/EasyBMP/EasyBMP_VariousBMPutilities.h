/*************************************************
*                                                *
*  EasyBMP Cross-Platform Windows Bitmap Library * 
*                                                *
*  Author: Paul Macklin                          *
*   email: macklin01@users.sourceforge.net       *
* support: http://easybmp.sourceforge.net        *
*                                                *
*          file: EasyBMP_VariousBMPutilities.h   *
*    date added: 05-02-2005                      *
* date modified: 12-01-2006                      *
*       version: 1.06                            *
*                                                *
*   License: BSD (revised/modified)              *
* Copyright: 2005-6 by the EasyBMP Project       * 
*                                                *
* description: Various utilities.                *
*                                                *
*************************************************/

#ifndef _EasyBMP_VariousBMPutilities_h_
#define _EasyBMP_VariousBMPutilities_h_

namespace EASY_BMP_NAMESPACE {

EASY_BMP_API BMFH GetBMFH( const char* szFileNameIn );
EASY_BMP_API BMIH GetBMIH( const char* szFileNameIn );
EASY_BMP_API void DisplayBitmapInfo( const char* szFileNameIn );
EASY_BMP_API int GetBitmapColorDepth( const char* szFileNameIn );
EASY_BMP_API void PixelToPixelCopy( BMP& From, int FromX, int FromY,
                                    BMP& To, int ToX, int ToY);
EASY_BMP_API void PixelToPixelCopyTransparent( BMP& From, int FromX, int FromY,
                                               BMP& To, int ToX, int ToY,
                                               RGBApixel& Transparent );
EASY_BMP_API void RangedPixelToPixelCopy( BMP& From, int FromL , int FromR, int FromB, int FromT,
                                          BMP& To, int ToX, int ToY );
EASY_BMP_API void RangedPixelToPixelCopyTransparent(
                     BMP& From, int FromL , int FromR, int FromB, int FromT, 
                     BMP& To, int ToX, int ToY ,
                     RGBApixel& Transparent );
EASY_BMP_API bool CreateGrayscaleColorTable( BMP& InputImage );

EASY_BMP_API bool Rescale( BMP& InputImage , char mode, int NewDimension );

}
#endif
