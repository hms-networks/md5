/*******************************************************************************
**    Copyright (C) 2018 HMS Industrial Networks Inc, all rights reserved
********************************************************************************
**
**       File: MD5_example_app.c
**    Summary: Example console application illustrating how the MD5-unit can
**             be used, while also providing a practical application capable
**             of both generating an MD5 as well as validating against an MD5.
**
********************************************************************************
********************************************************************************
*/

#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "MD5.h"

/*****************************************************************************
** Defines
******************************************************************************
*/

#define MAX_READ_FILE_SIZE             4096
#define CHARACTERS_PER_BYTE            2
#define NUM_ITERATIONS_PER_BECHMARK    10

#define CHECK_ARGUMENT( a, b ) CompareStrings( a, strlen( a ), b, strlen( b ) )

/*****************************************************************************
** Static variables
******************************************************************************
*/

/*
** Set of "sizes" to use for feeding bytes of data into the MD5 algorithm
** for benchmarking purposes.
*/
static const UINT16 aiBenchmarkReadSizes[] = { 1,   3,   10,  13,  63,   64,   128,
                                               256, 511, 512, 513, 1024, 2048, 4096 };

/*
** Various parameters set based on console input arguments
*/
static char* pacInputFilename  = NULL;
static char* pacOutputFilename = NULL;
static char* pacInMd5Filename  = NULL;
static BOOL fTestMode          = FALSE;
static BOOL fPrintHelp         = FALSE;
static BOOL fVerbose           = FALSE;
static BOOL fBenchmark         = FALSE;
static BOOL fWaitForInput      = FALSE;

/*****************************************************************************
** Forward declarations
******************************************************************************
*/

static void PrintHelp( void );
static void HandleWaitForInputOption( void );
static void StartCounter( double* prFrequency, UINT64* plCounterStart );
static double GetCounter( double rFrequency, UINT64 lCounterStart );
static BOOL ParseMd5File( char* acMd5Filename, UINT8* pbMd5 );
static BOOL CompareStrings( char* acStr1, UINT8 bStr1Len, char* acStr2, UINT8 bStr2Len );
static BOOL ParseArguments( int argc, char* argv[] );
static void WriteDigestToFile( const MD5_InstType* psInst, const char* pacOutputFilename );
static void PrintDigest( const MD5_InstType* psInst );
static BOOL ComputeMd5( FILE* psFile, UINT16 iRdSize, MD5_InstType* psMd5Inst, UINT8* pbExpectedDigest );

/*****************************************************************************
** Global routines
******************************************************************************
*/

/*----------------------------------------------------------------------------
** Main console application
*-----------------------------------------------------------------------------
*/
int main( int argc, char* argv[] )
{
   int dwReturn         = 0;
   BOOL fAllTestsPassed = TRUE;
   BOOL fValidArguments = ParseArguments( argc, argv );
   UINT8* pbDigest      = NULL;
   UINT8 abDigest[ MD5_DIGEST_SIZE ];
   MD5_InstType sMd5Inst;
   FILE* psFile = NULL;

   if( fPrintHelp )
   {
      PrintHelp();
   }

   if( !fValidArguments )
   {
      HandleWaitForInputOption();
      return -1;
   }

#if( MD5_USE_TEST_ROUTINE == 1 )
   if( fTestMode )
   {
      printf( "[TEST_MODE]\n" );

      fAllTestsPassed = MD5_RunTests( &sMd5Inst );

      printf( "\n" );
   }
#endif

   if( fVerbose )
   {
      printf( "[INPUT_FILE]\n%s\n\n", pacInputFilename );
   }

   if( pacInMd5Filename != NULL )
   {
      pbDigest = abDigest;
   }

   if( ( pbDigest != NULL ) && !ParseMd5File( pacInMd5Filename, pbDigest ) )
   {
      fAllTestsPassed = FALSE;
   }
   else
   {
      fopen_s( &psFile, pacInputFilename, "rb" );

      if( psFile != NULL )
      {
         const UINT16 iNumTestEntries = sizeof( aiBenchmarkReadSizes ) / sizeof( UINT16 );
         UINT16 iTestReadSizeEntry    = 0;

         if( !fBenchmark )
         {
            /* The last entry is assumed to be the default read size,
            ** when not benchmarking */
            iTestReadSizeEntry = iNumTestEntries - 1;
         }

         for( ; iTestReadSizeEntry < iNumTestEntries; iTestReadSizeEntry++ )
         {
            if( !ComputeMd5(
                   psFile, aiBenchmarkReadSizes[ iTestReadSizeEntry ], &sMd5Inst, pbDigest ) )
            {
               fAllTestsPassed = FALSE;
            }
         }

         fclose( psFile );
      }
      else
      {
         fAllTestsPassed = FALSE;
         printf( "Error: Failed to open file! (%s)\n", pacInputFilename );
      }
   }

   if( !fAllTestsPassed )
   {
      dwReturn = -1;
   }

   if( fBenchmark )
   {
      printf( "[RESULT]\n" );

      if( fAllTestsPassed )
      {
         printf( "PASS" );
      }
      else
      {
         printf( "FAIL" );
      }

      printf( "\n\n" );

      PrintDigest( &sMd5Inst );
   }
   else
   {
      if( pbDigest != NULL )
      {
         if( fVerbose )
         {
            printf( "[RESULT]\n" );
         }

         if( fAllTestsPassed )
         {
            printf( "VALID" );
         }
         else
         {
            printf( "INVALID" );
         }

         printf( "\n\n" );
      }
      else
      {
         PrintDigest( &sMd5Inst );
      }
   }

   if( pacOutputFilename != NULL )
   {
      WriteDigestToFile( &sMd5Inst, pacOutputFilename );
   }

   HandleWaitForInputOption();

   return dwReturn;
}

