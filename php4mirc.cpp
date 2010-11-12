#include <sapi/embed/php_embed.h>

#include <windows.h>
#include <fcntl.h>
#include <io.h>

#define VERSION      "1.5330"
#define BUFFER_SIZE  4096
#define WM_MCOMMAND  WM_USER + 200
#define WM_MEVALUATE WM_USER + 201

typedef struct {
    short major;
    short minor;
} MVERSION;

typedef struct {
    MVERSION mVersion;
    HWND     mHwnd;
    BOOL     mKeep;
} LOADINFO;

HWND mWnd;
BOOL loaded;
HANDLE hMapFile;
char * mData;

void
mIRC_execute ( const char * snippet ) {
    strcpy( mData, snippet );
    SendMessage( mWnd, WM_MCOMMAND, ( WPARAM ) NULL, ( LPARAM ) NULL );
    return;
}

const char *
mIRC_evaluate ( const char * variable ) {
    strcpy( mData, variable );
    return SendMessage( mWnd, WM_MEVALUATE, ( WPARAM ) NULL, ( LPARAM ) NULL ) ?
           mData : "";
}

int __stdcall version( HWND mWnd, HWND aWnd, char * data, char * parms,
                       BOOL show, BOOL nopause ) {
    strcpy( data, "PHP4mIRC v" VERSION " by Sanko Robinson <sanko@cpan.org>" );
    return 3;
}

int myapp_php_ub_write( const char * str, unsigned int str_length TSRMLS_DC ) {
    char *echo;
    spprintf( &echo, 0, "/.signal -n PHP_STDOUT %s%s",
              ( isdigit( *( const char * )str ) ? "" : "" ), str );
    mIRC_execute( echo );
    return str_length;
}

void myapp_php_ub_log_message( char *str ) {
    char *echo;
    spprintf( &echo, 0, "/.signal -n PHP_STDERR %s%s",
              ( isdigit( *( const char * )str ) ? "" : "" ), str );
    mIRC_execute( echo );
}

// Get everything going...
int __stdcall LoadDll( LOADINFO * limIRC  PTSRMLS_DC ) {
    mWnd = limIRC->mHwnd;
    limIRC->mKeep = TRUE; // Set to FALSE if PHP fails
    if ( hMapFile == NULL ) {
        /* Get things set for mIRC<=>php IO */
        hMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, 0, PAGE_READWRITE,
                                      0, 4096, TEXT( "mIRC" ) );
        mData = ( LPSTR )MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS,
                                        0, 0, 0 );
        /* Create "dummy" argc/argv to hide the arguments
         * meant for our actual application */
        int  argc = 1;
        char *argv[2] = { "embed4", NULL };
        /* IO wrappers */
        php_embed_module.ub_write    = myapp_php_ub_write;
        php_embed_module.log_message = myapp_php_ub_log_message;
		/* Make things pretty */
		php_embed_module.name        = "PHP4mIRC";
		php_embed_module.pretty_name = "PHP Embedded in mIRC";
        /* Create our persistant interpreter */
        php_embed_init( argc, argv PTSRMLS_CC );
        /* Let mIRC know all is well*/
        mIRC_execute( "/.signal -n PHP_ONLOAD" );
        loaded = TRUE;
    }
    limIRC->mKeep = loaded ? TRUE : FALSE;
    return 0;
}

int __stdcall UnloadDll( int mTimeout ) {
    if ( mTimeout == 0 ) { /* user called /dll -u */
        /* Destroy PHP */
        php_embed_shutdown( TSRMLS_C );
        /* Let mIRC know all is going according to plan */
        mIRC_execute( "/.signal -n PHP_UNLOAD" );
        /* PHP: mIRC, ...we're just in different places right now. */
        UnmapViewOfFile( mData );
        CloseHandle( hMapFile );
    }
    return 0;
}

int __stdcall php_eval_string (
    HWND   mWnd,  HWND   aWnd,
    char * data,  char * parms,
    BOOL   print, BOOL   nopause
) {
    /* ...what is this junk? Oh, it's...
    * mWnd    - the handle to the main mIRC window.
    * aWnd    - the handle of the window in which the command is being issued,
    *             this might not be the currently active window if the command
    *             is being called by a remote script.
    * data    - the information that you wish to send to the DLL. On return,
    *             the DLL can fill this variable with the command it wants
    *             mIRC to perform if any.
    * parms   - filled by the DLL on return with parameters that it wants mIRC
    *             to use when performing the command that it returns in the
    *             data variable.
    *           Note: The data and parms variables can each hold 900 chars
    *             maximum.
    * show    - FALSE if the . prefix was specified to make the command quiet,
    *            or TRUE otherwise.
    * nopause - TRUE if mIRC is in a critical routine and the DLL must not do
    *            anything that pauses processing in mIRC, eg. the DLL should
    *            not pop up a dialog.
    *
    *  We basically ignore the majority of these which is just simply wrong.
    *  This WILL change in the future.
    */
    int retval = 1; /* continue */
    zend_first_try { zend_eval_string( data, NULL,
                                       "Embed 2 Eval'd string" TSRMLS_CC );
                   }
    zend_catch { retval = 0; /* halt */
               }
    zend_end_try();
    return retval;
    /* We can return an integer to indicate what we want mIRC to do:
    * 0 means that mIRC should /halt processing
    * 1 means that mIRC should continue processing
    * 2 means that we have filled the data variable with a command which mIRC
    *   should perform and we filled parms with the parameters to use, if any,
    *   when performing the command.
    * 3 means that the DLL has filled the data variable with the result that
    *   $dll() as an identifier should return.
    *
    * For now, we always return 3. This may change in future.
    */
}
