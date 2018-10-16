/*******************************************************************************
**    Copyright (C) 2018 HMS Industrial Networks Inc, all rights reserved
********************************************************************************
**
**       File: MD5.c
**    Summary: Contains an implementation of the RFC1321 defined MD5 hashing
**             algorithm. This unit supports multiple simultaneous
**             computations of MD5 hashing via the use an MD5 instance handle
**             that tracks the state independently from other instances.
**             The unit accepts taking in subsets of the overall payload
**             lending itself well to stream based processing.
**
**             This implementation is a derivative implementation based on
**             several sources. Namely the MD5 algorithm described in RFC1321,
**             the pseudocode from Wikipedia, and an implementation from
**             rosettacode.
**
**             tools.ietf.org/html/rfc1321
**             rosettacode.org/wiki/MD5#C
**             en.wikipedia.org/wiki/MD5
**
********************************************************************************
********************************************************************************
*/

#include <string.h>
#if( MD5_USE_PRINTF == 1 )
#include <stdio.h>
#endif
#if( MD5_USE_T_TABLE == 0 )
#include <math.h>
#endif

#include "MD5.h"
#include "MD5_port.h"


#if( MD5_USE_BIG_ENDIAN == 1 )
#error "This mode of operation is not yet supported!"
#endif

/*******************************************************************************
** Constants
********************************************************************************
*/

#define MD5_NUM_ROUNDS        ( 4U )
#define MD5_NUM_OPERATIONS    ( 16U )

#define MD5_A_INDEX           ( 0 )
#define MD5_B_INDEX           ( 1 )
#define MD5_C_INDEX           ( 2 )
#define MD5_D_INDEX           ( 3 )

#define MD5_AUX_F(x,y,z)      ( ( x & y ) | ( ~x & z ) )
#define MD5_AUX_G(x,y,z)      ( ( x & z ) | ( y & ~z ) )
#define MD5_AUX_H(x,y,z)      ( x ^ y ^ z )
#define MD5_AUX_I(x,y,z)      ( y ^ ( x | ~z ) )

/*******************************************************************************
** Typedefs
********************************************************************************
*/

typedef UINT32 ( *MD5_t_DigestFunc )( UINT32 adwRegisters[] );

typedef struct MD5_TestStruct
{
   const char* acTestMsg;
   const UINT8 abExpectedDigest[ MD5_DIGEST_SIZE ];
} MD5_TestStructType;

/*******************************************************************************
** Public Globals
********************************************************************************
*/

/*******************************************************************************
** Private Globals
********************************************************************************
*/
#if( MD5_USE_16BIT_CHAR == 1 )
   static const UINT8 bCharNumBytes = 2;
#else
   static const UINT8 bCharNumBytes = 1;
#endif

/*----------------------------------------------------------------------------
** Simple test case structure for specifying known/documented MD5 results
** These test cases do not include the NULL terminator and are evaluated
** against strlen().
**----------------------------------------------------------------------------
*/
static const MD5_TestStructType MD5_asTestCases[] =
{
   { "",
     { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
       0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e } },
   { "a",
     { 0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
       0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61 } },
   { "abc",
     { 0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
       0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72 } },
   { "message digest",
     { 0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d,
       0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0 } },
   { "abcdefghijklmnopqrstuvwxyz",
     { 0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00,
       0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b } },
   { "The quick brown fox jumps over the lazy dog",
     { 0x9E, 0x10, 0x7D, 0x9D, 0x37, 0x2B, 0xB6, 0x82,
       0x6B, 0xD8, 0x1D, 0x35, 0x42, 0xA4, 0x19, 0xD6 } },
   { "The quick brown fox jumps over the lazy dog.",
     { 0xe4, 0xd9, 0x09, 0xc2, 0x90, 0xd0, 0xfb, 0x1c,
       0xa0, 0x68, 0xff, 0xad, 0xdf, 0x22, 0xcb, 0xd0 } },
   { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
     { 0xd1, 0x74, 0xab, 0x98, 0xd2, 0x77, 0xd9, 0xf5,
       0xa5, 0x61, 0x1c, 0x2c, 0x9f, 0x41, 0x9d, 0x9f } },
   { "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
     { 0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
       0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a } },
};