/*****************************************************************************
** Static routines
******************************************************************************
*/

/*----------------------------------------------------------------------------
** Print help information to stdout
*-----------------------------------------------------------------------------
*/
static void PrintHelp( void )
{
   printf(
      "MD5 Example Console Application\n"
      "\n"
      "USAGE :\n"
      "  MD5.exe -i <filename> [-o <filename>] ... [--help]\n"
      "\n"
      "OPTIONS :\n"
      "  --help             Prints this help menu.\n"
      "  --test             Test mode to check that the application is computing\n"
      "                     correct MD5 results by comparing against known MD5 test\n"
      "                     cases.\n"
      "  --wait             Wait for a ENTER before exiting.\n"
      "  --benchmark        Performes the operation under different circumstances\n"
      "                     while also timestamping and averaging the results.\n"
      "  --md5 <filename>   When using this option, the application will compare the\n"
      "                     computed digest against the speficied MD5 file and\n"
      "                     report whether the file is VALID or INVALID.\n"
      "  -v                 Enables additional verbose output.\n"
      "  -o    <filename>   Output file to write the MD5 digest to. If no output file\n"
      "                     is provided, the digest will only be written to the\n"
      "                     console."
      "\n"
      "PARAMETERS :\n"
      "  -i    <filename>   Input file to compute the MD5 for.\n"
      "\n" );
}

/*----------------------------------------------------------------------------
** Handles whether the application should wait for input before returing from main
*-----------------------------------------------------------------------------
*/
static void HandleWaitForInputOption( void )
{
   char cKey;

   if( fWaitForInput )
   {
      scanf_s( "%c", &cKey, sizeof( cKey ) );
   }
}

/*----------------------------------------------------------------------------
** Starts a counter to benchmarking performance
*-----------------------------------------------------------------------------
*/
static void StartCounter( double* prFrequency, UINT64* plCounterStart )
{
   LARGE_INTEGER lTmp;

   if( !QueryPerformanceFrequency( &lTmp ) )
   {
      printf( "QueryPerformanceFrequency failed!\n" );
   }

   *prFrequency = (double)( lTmp.QuadPart ) / 1000.0f;

   QueryPerformanceCounter( &lTmp );
   *plCounterStart = (UINT64)lTmp.QuadPart;
}

/*----------------------------------------------------------------------------
** Gets the counters elapsed time
*-----------------------------------------------------------------------------
*/
static double GetCounter( double rFrequency, UINT64 lCounterStart )
{
   LARGE_INTEGER uPerfCounter;
   QueryPerformanceCounter( &uPerfCounter );
   return (double)( uPerfCounter.QuadPart - lCounterStart ) / rFrequency;
}

