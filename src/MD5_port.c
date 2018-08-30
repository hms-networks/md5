/*******************************************************************************
**    Copyright (C) 2018 HMS Industrial Networks Inc, all rights reserved
********************************************************************************
**
**       File: MD5_port.c
**    Summary: Routines that may be ported to support differences in architecture
**             or to utilize optimized/built-in compiler routines.
**
********************************************************************************
********************************************************************************
*/
#include "MD5.h"

#if( MD5_USE_16BIT_CHAR == 1 )

void MD5_PORT_CopyOctetsImpl( void* pxDest, UINT16 iDestOctetOffset,
                              const void* pxSrc, UINT16 iSrcOctetOffset,
                              UINT16 iNumOctets )
{
   UINT16 i;
   UINT16 iData;
   BOOL fOddDestOctet;
   BOOL fOddSrcOctet;
   UINT16* piDest;
   UINT16* piSrc;

   fOddDestOctet = iDestOctetOffset & 1;
   fOddSrcOctet =  iSrcOctetOffset & 1;
   piDest =   (UINT16*)pxDest + ( iDestOctetOffset >> 1 );
   piSrc =    (UINT16*)pxSrc + ( iSrcOctetOffset >> 1 );

   for( i = 0; i < iNumOctets; i++ )
   {
      if( fOddSrcOctet )
      {
         iData = MD5_PORT_GetHighAddrOct( *piSrc );
         piSrc++;
      }
      else
      {
         iData = MD5_PORT_GetLowAddrOct( *piSrc );
      }
      fOddSrcOctet ^= 1;

      if( fOddDestOctet )
      {
         MD5_PORT_SetHighAddrOct( *piDest, iData );
         piDest++;
      }
      else
      {
         MD5_PORT_SetLowAddrOct( *piDest, iData );
      }
      fOddDestOctet ^= 1;
   }
}

void MD5_PORT_StrCpyToNativeImpl( void* pxDest, const void* pxSrc,
                                  UINT16 iSrcOctetOffset, UINT16 iNbrOfChars )
{
   UINT16 i;
   UINT16* piDest;
   const UINT16* piSrc;
   BOOL fOddSrc;

   piDest = (UINT16*)pxDest;
   piSrc =  (UINT16*)pxSrc;
   fOddSrc = iSrcOctetOffset & 1;

   for( i = 0; i < iNbrOfChars; i++ )
   {
      if ( fOddSrc )
      {
         piDest[ i ] = MD5_PORT_GetHighAddrOct( piSrc[ i >> 1 ] );
      }
      else
      {
         piDest[ i ] = MD5_PORT_GetLowAddrOct( piSrc[ i >> 1 ] );
      }
      fOddSrc ^= 1;
   }
}

void MD5_PORT_StrCpyToPackedImpl( void* pxDest, UINT16 iDestOctetOffset,
                                  const void* pxSrc, UINT16 iNbrOfChars )
{
   UINT16 i;
   UINT16* piDest;
   const UINT16* piSrc;
   BOOL fOddDest;

   piDest = (UINT16*)pxDest;
   piSrc =  (UINT16*)pxSrc;
   fOddDest = iDestOctetOffset & 1;

   for( i = 0; i < iNbrOfChars; i++ )
   {
      if ( fOddDest )
      {
         MD5_PORT_SetHighAddrOct( piDest[ i >> 1 ], piSrc[ i ] );
      }
      else
      {
         MD5_PORT_SetLowAddrOct( piDest[ i >> 1 ], piSrc[ i ] );
      }
      fOddDest ^= 1;
   }
}
#endif