#if( MD5_USE_T_TABLE == 1 )
/*----------------------------------------------------------------------------
** 'T' is defined as the binary integer part of the expression:
**
**   ( 2^32 * abs( sin( i ) ) ), where 'i' is in radians from 1 ... 64.
**
** Reference: RFC1321 Section 3.4
**----------------------------------------------------------------------------
*/
static const UINT32 MD5_adwTableT[ MD5_BLOCK_SIZE ] =
{
   0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
   0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
   0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
   0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
   0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
   0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
   0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
   0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
   0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
   0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
   0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
   0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
   0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
   0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
   0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
   0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};
#endif

/*----------------------------------------------------------------------------
** 's' is defined as the number of "rotate left" bit-shifts to perform.
** For each round a set of 4 's' values are used for the 16 operations
** to be performed.
**
** Reference: RFC1321 Section 3.4
**----------------------------------------------------------------------------
*/
#if( MD5_USE_S_TABLE == 1 )
static const UINT8 MD5_abTableS[ MD5_BLOCK_SIZE ] =
{
   7, 12, 17, 22,
   7, 12, 17, 22,
   7, 12, 17, 22,
   7, 12, 17, 22,
   5, 9,  14, 20,
   5, 9,  14, 20,
   5, 9,  14, 20,
   5, 9,  14, 20,
   4, 11, 16, 23,
   4, 11, 16, 23,
   4, 11, 16, 23,
   4, 11, 16, 23,
   6, 10, 15, 21,
   6, 10, 15, 21,
   6, 10, 15, 21,
   6, 10, 15, 21
};
#endif

/*----------------------------------------------------------------------------
** 'k' is defined as the index to access in X[]. X[] is the a subset of M[].
**
** Reference: RFC1321 Section 3.4
**----------------------------------------------------------------------------
*/
#if( MD5_USE_K_TABLE == 1 )
static const UINT8 MD5_abTableK[ MD5_BLOCK_SIZE ] =
{
   0,  1,  2,  3,
   4,  5,  6,  7,
   8,  9,  10, 11,
   12, 13, 14, 15,
   1,  6,  11, 0,
   5,  10, 15, 4,
   9,  14, 3,  8,
   13, 2,  7,  12,
   5,  8,  11, 14,
   1,  4,  7,  10,
   13, 0,  3,  6,
   9,  12, 15, 2,
   0,  7,  14, 5,
   12, 3,  10, 1,
   8,  15, 6,  13,
   4,  11, 2,  9
};
#endif

/*------------------------------------------------------------------------------
** Forward declarations
**------------------------------------------------------------------------------
*/

static UINT32 MD5_AUXILIARY_F( UINT32 adwBufferABCD[] );
static UINT32 MD5_AUXILIARY_G( UINT32 adwBufferABCD[] );
static UINT32 MD5_AUXILIARY_H( UINT32 adwBufferABCD[] );
static UINT32 MD5_AUXILIARY_I( UINT32 adwBufferABCD[] );
static UINT32 MD5_RotateLeft( UINT32 dwRegister, UINT8 bRotateCount );
static UINT32 MD5_GetValueT( UINT8 bIndex );
static void MD5_ProcessBlock( MD5_InstType* psInst );

/*******************************************************************************
** Private Services
********************************************************************************
*/

/*****************************************************************************
** Static routines
******************************************************************************
*/

/*------------------------------------------------------------------------------
** MD5-defined auxiliary function "F".
**
** Reference: RFC1321 Section 3.4
**------------------------------------------------------------------------------
** Arguments:
**    adwBufferABCD - Array containing registers A, B, C, and D
**
** Returns:
**    UINT32 - Result of function "F"
**------------------------------------------------------------------------------
*/
static UINT32 MD5_AUXILIARY_F( UINT32 adwBufferABCD[] )
{
   return MD5_AUX_F( adwBufferABCD[ MD5_B_INDEX ],
                     adwBufferABCD[ MD5_C_INDEX ],
                     adwBufferABCD[ MD5_D_INDEX ] );
}