/*----------------------------------------------------------------------------
** Parse and MD5 file (ASCII formatted)
*-----------------------------------------------------------------------------
*/
static BOOL ParseMd5File( char* acMd5Filename, UINT8* pbMd5 )
{
   BOOL fSuccess = TRUE;
   FILE* psFile;

   fopen_s( &psFile, acMd5Filename, "rb" );

   if( psFile == NULL )
   {
      printf( "Error: Failed to open MD5 file! (%s)\n", acMd5Filename );
      fSuccess = FALSE;
   }
   else
   {
      const UINT8 bElementSize = 1;
      UINT16 iBytesToRead = CHARACTERS_PER_BYTE * MD5_DIGEST_SIZE + 1;
      UINT16 iBytesRead;
      char acReadBuffer[ CHARACTERS_PER_BYTE * MD5_DIGEST_SIZE + 1 ];
      UINT8* pbReadData = (UINT8*)acReadBuffer;

      iBytesRead = (UINT16)fread( acReadBuffer, bElementSize, (size_t)iBytesToRead, psFile );

      if( iBytesRead >= MD5_DIGEST_SIZE << 1 )
      {
         UINT8 iByteIndex;

         if( fVerbose )
         {
            printf( "[INPUT_DIGEST]\n" );
         }

         for( iByteIndex = 0; iByteIndex < MD5_DIGEST_SIZE; iByteIndex++ )
         {
            int buf;

            sscanf_s( (char*)pbReadData, "%02x", &buf );
            pbMd5[ iByteIndex ] = (UINT8)buf;
            if( fVerbose )
            {
               printf( "%02X ", pbMd5[ iByteIndex ] );
            }
            pbReadData += CHARACTERS_PER_BYTE;
         }

         if( fVerbose )
         {
            printf( "\n\n" );
         }
      }
      else
      {
         printf( "Error: Unexpected MD5 size read from file!\n" );
         fSuccess = FALSE;
      }

      fclose( psFile );
   }

   return fSuccess;
}

/*----------------------------------------------------------------------------
** Compares to strings for equality
*-----------------------------------------------------------------------------
*/
static BOOL CompareStrings( char* acStr1, UINT8 bStr1Len, char* acStr2, UINT8 bStr2Len )
{
   BOOL fExactMatch = TRUE;

   if( bStr1Len != bStr2Len )
   {
      fExactMatch = FALSE;
   }
   else if( memcmp( acStr1, acStr2, bStr2Len ) != 0 )
   {
      fExactMatch = FALSE;
   }

   return fExactMatch;
}

/*----------------------------------------------------------------------------
** Parse argument passed in from console
*-----------------------------------------------------------------------------
*/
static BOOL ParseArguments( int argc, char* argv[] )
{
   BOOL fValidArguments = TRUE;

   if( argc == 1 )
   {
      fPrintHelp = TRUE;
   }
   else
   {
      int dwArgument = 1;

      while( dwArgument < argc )
      {
         if( CHECK_ARGUMENT( argv[ dwArgument ], "-i" ) )
         {
            pacInputFilename = argv[ ++dwArgument ];
         }
         else if( CHECK_ARGUMENT( argv[ dwArgument ], "-o" ) )
         {
            pacOutputFilename = argv[ ++dwArgument ];
         }
         else if( CHECK_ARGUMENT( argv[ dwArgument ], "-v" ) )
         {
            fVerbose = TRUE;
         }
         else if( CHECK_ARGUMENT( argv[ dwArgument ], "--md5" ) )
         {
            pacInMd5Filename = argv[ ++dwArgument ];
         }
#if( MD5_USE_TEST_ROUTINE == 1 )
         else if( CHECK_ARGUMENT( argv[ dwArgument ], "--test" ) )
         {
            fTestMode = TRUE;
            fVerbose  = TRUE;
         }
#endif
         else if( CHECK_ARGUMENT( argv[ dwArgument ], "--wait" ) )
         {
            fWaitForInput = TRUE;
         }
         else if( CHECK_ARGUMENT( argv[ dwArgument ], "--benchmark" ) )
         {
            fBenchmark = TRUE;
            fVerbose   = TRUE;
         }
         else if( CHECK_ARGUMENT( argv[ dwArgument ], "--help" ) )
         {
            fPrintHelp = TRUE;
            break;
         }
         else
         {
            printf( "Invalid Argument: %s\n", argv[ dwArgument ] );
            fValidArguments = FALSE;
            break;
         }

         dwArgument++;
      }
   }

   if( pacInputFilename == NULL )
   {
      fValidArguments = FALSE;
   }

   if( fValidArguments == FALSE )
   {
      fPrintHelp = TRUE;
   }

   return fValidArguments;
}

