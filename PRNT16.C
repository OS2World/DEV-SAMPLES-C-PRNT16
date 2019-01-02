/**************************************************************************/
/*------------------------------- PRNT16.C -------------------------------*/
/*                                                                        */
/* PRINTING TEXT TO THE DEFAULT OS/2 1.3 PRINTER                          */
/*                                                                        */
/* On my printer with my dot matrix printer, the INI settings:            */
/*    PM_SPOOLER                                                          */
/*        PRINTER: EPSON01;                                               */
/*                                                                        */
/*    PM_SPOOLER_PRINTER                                                  */
/*        EPSON01: LPT1;EPSON.FX-286e;EPSON01;;                           */
/*                                                                        */
/* You can use the Splxxx or Devxxx calls to print text to the printer.   */
/* This program shows how to use both. It is conditionally compiled       */
/* depending on the DEV macro in the makefile. It is conditionally linked */
/* using the PROGTYPE macro which is dependent on the DEV macro.          */
/*                                                                        */
/* To make this program use Splxxx calls, comment out the DEV macro in    */
/* the makefile like this: #DEV=-DDEV                                     */
/* This will undefine DEV.                                                */
/*                                                                        */
/* To make this program use Devxxx calls, change the DEV macro in the     */
/* make file to be DEV=-DDEV                                              */
/*                                                                        */
/* NOTES: This program is set up as a PM program if it is compiled to use */
/*        the DevXxx calls since DevOpenDC needs an anchor block handle.  */
/*        If you use the Splxxx Calls, it does not need to be a PM        */
/*        program. The make file controls whether or not it is a PM       */
/*        program by the PROGTYPE macro which is automatically set by the */
/*        defining or not defining of the DEV macro.                      */
/*                                                                        */
/* Rick Fishman                                                           */
/* Code Blazers, Inc.                                                     */
/* 4113 Apricot                                                           */
/* Irvine, CA 92720                                                       */
/* CIS ID: 72251,750                                                      */
/*                                                                        */
/*------------------------------------------------------------------------*/
/**************************************************************************/
#define INCL_DEV
#define INCL_SPL
#define INCL_WINERRORS
#define INCL_WINMESSAGEMGR
#define INCL_WINSHELLDATA

#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

USHORT     DoDevCalls    ( VOID );
USHORT     DoSplCalls    ( VOID );
VOID       PausePrintJob ( VOID );
VOID cdecl DisplayMessage( PSZ szFormat, ... );

#define DOCNAME         "Testdoc.Ric"
#define MESSAGE_SIZE    1024

#ifdef DEV                  // Turn off WinMessageBox if nonPM program
    #define Msg DisplayMessage
#else
    #define Msg printf
#endif

// Default settings

DEVOPENSTRUC dop = { "LPT1Q", "IBM4201", 0L, "PM_Q_RAW", 0L, 0L, 0L, 0L, 0L };

DRIVDATA drivd;

PSZ  szDeviceName;

HSPL hspl;
HDC  hdc;

CHAR szMessage[] = "This text looks alright to me!\n";

CHAR szBuf1[ 256 ];
CHAR szBuf2[ 256 ];

/**********************************************************************/
/*------------------------------ main --------------------------------*/
/*                                                                    */
/*  PROGRAM ENTRYPOINT                                                */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
int main()
{
    USHORT usSize = (SHORT) PrfQueryProfileString( HINI_PROFILE,
                                                   SPL_INI_SPOOLER, "PRINTER",
                                                   NULL, szBuf1,
                                                   (LONG) sizeof( szBuf1 ) );
    if( usSize )
    {
        szBuf1[ usSize - 2 ] = 0;    // get rid of terminating semicolon

        if( PrfQueryProfileString( HINI_PROFILE, SPL_INI_PRINTER, szBuf1, NULL,
                                   szBuf2, (LONG) sizeof( szBuf2 ) ) )
        {
            dop.pszDriverName  = strchr( szBuf2, ';' ) + 1;
            dop.pszLogAddress  = strchr( dop.pszDriverName, ';' ) + 1;
            szDeviceName       = strchr( dop.pszDriverName, '.' );
            dop.pszLogAddress  = strtok( dop.pszLogAddress, ",;" );
            dop.pszDriverName  = strtok( dop.pszDriverName, ",.;" );
        }
        else
            Msg( "PrfQueryProfileString for printer data failed\n" );
    }
    else
        Msg( "PrfQueryProfileString for default printer failed\n" );

#ifdef DEV
    DoDevCalls();
#else
    DoSplCalls();
#endif

    return 0;
}

