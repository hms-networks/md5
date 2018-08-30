/*******************************************************************************
**    Copyright (C) 2018 HMS Industrial Networks Inc, all rights reserved
********************************************************************************
**
**       File: MD5_int.h
**    Summary: Defines integral types used within the MD5 unit. These types
**             can be overriden or adapted as needed per the requirements of
**             the host platform.
**
********************************************************************************
********************************************************************************
*/

#ifndef HMS_SC_MD5_INT_H_
#define HMS_SC_MD5_INT_H_

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE ( !FALSE )
#endif

#ifndef BOOL
#define BOOL unsigned char
#endif

#ifndef UINT8
#define UINT8 unsigned char
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef UINT32
#define UINT32 unsigned long
#endif

#ifndef UINT64
#define UINT64 unsigned long long
#endif

#endif /* HMS_SC_MD5_INT_H_ */