/*----------------------------------------------------------------------------
** Write the MD5 digest to the output file
*-----------------------------------------------------------------------------
*/
static void WriteDigestToFile( const MD5_InstType* psInst, const char* pacOutputFilename )
{
   FILE* psFile;
   char acDigest[ ( 2 * MD5_DIGEST_SIZE ) + 1 ];
   UINT8 bDigestIndex;

   fopen_s( &psFile, pacOutputFilename, "w" );

   for( bDigestIndex = 0; bDigestIndex < MD5_DIGEST_SIZE; bDigestIndex++ )
   {
      UINT8 bCharOffset = 2 * bDigestIndex;
      sprintf_s( &acDigest[ bCharOffset ],
                 sizeof( acDigest ) - bCharOffset,
                 "%02x", ( (UINT8*)psInst->adwDigest )[ bDigestIndex ] );
   }

   if( psFile != NULL )
   {
      fwrite( acDigest, 1, sizeof( acDigest ) - 1, psFile );
      fclose( psFile );

      if( fVerbose )
      {
         printf( "[OUTPUT_FILE]\n%s\n\n", pacOutputFilename );
      }

   }
}


/*----------------------------------------------------------------------------
** Print the digest to stdout
*-----------------------------------------------------------------------------
*/
static void PrintDigest( const MD5_InstType* psInst )
{
   UINT8 bDigestIndex;

   if( fVerbose )
   {
      printf( "[DIGEST]\n" );
   }

   for( bDigestIndex = 0; bDigestIndex < MD5_DIGEST_SIZE; bDigestIndex++ )
   {
      printf( "%02X ", ( (UINT8*)psInst->adwDigest )[ bDigestIndex ] );
   }

   if( fVerbose )
   {
      printf( "\n\n" );
   }
}

/*----------------------------------------------------------------------------
** Compute the MD5 of the provided file
*-----------------------------------------------------------------------------
*/
static BOOL ComputeMd5( FILE* psFile, UINT16 iRdSize, MD5_InstType* psMd5Inst, UINT8* pbExpectedDigest )
{
   BOOL fAllIterationsPassed = TRUE;
   UINT16 iIterationsPerSize = NUM_ITERATIONS_PER_BECHMARK;
   UINT16 iIteration;
   double rAvgElapsedMilliseconds = 0.0f;
   double rElapsedMilliseconds;
   UINT8 abReadBuffer[ MAX_READ_FILE_SIZE ];
   UINT16 iElemsToRead = iRdSize;
   UINT16 iElemsRead;
   UINT16 iElemSize = sizeof( UINT8 );

   if( iElemsToRead > sizeof( abReadBuffer ) )
   {
      if( fBenchmark || fVerbose )
      {
         printf( "[WARNING]\nTest size %d exceeds buffer size %d!\n\n",
                     iElemsToRead,
                     sizeof( abReadBuffer ) );
      }

      iElemsToRead = sizeof( abReadBuffer );
   }

   if( fBenchmark )
   {
      printf( "[BENCHMARK]\nRead Size: %d\n\n", iElemsToRead );
   }
   else
   {
      iIterationsPerSize = 1;
   }

   for( iIteration = 0; iIteration < iIterationsPerSize; iIteration++ )
   {
      double rFreq = 0.0;
      UINT64 lCounterStart = 0;

      fseek( psFile, 0, SEEK_SET );

      StartCounter( &rFreq, &lCounterStart );

      MD5_Init( psMd5Inst );

      do
      {
         iElemsRead = (UINT16)fread( abReadBuffer, iElemSize, (size_t)iElemsToRead, psFile );
         MD5_Update( psMd5Inst, abReadBuffer, iElemsRead );

         if( feof( psFile ) || ferror( psFile ) )
         {
            break;
         }
      } while( TRUE );

      MD5_Final( psMd5Inst );

      rElapsedMilliseconds = GetCounter( rFreq, lCounterStart );
      rAvgElapsedMilliseconds += rElapsedMilliseconds;

      if( pbExpectedDigest )
      {
         if( memcmp( psMd5Inst->adwDigest, pbExpectedDigest, MD5_DIGEST_SIZE ) != 0 )
         {
            fAllIterationsPassed = FALSE;
            break;
         }
      }
   }

   if( fBenchmark )
   {
      if( pbExpectedDigest != NULL )
      {
         if( fAllIterationsPassed )
         {
            printf( "Result: PASSED\n" );
         }
         else
         {
            printf( "MD5:" );
            PrintDigest( psMd5Inst );
            printf( "\nResult: FAILED\n" );
         }
      }

      printf( "Avg. Time Elapsed: %lf ms\n\n",
              rAvgElapsedMilliseconds / (double)iIterationsPerSize );
   }

   return fAllIterationsPassed;
}