/**********************************************************************/
/*--------------------------- DoDevCalls -----------------------------*/
/*                                                                    */
/*  DO Dev API CALLS TO PRINT STUFF.                                  */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
USHORT DoDevCalls( )
{
    HDC       hdc;
    ULONG     ulBytes;
    HAB       hab = WinInitialize( 0 );
    HMQ       hmq;

    if( !hab )
        DosBeep( 100, 100 );
    else
        hmq = WinCreateMsgQueue( hab, 0 );

    if( !hmq )
        DosBeep( 100, 100 );

// If this printer driver has multiple flavors (like the EPSON), it will have
// a dot separating the printer driver name from the qualifier. For instance
// my dot-matrix driver is EPSON.FX-286e; szDeviceName now points to the period
// In this case we need to include the driver data to differentiate which
// mode to run the driver in.

    if( szDeviceName )
    {
        drivd.cb = sizeof( DRIVDATA );

        szDeviceName++;

        szDeviceName = strtok( szDeviceName, ";" );

        strcpy( drivd.szDeviceName, szDeviceName );

        dop.pdriv = &drivd;
    }

// Create a printer device context for the print queue

    if( !(hdc = DevOpenDC( hab, OD_QUEUED, "*", 4L,
                           (PDEVOPENDATA) &dop, (HDC) NULL )) )
        Msg( "DevOpenDC failed. RC=%x\n", (USHORT) WinGetLastError( 0 ) );

// Start the spool file and name it

    if( DEV_OK != DevEscape( hdc, DEVESC_STARTDOC, strlen( DOCNAME ),
                             (PBYTE) DOCNAME, &ulBytes, NULL ) )
        Msg( "DevEscape STARTDOC failed. RC=%x\n",
                (USHORT) WinGetLastError( 0 ) );

// This function will show how to put the print job on hold

//    PausePrintJob();

// Write data to the spool file

    if( DEV_OK != DevEscape( hdc, DEVESC_RAWDATA, strlen( szMessage ),
                             (PBYTE) szMessage, &ulBytes, NULL ) )
        Msg( "DevEscape RAWDATA failed. RC=%x\n",
                (USHORT) WinGetLastError( 0 ) );

// Close the spool file and start it printing

    if( DEV_OK != DevEscape( hdc, DEVESC_ENDDOC, 0, NULL, &ulBytes, NULL ) )
        Msg( "DevEscape ENDDOC failed. RC=%x\n",
                (USHORT) WinGetLastError( 0 ) );

// Close the device context

    if( DEV_ERROR == (HMF) DevCloseDC( hdc ) )
        Msg( "DevCloseDC failed. RC=%x\n", (USHORT) WinGetLastError( 0 ) );

    WinDestroyMsgQueue( hmq );

    WinTerminate( hab );

    return 0;
}

/**********************************************************************/
/*--------------------------- DoSplCalls -----------------------------*/
/*                                                                    */
/*  DO Spl API CALLS TO PRINT STUFF.                                  */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
USHORT DoSplCalls( )
{
    HSPL        hspl;
    USHORT      usJobID;

// Open the Print Manager

    if( !(hspl = SplQmOpen( "*", 4L, (PQMOPENDATA) &dop )) )
        Msg( "SplQmOpen failed\n" );

// Start the spool file and name it

    if( !(SplQmStartDoc( hspl, DOCNAME )) )
        Msg( "SplQmStartDoc failed\n" );

// This function will show how to put the print job on hold

//    PausePrintJob();

// Write to the spool file

    if( !SplQmWrite( hspl, strlen( szMessage ), szMessage ) )
        Msg( "SplQmWrite failed\n" );

// End the spool file. This starts it printing.

    if( !(usJobID = SplQmEndDoc( hspl )) )
        Msg( "SplQmEndDoc failed\n" );

//****************************************************************************
// Cannot do the DosPrintJobPause here because SplQmEndDoc starts the job
// printing and then it is too late to pause it (DosPrintJobPause gets a 2164
// error if we issue it after the SplQmEndDoc). Too bad because that call
// returns the job ID. So instead we had to do all the code in the
// PausePrintJob below.
//****************************************************************************

// Close the Print Manager

    if( !SplQmClose( hspl ) )
        Msg( "SplQmClose failed\n" );

    return 0;
}

/**********************************************************************/
/*-------------------------- PausePrintJob ---------------------------*/
/*                                                                    */
/*  PAUSE THE PRINT JOB.                                              */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
VOID PausePrintJob( )
{
    USHORT      usJobID = 0, usJobsReturned, usJobsAvailable, usSize, usCount;
    SPLERR      splerr;
    PPRJINFO2   pprjinfo2, prjPtr;

    usSize = sizeof( PRJINFO2 ) * 10;

    if( !(pprjinfo2 = (PPRJINFO2) malloc( usSize )) )
        Msg( "malloc failed\n" );

    if( splerr = DosPrintJobEnum( NULL, dop.pszLogAddress, 2, (PBYTE) pprjinfo2,
                                  usSize, &usJobsReturned, &usJobsAvailable ) )
        Msg( "DosPrintJobEnum failed. RC = %u\n", splerr );

    for( usCount = 0, prjPtr = pprjinfo2;
         usCount < usJobsReturned;
         usCount++, prjPtr++ )
    {
        if( !stricmp( prjPtr->pszDocument, DOCNAME ) )
        {
            usJobID = prjPtr->uJobId;
            break;
        }
    }

    if( !usJobID )
        Msg( "Couldn't find the job using DosPrintJobEnum.\n" );

    if( splerr = DosPrintJobPause( NULL, usJobID ) )
        Msg( "DosPrintJobPause failed. RC = %u\n", splerr );

    free( pprjinfo2 );
}

/**********************************************************************/
/*------------------------- DisplayMessage ---------------------------*/
/*                                                                    */
/*  INPUT: a message in printf format with its parms                  */
/*                                                                    */
/*  DISPLAY A MESSAGE TO THE USER.                                    */
/*                                                                    */
/*  1. Format the message using vsprintf.                             */
/*  2. Sound a warning sound.                                         */
/*  3. Display the message in a message box.                          */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
VOID cdecl DisplayMessage( PSZ szFormat,... )
{
    PSZ     szMsg;
    va_list argptr;

    if( (szMsg = (PSZ) malloc( MESSAGE_SIZE )) == NULL )
    {
        DosBeep( 1000, 1000 );

        return;
    }

    va_start( argptr, szFormat );

    vsprintf( szMsg, szFormat, argptr );

    va_end( argptr );

    szMsg[ MESSAGE_SIZE - 1 ] = 0;

    WinAlarm( HWND_DESKTOP, WA_WARNING );

    WinMessageBox(  HWND_DESKTOP, HWND_DESKTOP, szMsg,
                    "prnt16.exe", 1, MB_OK | MB_MOVEABLE );

    free( szMsg );
}