/*------------------------------------------------------------------------------
** MD5-defined auxiliary function "G"
**
** Reference: RFC1321 Section 3.4
**------------------------------------------------------------------------------
** Arguments:
**    adwBufferABCD - Array containing registers A, B, C, and D
**
** Returns:
**    UINT32 - Result of function "G"
**------------------------------------------------------------------------------
*/
static UINT32 MD5_AUXILIARY_G( UINT32 adwBufferABCD[] )
{
   return MD5_AUX_G( adwBufferABCD[ MD5_B_INDEX ],
                     adwBufferABCD[ MD5_C_INDEX ],
                     adwBufferABCD[ MD5_D_INDEX ] );
}

/*------------------------------------------------------------------------------
** MD5-defined auxiliary function "H"
**
** Reference: RFC1321 Section 3.4
**------------------------------------------------------------------------------
** Arguments:
**    adwBufferABCD - Array containing registers A, B, C, and D
**
** Returns:
**    UINT32 - Result of function "H"
**------------------------------------------------------------------------------
*/
static UINT32 MD5_AUXILIARY_H( UINT32 adwBufferABCD[] )
{
   return MD5_AUX_H( adwBufferABCD[ MD5_B_INDEX ],
                     adwBufferABCD[ MD5_C_INDEX ],
                     adwBufferABCD[ MD5_D_INDEX ] );
}

/*------------------------------------------------------------------------------
** MD5-defined auxiliary function "I"
**
** Reference: RFC1321 Section 3.4
**------------------------------------------------------------------------------
** Arguments:
**    adwBufferABCD - Array containing registers A, B, C, and D
**
** Returns:
**    UINT32 - Result of function "I"
**------------------------------------------------------------------------------
*/
static UINT32 MD5_AUXILIARY_I( UINT32 adwBufferABCD[] )
{
   return MD5_AUX_I( adwBufferABCD[ MD5_B_INDEX ],
                     adwBufferABCD[ MD5_C_INDEX ],
                     adwBufferABCD[ MD5_D_INDEX ] );
}

/*------------------------------------------------------------------------------
** This routine rotates a 32-bit register by the specified amount
**------------------------------------------------------------------------------
** Arguments:
**    dwRegister   - The register to rotate left
**    bRotateCount - The number of bit positions to rotate
**
** Returns:
**    UINT32 - The rotated register result
**------------------------------------------------------------------------------
*/
static UINT32 MD5_RotateLeft( UINT32 dwRegister, UINT8 bRotateCount )
{
   const UINT8 bRegisterBitSize = 32;
   UINT32 dwMask = ( 1UL << bRotateCount ) - 1;

   return ( ( ( dwRegister >> ( bRegisterBitSize - bRotateCount ) ) & dwMask ) |
            ( ( dwRegister << bRotateCount ) & ~dwMask ) );
}

