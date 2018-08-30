/*******************************************************************************
**    Copyright (C) 2018 HMS Industrial Networks Inc, all rights reserved
********************************************************************************
**
**       File: MD5_cfg.h
**    Summary: User configurable options of the MD5-unit
**
********************************************************************************
********************************************************************************
*/

#ifndef HMS_SC_MD5_CFG_H_
#define HMS_SC_MD5_CFG_H_

/*
** Processors that can not access byte-addresses (i.e. 16-bit addressable)
** should set this value to 1, else this should be set to 0
*/
#define MD5_USE_16BIT_CHAR          ( 0 )

/*
** Processors that use big-endian notation should set this value to 1.
*/
#define MD5_USE_BIG_ENDIAN          ( 0 )

/*
** Variable lookup table options. If space constrained, the user can
** set these to 0, to use a more computationally intensive approach
** to computing the MD5.
*/
#define MD5_USE_T_TABLE             ( 1 )
#define MD5_USE_S_TABLE             ( 1 )
#define MD5_USE_K_TABLE             ( 1 )

/*
** Enable/disable debug output (for MD5 porting/development purposes).
*/
#define MD5_DEBUG                   ( 0 )

/*
** Enable/disable use of MD5_PRINTF macro. Used mainly in MD5_DEBUG related
** logic but also used by MD5_USE_TEST_ROUTINE.
*/
#define MD5_USE_PRINTF              ( 1 )

/*
** Enable/disable test routine for validation of correct MD5 computation.
*/
#define MD5_USE_TEST_ROUTINE        ( 1 )

#endif /* HMS_SC_MD5_CFG_H_ */
