/*******************************************************************************
**    Copyright (C) 2018 HMS Industrial Networks Inc, all rights reserved
********************************************************************************
**
**       File: md5.h
**    Summary: Main MD5 header to be included to use the MD5-unit
**
********************************************************************************
********************************************************************************
*/
#ifndef HMS_SC_MD5_H_
#define HMS_SC_MD5_H_

#if( MD5_USE_PRINTF == 1 )
#include <stdio.h>
#endif

#include "MD5_cfg.h"
#include "MD5_int.h"

/*******************************************************************************
** Constants
********************************************************************************
*/

#define MD5_BLOCK_SIZE           ( 64U )
#define MD5_DIGEST_SIZE          ( 16U )
#define MD5_DIGEST_SIZE_DWORDS   ( MD5_DIGEST_SIZE >> 2 )

/*******************************************************************************
** Typedefs
********************************************************************************
*/

typedef union MD5_BlockBuf
{
   UINT32 adw[ MD5_BLOCK_SIZE >> 2 ];
   UINT8 ab[ MD5_BLOCK_SIZE ];
} MD5_BlockBufType;

typedef struct MD5_Instance
{
   UINT64 lTotalByteSize;
   UINT32 adwDigest[ MD5_DIGEST_SIZE_DWORDS ];
   UINT16 iBlockOffset;
   MD5_BlockBufType uBlockBuffer;
} MD5_InstType;

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
void MD5_Init( MD5_InstType* psInst );

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
void MD5_Update( MD5_InstType* psInst, const UINT8* pbData, UINT16 iDataLen );

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
void MD5_UpdateByte( MD5_InstType* psInst, const UINT8 bValue, UINT16 iCount );

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
void MD5_Final( MD5_InstType* psInst );

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
void MD5_Compute( MD5_InstType* psInst, const UINT8* pbMsg, UINT16 iMsgLen );

#if( MD5_USE_TEST_ROUTINE == 1 )
/*------------------------------------------------------------------------------
** Routine to perform a set of predefine tests to ensure that the algorithm
** computes the correct results.
**------------------------------------------------------------------------------
** Arguments:
**    psInst  - Pointer to an instance containing the current state of the MD5
**
** Returns:
**    BOOL - TRUE if all tests have passed.
**------------------------------------------------------------------------------
*/
BOOL MD5_RunTests( MD5_InstType* psInst );
#endif /* ( MD5_USE_TEST_ROUTINE == 1 ) */

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
void MD5_Print( MD5_InstType* psInst );

#endif /* HMS_SC_MD5_H_ */