/*------------------------------------------------------------------------------
** Returns the value contained in T[i]
**
** 'T[i]' is defined as the binary integer part of the expression:
**
**   ( 2^32 * abs( sin( i ) ) ), where 'i' is in radians from 1 ... 64.
**
** Reference: RFC1321 Section 3.4
**------------------------------------------------------------------------------
** Arguments:
**    bIndex - Value ranges from 0..63. No protection is performed to ensure the
**             argument is within the correct bounds!
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
static UINT32 MD5_GetValueT( UINT8 bIndex )
{
#if( MD5_USE_T_TABLE == 0 )
   const UINT64 c = 1ULL << 32;
   return (UINT32)( c * fabs( sin( 1.0f + bIndex ) ) );
#else
   return MD5_adwTableT[ bIndex ];
#endif
}

/*------------------------------------------------------------------------------
** This routine processes a full 512-bit block.
** The algorithm as defined in RFC1321 performs a total of 64 calculations that
** involve a set of registers 'A', 'B', 'C' and 'D', an array 'X' (referred to
** as the "block" in this unit), a table 'T'.
**------------------------------------------------------------------------------
** Arguments:
**    psInst - Pointer to an instance containing the current state of the MD5
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
static void MD5_ProcessBlock( MD5_InstType* psInst )
{
   /* Only process the block if it has been filled with data */
   if( psInst->iBlockOffset == MD5_BLOCK_SIZE )
   {
      UINT32 adwBufferABCD[ MD5_DIGEST_SIZE_DWORDS ];
      UINT8 bOperation;
      UINT8 bRound;
      UINT8 bRegister;

#if( MD5_DEBUG == 1 )
      {
         UINT16 iByteIndex;

         MD5_PRINTF( "Block Data Set:\n\t" );

         for( iByteIndex = 0; iByteIndex < MD5_BLOCK_SIZE; iByteIndex++ )
         {
            MD5_PRINTF( " %02X", psInst->uBlockBuffer.ab[ iByteIndex ] );

            if( ( iByteIndex + 1 ) % 16 == 0 )
            {
               MD5_PRINTF( "\n\t" );
            }
         }

         MD5_PRINTF( "\n" );
      }
#endif

      /* Initialize intermediate MD state */
      for( bRegister = 0; bRegister < MD5_DIGEST_SIZE_DWORDS; bRegister++ )
      {
         adwBufferABCD[ bRegister ] = psInst->adwDigest[ bRegister ];
      }

      /*
      ** The are 4 "rounds", each round performs 16 "operations"
      ** making up a total of 64 calculations in total.
      */
      for( bRound = 0; bRound < MD5_NUM_ROUNDS; bRound++ )
      {
#if( MD5_USE_S_TABLE == 0 )
         const UINT8  abRotationRound0[]  = { 7, 12, 17, 22 };
         const UINT8  abRotationRound1[]  = { 5, 9, 14, 20 };
         const UINT8  abRotationRound2[]  = { 4, 11, 16, 23 };
         const UINT8  abRotationRound3[]  = { 6, 10, 15, 21 };
         const UINT8* apbRotationSets[]   = { abRotationRound0, abRotationRound1, abRotationRound2, abRotationRound3 };
         const UINT8* pbRotationSet       = apbRotationSets[ bRound ];
#endif
         for( bOperation = 0; bOperation < MD5_NUM_OPERATIONS; bOperation++ )
         {
#if( MD5_USE_K_TABLE == 0 )
            const UINT8 abMultiplerK[] = { 1, 5, 3, 7 };
            const UINT8 abOffsetK[] = { 0, 1, 5, 0 };
#endif
            const MD5_t_DigestFunc pnAuxFuncs[] =
            {
               &MD5_AUXILIARY_F,
               &MD5_AUXILIARY_G,
               &MD5_AUXILIARY_H,
               &MD5_AUXILIARY_I
            };

            UINT32 dwRegister;
            UINT8 bBlockIndex;  /* Referred to as 'k' in RFC1321 */
            UINT8 bLutIndex;    /* Referred to as 'i' in RFC1321 */
            UINT8 bRotateCount; /* Referred to as 's' in RFC1321 */

            bLutIndex = bOperation + ( MD5_NUM_OPERATIONS * bRound );

#if( MD5_USE_K_TABLE == 1 )
            bBlockIndex = MD5_abTableK[ bLutIndex ];
#else
            bBlockIndex = bOperation * abMultiplerK[ bRound ];
            bBlockIndex += abOffsetK[ bRound ];
            bBlockIndex %= MD5_NUM_OPERATIONS;
#endif

            dwRegister = ( adwBufferABCD[ MD5_A_INDEX ] +
                           pnAuxFuncs[ bRound ]( adwBufferABCD ) +
                           MD5_GetValueT( bLutIndex ) +
                           psInst->uBlockBuffer.adw[ bBlockIndex ] );

#if( MD5_USE_S_TABLE == 0 )
            bRotateCount = pbRotationSet[ bOperation % MD5_NUM_ROUNDS ];
#else
            bRotateCount = MD5_abTableS[ bLutIndex ];
#endif

            /* Rotate the state registers to keep the same context of the operations above */
            adwBufferABCD[ MD5_A_INDEX ] = adwBufferABCD[ MD5_D_INDEX ];
            adwBufferABCD[ MD5_D_INDEX ] = adwBufferABCD[ MD5_C_INDEX ];
            adwBufferABCD[ MD5_C_INDEX ] = adwBufferABCD[ MD5_B_INDEX ];
            adwBufferABCD[ MD5_B_INDEX ] += MD5_RotateLeft( dwRegister, bRotateCount );
         }
      }

      /* Add in the register results to the last state of the digest */
      for( bRegister = 0; bRegister < MD5_DIGEST_SIZE_DWORDS; bRegister++ )
      {
         psInst->adwDigest[ bRegister ] += adwBufferABCD[ bRegister ];
      }

      psInst->iBlockOffset = 0;
   }
}

#if( MD5_USE_TEST_ROUTINE == 1 )
/*------------------------------------------------------------------------------
** Routine to print to stdout the formated MD5 digest.
**------------------------------------------------------------------------------
** Arguments:
**    psInst  - Pointer to an instance containing the current state of the MD5
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
static void MD5_Print( MD5_InstType* psInst )
{
   UINT8 bDigestIndex;

   for( bDigestIndex = 0; bDigestIndex < MD5_DIGEST_SIZE; bDigestIndex++ )
   {
      MD5_PRINTF( "%02X ", ( (UINT8*)psInst->adwDigest )[ bDigestIndex ] );
   }
}
#endif /* ( MD5_USE_TEST_ROUTINE == 1 ) */

/*******************************************************************************
** Public Services
********************************************************************************
*/

/*------------------------------------------------------------------------------
** This routine initializes the supplied instance structure for computing
** a new MD5 digest.
**------------------------------------------------------------------------------
** Arguments:
**    psInst - Pointer to an instance containing the current state of the MD5
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
void MD5_Init( MD5_InstType* psInst )
{

   const UINT32 adwInitState[ MD5_DIGEST_SIZE_DWORDS ] =
   {
      0x67452301,
      0xEFCDAB89,
      0x98BADCFE,
      0x10325476
   };

   psInst->iBlockOffset = 0;
   psInst->lTotalByteSize = 0;
   MD5_MEMCPY( psInst->adwDigest, adwInitState, sizeof( psInst->adwDigest ) * bCharNumBytes );
}

/*------------------------------------------------------------------------------
** This is the main routine the user should call to supply new data to be
** processed by the MD5-unit.
**------------------------------------------------------------------------------
** Arguments:
**    psInst   - Pointer to an instance containing the current state of the MD5
**    pbData   - Pointer to data to be sent to the working buffer
**    iDataLen - Length of the supplied data in bytes. The length supplied
**               is only limited by the arguments datatype.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
void MD5_Update( MD5_InstType* psInst, const UINT8* pbData, UINT16 iDataLen )
{
   UINT8 bBytesLeftInBlock = MD5_BLOCK_SIZE - psInst->iBlockOffset;
#if( MD5_USE_16BIT_CHAR == 1 )
   UINT16 iOffset = 0;
#endif

   psInst->lTotalByteSize += iDataLen;

   if( bBytesLeftInBlock == 0 )
   {
      MD5_ProcessBlock( psInst );
   }
   else if( iDataLen >= bBytesLeftInBlock )
   {
#if( MD5_USE_16BIT_CHAR == 1 )
      MD5_PORT_CopyOctetsImpl( &psInst->uBlockBuffer.ab, psInst->iBlockOffset, pbData, 0, bBytesLeftInBlock );
      iOffset += bBytesLeftInBlock;
#else
      MD5_MEMCPY( &psInst->uBlockBuffer.ab[ psInst->iBlockOffset ], pbData, bBytesLeftInBlock );
      pbData += bBytesLeftInBlock;
#endif
      psInst->iBlockOffset = MD5_BLOCK_SIZE;
      iDataLen -= bBytesLeftInBlock;

      MD5_ProcessBlock( psInst );
   }

   while( iDataLen != 0 )
   {
      UINT16 iCopySize = iDataLen;

      if( iCopySize > MD5_BLOCK_SIZE )
      {
         iCopySize = MD5_BLOCK_SIZE;
      }
#if( MD5_USE_16BIT_CHAR == 1 )
      MD5_PORT_CopyOctetsImpl( &psInst->uBlockBuffer.ab, psInst->iBlockOffset, pbData,iOffset, iCopySize );
      iOffset += iCopySize;
#else
      MD5_MEMCPY( &psInst->uBlockBuffer.ab[ psInst->iBlockOffset ], pbData, iCopySize );
      pbData += iCopySize;
#endif
      psInst->iBlockOffset += iCopySize;
      iDataLen -= iCopySize;

      MD5_ProcessBlock( psInst );
   }
}

/*------------------------------------------------------------------------------
** This routine provides a simple way to apply a constant value to a range of
** bytes to be processed by the MD5-unit.
**------------------------------------------------------------------------------
** Arguments:
**    psInst  - Pointer to an instance containing the current state of the MD5
**    bValue  - Value to apply to the to the working buffer
**    iCount  - Number of times to apply the value to the working buffer
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
void MD5_UpdateByte( MD5_InstType* psInst, const UINT8 bValue, UINT16 iCount )
{
   UINT8 bBytesLeftInBlock = MD5_BLOCK_SIZE - psInst->iBlockOffset;

   psInst->lTotalByteSize += iCount;

   if( bBytesLeftInBlock == 0 )
   {
      MD5_ProcessBlock( psInst );
   }
   else if( iCount >= bBytesLeftInBlock )
   {
#if( MD5_USE_16BIT_CHAR == 1 )
      MD5_PORT_SetOctetsImpl( &psInst->uBlockBuffer.ab, psInst->iBlockOffset , bValue, bBytesLeftInBlock );
#else
      MD5_MEMSET( &psInst->uBlockBuffer.ab[ psInst->iBlockOffset ], bValue, bBytesLeftInBlock );
#endif
      psInst->iBlockOffset = MD5_BLOCK_SIZE;
      iCount -= bBytesLeftInBlock;
      MD5_ProcessBlock( psInst );
   }

   while( iCount != 0 )
   {
      UINT16 iCopySize = iCount;

      if( iCopySize > MD5_BLOCK_SIZE )
      {
         iCopySize = MD5_BLOCK_SIZE;
      }
#if( MD5_USE_16BIT_CHAR == 1 )
      MD5_PORT_SetOctetsImpl( &psInst->uBlockBuffer.ab, psInst->iBlockOffset, bValue, iCopySize );
#else
      MD5_MEMSET( &psInst->uBlockBuffer.ab[ psInst->iBlockOffset ], bValue, iCopySize );
#endif
      psInst->iBlockOffset += iCopySize;
      iCount -= iCopySize;
      MD5_ProcessBlock( psInst );
   }
}

/*------------------------------------------------------------------------------
** This routine processes any remaining data in the working buffer
** thus providing the final state of the MD5 digest.
** The final steps to producing the digest are as follows:
** - Append 1 (0x80) after the last byte of user-provided data
** - Append the "length" (in bits) of the overall dataset.
** - Append 0-padding to the dataset to align to a size that is evenly divisible by 512.
** - Compute the final block of data that remains in the working buffer.
** - Format the MD5 digest.
**------------------------------------------------------------------------------
** Arguments:
**    psInst  - Pointer to an instance containing the current state of the MD5
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
void MD5_Final( MD5_InstType* psInst )
{
   const UINT8 bPadLeadingOne = 0x80;
   UINT64 lTotalBitSize = psInst->lTotalByteSize * 8;
   UINT8 bZeroPadLength;
   UINT8 bBytesLeftInBlock;

   /* Append 1 */
   MD5_UpdateByte( psInst, bPadLeadingOne, sizeof(bPadLeadingOne) );
   bBytesLeftInBlock = MD5_BLOCK_SIZE - ( psInst->iBlockOffset % MD5_BLOCK_SIZE );

   /* Append 0-padding bits + 64-bit length field. */
   if( bBytesLeftInBlock < ( sizeof(UINT64) * bCharNumBytes ) )
   {
      /* There is not enough room to fit the 64-bit length into this block.
      ** Fill the remainder of the block with zeros, process it, and continue
      ** with zero padding the next block */
      MD5_UpdateByte( psInst, 0, bBytesLeftInBlock );
   }

   bBytesLeftInBlock = MD5_BLOCK_SIZE - ( psInst->iBlockOffset % MD5_BLOCK_SIZE );
   bZeroPadLength = bBytesLeftInBlock - ( sizeof(UINT64) * bCharNumBytes );

   if( bZeroPadLength > 0 )
   {
      MD5_UpdateByte( psInst, 0, bZeroPadLength );
   }

   MD5_Update( psInst, (UINT8*)&lTotalBitSize, sizeof(lTotalBitSize) * bCharNumBytes );
}

/*------------------------------------------------------------------------------
** Routine to perform a single call to compute and finalize an MD5 digest.
** This also illustrates basic usage of the underlying routines that can be
** used for more advanced applications, such as streaming in data to be processed.
**------------------------------------------------------------------------------
** Arguments:
**    psInst  - Pointer to an instance containing the current state of the MD5
**    pbMsg   - Pointer to a message buffer that the MD5 will be computed for
**    iMsgLen - Length of the provided message buffer
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
void MD5_Compute( MD5_InstType* psInst, const UINT8* pbMsg, UINT16 iMsgLen )
{
   MD5_Init( psInst );
   MD5_Update( psInst, pbMsg, iMsgLen );
   MD5_Final( psInst );
}

#if( MD5_USE_TEST_ROUTINE == 1 )
/*------------------------------------------------------------------------------
** Routine to perform a set of predefine tests to ensure that the algorithm
** computes the correct results.
**------------------------------------------------------------------------------
** Arguments:
**    psInst  - Pointer to an instance containing the current state of the MD5
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
BOOL MD5_RunTests( MD5_InstType* psInst )
{
   BOOL fAllPassed = TRUE;
   UINT8 bTestEntry;

   for( bTestEntry = 0; bTestEntry < sizeof( MD5_asTestCases ) / sizeof( MD5_TestStructType );
        bTestEntry++ )
   {
      BOOL fMismatch        = FALSE;
      const char* acTestMsg = MD5_asTestCases[ bTestEntry ].acTestMsg;
#if( MD5_USE_16BIT_CHAR == 1 )
      UINT32 adwExpectedDigest[ MD5_DIGEST_SIZE_DWORDS ];
      char acTestMsgPacked[ 256 ];
      MD5_PORT_StrCpyToPackedImpl( acTestMsgPacked, 0, acTestMsg, strlen( acTestMsg ) );
      MD5_Compute( psInst, (UINT8*)acTestMsgPacked, (UINT16)strlen( acTestMsg ) );
      MD5_PORT_StrCpyToPackedImpl( &adwExpectedDigest, 0, &MD5_asTestCases[ bTestEntry ].abExpectedDigest, MD5_DIGEST_SIZE );
      if( MD5_MEMCMP( psInst->adwDigest, adwExpectedDigest, MD5_DIGEST_SIZE ) != 0 )
#else
      MD5_PRINTF( "TEST_%03d: MSG_SIZE = %ld\t: ", bTestEntry, (UINT32)strlen( acTestMsg ) );

      MD5_Compute( psInst, (UINT8*)acTestMsg, (UINT16)strlen( acTestMsg ) );

      if( MD5_MEMCMP( psInst->adwDigest,
                      MD5_asTestCases[ bTestEntry ].abExpectedDigest,
                      MD5_DIGEST_SIZE ) != 0 )
#endif
      {
         fMismatch  = TRUE;
         fAllPassed = FALSE;
      }

      if( fMismatch )
      {
         MD5_PRINTF( "FAILED\n" );
         MD5_PRINTF( "  MD5: " );
         MD5_Print( psInst );
         MD5_PRINTF( "\n" );
      }
      else
      {
         MD5_PRINTF( "PASSED\n" );
      }
   }

   return fAllPassed;
}
#endif /* ( MD5_USE_TEST_ROUTINE == 1 ) */
