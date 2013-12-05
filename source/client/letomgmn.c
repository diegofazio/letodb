/*  $Id: letomgmn.c,v 1.35 2010/08/19 15:43:07 ptsarenko Exp $  */

/*
 * Harbour Project source code:
 * Harbour Leto management functions
 *
 * Copyright 2008 Pavel Tsarenko <tpe2 / at / mail.ru>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

#include "hbapi.h"
#include "hbapiitm.h"
#include "rddleto.h"
#ifdef __XHARBOUR__
#include "hbstack.h"
#endif

#if defined( __XHARBOUR__ ) || ( defined( __HARBOUR__ ) && ( __HARBOUR__ - 0 <= 0x010100 ) )
   #define hb_snprintf snprintf
#endif

static int iFError = 0;
extern LETOCONNECTION * pCurrentConn;
extern USHORT uiConnCount;

extern int leto_getIpFromPath( const char * sSource, char * szAddr, int * piPort, char * szPath, BOOL lFile );
extern void leto_getFileFromPath( const char * sSource, char * szFile );
extern const char * leto_RemoveIpFromPath(const char * szPath);

extern LETOCONNECTION * leto_ConnectionFind( const char * szAddr, int iPort );
extern LETOCONNECTION * leto_ConnectionNew( const char * szAddr, int iPort, const char * szUser, const char * szPass, int iTimeOut );
extern void leto_ConnectionClose( LETOCONNECTION * pConnection );
extern LETOCONNECTION * leto_getConnectionPool( void );
extern LETOCONNECTION * leto_getConnection( int iParam );
extern long int leto_DataSendRecv( LETOCONNECTION * pConnection, const char * sData, ULONG ulLen );
extern BOOL leto_CheckServerVer( LETOCONNECTION * pConnection, USHORT uiVer );
char * leto_firstchar( void );
int leto_getCmdItem( char ** pptr, char * szDest );
char * leto_DecryptText( LETOCONNECTION * pConnection, ULONG * pulLen );
BOOL leto_CheckArea( LETOAREAP pArea );

static LETOCONNECTION * letoParseFile( const char *szSource, char *szFile)
{
   LETOCONNECTION * pConnection = NULL;
   char szAddr[96];
   int iPort;

   if( leto_getIpFromPath( szSource, szAddr, &iPort, szFile, TRUE ) &&
       ( ( ( pConnection = leto_ConnectionFind( szAddr, iPort ) ) != NULL ) ||
         ( ( pConnection = leto_ConnectionNew( szAddr, iPort, NULL, NULL, 0 ) ) ) != NULL ) )
   {
      unsigned int uiLen = strlen( szFile );

      if( szFile[uiLen-1] != '/' && szFile[uiLen-1] != '\\' )
      {
         szFile[uiLen++] = szFile[0];
         szFile[uiLen] = '\0';
      }
      leto_getFileFromPath( szSource, szFile+uiLen );
   }
   return pConnection;
}

HB_FUNC( LETO_FERROR )
{
   hb_retni( iFError );
}

LETOCONNECTION * letoParseParam( const char * szParam, char * szFile )
{
   LETOCONNECTION * pConnection;

   if( strlen( szParam ) >= 2 && szParam[0] == '/' && szParam[1] == '/' )
   {
      pConnection = letoParseFile( szParam, szFile);
   }
   else
   {
      strcpy( szFile, szParam );
      pConnection = pCurrentConn;
   }
   return pConnection;
}

HB_FUNC( LETO_FILE )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];

   if( HB_ISCHAR(1) && ( pConnection = letoParseParam( hb_parc(1), szFile ) ) != NULL )
   {
      char szData[_POSIX_PATH_MAX + 16];

      hb_snprintf( szData, _POSIX_PATH_MAX + 16, "file;01;%s;\r\n", szFile );

      if ( !leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         hb_retl( FALSE );
      }
      else
      {
         char * ptr = leto_firstchar();
         leto_getCmdItem( &ptr, szData ); ptr ++;

         hb_retl( szData[0] == 'T' );
      }
   }
   else
      hb_retl( FALSE );
}

HB_FUNC( LETO_FERASE )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];

   if( HB_ISCHAR(1) && ( pConnection = letoParseParam( hb_parc(1), szFile ) ) != NULL )
   {
      char szData[_POSIX_PATH_MAX + 16];

      hb_snprintf( szData, _POSIX_PATH_MAX + 16, "file;02;%s;\r\n", szFile );

      if ( !leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         iFError = -1;
         hb_retni( -1 );
      }
      else
      {
         char * ptr = leto_firstchar();

         leto_getCmdItem( &ptr, szData ); ptr ++;
         hb_retni( ( szData[0] == 'T') ? 0 : -1 );
         leto_getCmdItem( &ptr, szData ); ptr ++;
         sscanf( szData, "%d", &iFError );
      }
   }
   else
   {
      iFError = -1;
      hb_retni( -1 );
   }
}

HB_FUNC( LETO_FRENAME )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];

   if( HB_ISCHAR(1) && HB_ISCHAR(2) && (pConnection = letoParseParam( hb_parc(1), szFile) ) != NULL )
   {
      char szData[_POSIX_PATH_MAX + _POSIX_PATH_MAX + 16];

      hb_snprintf( szData, _POSIX_PATH_MAX + _POSIX_PATH_MAX + 16, "file;03;%s;%s;\r\n",
         szFile, leto_RemoveIpFromPath( hb_parc(2) ) );

      if ( !leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         iFError = -1;
         hb_retni( -1 );
      }
      else
      {
         char * ptr = leto_firstchar();

         leto_getCmdItem( &ptr, szData ); ptr ++;
         hb_retni( ( szData[0] == 'T') ? 0 : -1 );
         leto_getCmdItem( &ptr, szData ); ptr ++;
         sscanf( szData, "%d", &iFError );
      }
   }
   else
   {
      iFError = -1;
      hb_retni( -1 );
   }
}

HB_FUNC( LETO_MEMOREAD )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];

   if( HB_ISCHAR(1) && ( pConnection = letoParseParam( hb_parc(1), szFile ) ) != NULL )
   {
      char szData[_POSIX_PATH_MAX + 16];

      hb_snprintf( szData, _POSIX_PATH_MAX + 16, "file;04;%s;\r\n", szFile );

      if ( ! leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         hb_retc( "" );
      }
      else
      {
         ULONG ulLen;
         char * ptr = leto_DecryptText( pConnection, &ulLen );

         if( ulLen )
            hb_retclen( ptr, ulLen - 1 );
         else
            hb_retc("");
      }
   }
   else
      hb_retc( "" );
}

// leto_FileRead( cFile, nStart, nLen, @cBuf )
HB_FUNC( LETO_FILEREAD )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];
   ULONG ulStart = hb_parnl(2);
   ULONG ulLen   = hb_parnl(3);
   ULONG ulMesLen;

   iFError = -1;

   if( HB_ISCHAR(1) && ulLen > 0 && ( pConnection = letoParseParam( hb_parc(1), szFile ) ) != NULL )
   {
      char szData[_POSIX_PATH_MAX + 50];

      hb_snprintf( szData, _POSIX_PATH_MAX + 50, "file;10;%s;%lu;%lu;\r\n", szFile, ulStart, ulLen );

      ulMesLen = leto_DataSendRecv( pConnection, szData, 0 );
      if ( ulMesLen >= 5 )
      {
         char * ptr = leto_firstchar();
         if( *(ptr-1) == '+' )
         {
            leto_getCmdItem( &ptr, szData ); ptr ++;
            if( szData[0] == 'T' )
            {
               ulLen = HB_GET_LE_UINT32( ptr );
               ptr  += 4;

               if( ulLen < ulMesLen )
               {
                  hb_storclen( ptr, ulLen, 4 );
                  hb_retnl( ulLen );
                  iFError = 0;
                  return;
               }
               else
                  leto_writelog( NULL,"!Error! LETO_FREAD! (message length > packet)" );
            }
            else
            {
               leto_getCmdItem( &ptr, szData );
               sscanf( szData, "%d;", &iFError );
            }
         }
      }
      else
      {
         leto_writelog( NULL,"!Error! LETO_FREAD! broken message (length < 5)" );
      }
   }

   hb_retnl( -1 );
}

HB_FUNC( LETO_MEMOWRITE )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];

   if( HB_ISCHAR(1) && HB_ISCHAR(2) && ( pConnection = letoParseParam( hb_parc(1), szFile ) ) != NULL )
   {
      ULONG ulLen, ulBufLen = hb_parclen(2);
      char *szData = hb_xgrab( _POSIX_PATH_MAX + 32 + ulBufLen );

      hb_snprintf( szData, _POSIX_PATH_MAX + 32, "file;13;%s;;;", szFile );
      ulLen = strlen( szData );

      HB_PUT_LE_UINT32( szData+ulLen, ulBufLen );  ulLen += 4;
      memcpy( szData+ulLen, hb_parc(2), ulBufLen );
      ulLen += ulBufLen;

      szData[ulLen  ] = '\r';
      szData[ulLen+1] = '\n';
      szData[ulLen+2] = '0';
      ulLen += 2;

      if ( ! leto_DataSendRecv( pConnection, szData, ulLen ) )
      {
         hb_retl( FALSE );
      }
      else
      {
         char * ptr = leto_firstchar();
         if( *(ptr-1) == '+' )
         {
            leto_getCmdItem( &ptr, szData ); ptr ++;
            if( szData[0] == 'T' )
            {
               hb_retl( TRUE );
            }
            else
            {
               leto_getCmdItem( &ptr, szData );
               sscanf( szData, "%d;", &iFError );
               hb_retl( FALSE );
            }
         }
         else
            hb_retl( FALSE );
      }
      hb_xfree( szData );
   }
   else
      hb_retl( FALSE );
}

// leto_FileWrite( cFile, nStart, cBuf )
HB_FUNC( LETO_FILEWRITE )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];
   ULONG ulStart  = hb_parnl(2);
   ULONG ulBufLen = hb_parclen(3);

   iFError = -1;

   if( HB_ISCHAR(1) && HB_ISCHAR(3) && ulBufLen > 0 && ( pConnection = letoParseParam( hb_parc(1), szFile ) ) != NULL )
   {
      ULONG ulLen;
      char *szData = hb_xgrab( _POSIX_PATH_MAX + 50 + ulBufLen );

      hb_snprintf( szData, _POSIX_PATH_MAX + 50, "file;14;%s;%lu;%lu;", szFile, ulStart, ulBufLen );
      ulLen = strlen( szData );

      HB_PUT_LE_UINT32( szData+ulLen, ulBufLen );  ulLen += 4;
      memcpy( szData+ulLen, hb_parc(3), ulBufLen );
      ulLen += ulBufLen;

      szData[ulLen  ] = '\r';
      szData[ulLen+1] = '\n';
      szData[ulLen+2] = '0';
      ulLen += 2;

      if ( ! leto_DataSendRecv( pConnection, szData, ulLen ) )
      {
         hb_retl( FALSE );
      }
      else
      {
         char * ptr = leto_firstchar();
         if( *(ptr-1) == '+' )
         {
            leto_getCmdItem( &ptr, szData ); ptr ++;
            if( szData[0] == 'T' )
            {
               hb_retl( TRUE );
            }
            else
            {
               leto_getCmdItem( &ptr, szData );
               sscanf( szData, "%d;", &iFError );
               hb_retl( FALSE );
            }
         }
         else
            hb_retl( FALSE );
      }
      hb_xfree( szData );
   }
   else
      hb_retl( FALSE );
}

// leto_FileSize( cFile )
HB_FUNC( LETO_FILESIZE )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];
   ULONG ulLen = 0;

   iFError = -1;

   if( HB_ISCHAR(1) && ( pConnection = letoParseParam( hb_parc(1), szFile ) ) != NULL )
   {
      char szData[_POSIX_PATH_MAX + 50];

      hb_snprintf( szData, _POSIX_PATH_MAX + 50, "file;11;%s;\r\n", szFile );

      if ( leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         char * ptr = leto_firstchar();
         if( *(ptr-1) == '+' )
         {
            leto_getCmdItem( &ptr, szData ); ptr ++;
            if( szData[0] == 'T' )
            {
               leto_getCmdItem( &ptr, szData );
               sscanf( szData, "%lu;", &ulLen );
               hb_retnl( ulLen );
               return;
            }
            else
            {
               leto_getCmdItem( &ptr, szData );
               sscanf( szData, "%d;", &iFError );
            }
         }
      }
   }

   hb_retnl( -1 );
}

// leto_Directory( cPathSpec, cAttributes )
HB_FUNC( LETO_DIRECTORY )
{
   LETOCONNECTION * pConnection;
   char szData[62 + _POSIX_PATH_MAX];
   PHB_ITEM aInfo;

   iFError = -1;
   aInfo = hb_itemArrayNew( 0 );

   strcpy( szData, "file;12;" );
   if( HB_ISCHAR(1) && ( pConnection = letoParseParam( hb_parc(1), szData + strlen(szData) ) ) != NULL )
   {
      if( HB_ISCHAR(2) )
         hb_snprintf( szData + strlen(szData), 32, ";%s;\r\n", hb_parc(2) );
      else
         strcat( szData, ";;\r\n" );

      if ( leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         char * ptr = leto_firstchar();
         if( *(ptr-1) == '+' )
         {
            leto_getCmdItem( &ptr, szData ); ptr ++;
            if( szData[0] == 'T' )
            {
               USHORT uiItems;
               ULONG ulData;
               PHB_ITEM pSubarray = hb_itemNew( NULL );

               uiItems = 1;
               while( leto_getCmdItem( &ptr, szData ) )
               {
                  ptr ++;
                  if( uiItems == 1 )
                     hb_arrayNew( pSubarray, 5 );

                  switch( uiItems )
                  {
                     case 1:
                     case 4:
                     case 5:
                        hb_arraySetC( pSubarray, uiItems, szData );
                        break;
                     case 2:
                        sscanf( szData, "%lu", &ulData );
                        hb_arraySetNInt( pSubarray, 2, ulData );
                        break;
                     case 3:
                        sscanf( szData, "%lu", &ulData );
                        hb_arraySetDL( pSubarray, 3, ulData );
                        break;
                  }
                  uiItems ++;
                  if( uiItems > 5 )
                  {
                     uiItems = 1;
                     hb_arrayAddForward( aInfo, pSubarray );
                  }
               }

               hb_itemRelease( pSubarray );
            }
            else
            {
               leto_getCmdItem( &ptr, szData );
               sscanf( szData, "%d;", &iFError );
            }
         }
      }
   }
   hb_itemReturnRelease( aInfo );
}

HB_FUNC( LETO_MAKEDIR )
{
   LETOCONNECTION * pConnection;
   char szFile[_POSIX_PATH_MAX + 1];

   if( HB_ISCHAR(1) && ( pConnection = letoParseParam( hb_parc(1), szFile ) ) != NULL )
   {
      char szData[_POSIX_PATH_MAX + 16];

      sprintf( szData,"file;05;%s;\r\n", szFile );

      if ( !leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         iFError = -1;
         hb_retni( -1 );
      }
      else
      {
         char * ptr = leto_firstchar();

         leto_getCmdItem( &ptr, szData ); ptr ++;
         hb_retni( ( szData[0] == 'T') ? 0 : -1 );
         leto_getCmdItem( &ptr, szData ); ptr ++;
         sscanf( szData, "%d", &iFError );
      }
   }
   else
   {
      iFError = -1;
      hb_retni( -1 );
   }
}

HB_FUNC( LETO_UDFEXIST )
{
   LETOCONNECTION * pConnection;
   char szFuncName[ HB_SYMBOL_NAME_LEN + 1 ];

   if( HB_ISCHAR(1) && ( pConnection = letoParseParam( hb_parc(1), szFuncName ) ) != NULL )
   {
      char szData[ HB_SYMBOL_NAME_LEN + 15], *ptr;

      ptr = (szFuncName[0] == '/') ? szFuncName + 1 : szFuncName;
      hb_snprintf( szData, HB_SYMBOL_NAME_LEN + 15, "udf_fun;03;%s;\r\n", ptr );
      if ( !leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         hb_retl( FALSE );
      }
      else
      {
         ptr = leto_firstchar();

         leto_getCmdItem( &ptr, szData ); ptr ++;
         hb_retl( ( szData[0] == 'T') );
      }
   }
   else
      hb_retl( FALSE );
}

HB_FUNC( LETO_UDF )
{
   LETOCONNECTION * pConnection;
   char szFuncName[ HB_SYMBOL_NAME_LEN + 1 ];

   if( HB_ISCHAR(1) && ( pConnection = letoParseParam( hb_parc(1), szFuncName ) ) != NULL )
   {
      char *szData = hb_xgrab( hb_parclen(1) + hb_parclen(2) + 12 );

      sprintf( szData,"usr;01;%s;%s;\r\n", szFuncName+1, hb_parc(2) );

      if ( !leto_DataSendRecv( pConnection, szData, 0 ) )
      {
         hb_ret( );
      }
      else
      {
         char * ptr = leto_firstchar();
         ULONG ulLen = HB_GET_LE_UINT32( ptr );

         if( ulLen )
            hb_retclen( ptr + sizeof( ULONG ), ulLen );
         else
            hb_ret( );
      }
      hb_xfree(szData);
   }
   else
      hb_ret( );
}

static void leto_SetPath( LETOCONNECTION * pConnection, const char * szPath )
{
   if( pConnection->szPath )
      hb_xfree( pConnection->szPath );
   pConnection->szPath = (char*) hb_xgrab( strlen( szPath ) + 1 );
   strcpy( pConnection->szPath, szPath );
}

HB_FUNC( LETO_CONNECT )
{
   LETOCONNECTION * pConnection;
   char szAddr[96];
   char szPath[_POSIX_PATH_MAX + 1];
   int iPort;
   const char * szUser = (HB_ISNIL(2)) ? NULL : hb_parc(2);
   const char * szPass = (HB_ISNIL(3)) ? NULL : hb_parc(3);
   int iTimeOut = hb_parni(4);

   szPath[0] = '\0';
   if( HB_ISCHAR(1) && leto_getIpFromPath( hb_parc(1), szAddr, &iPort, szPath, FALSE ) &&
       ( ( ( pConnection = leto_ConnectionFind( szAddr, iPort ) ) != NULL ) ||
         ( ( pConnection = leto_ConnectionNew( szAddr, iPort, szUser, szPass, iTimeOut ) ) ) != NULL ) )
   {
      pCurrentConn = pConnection;
      if( HB_ISNUM(5) )
         pConnection->uiBufRefreshTime = hb_parni(5);

      if( strlen(szPath) > 1 && pConnection->szPath == NULL )
         leto_SetPath( pConnection, szPath );
      hb_retni( pConnection - leto_getConnectionPool() + 1 );
   }
   else
   {
      pCurrentConn = NULL;
      hb_retni( -1 );
   }
}

HB_FUNC( LETO_DISCONNECT )
{
   LETOCONNECTION * pConnection = leto_getConnection( 1 );

   if( pConnection )
   {
      leto_ConnectionClose( pConnection );
      if( pConnection == pCurrentConn )
         pCurrentConn = NULL;
   }
   hb_ret( );
}

HB_FUNC( LETO_SETCURRENTCONNECTION )
{
   USHORT uiConn = hb_parni(1);

   if( uiConn && uiConn <= uiConnCount )
      pCurrentConn = leto_getConnectionPool() + uiConn - 1;
   else
      pCurrentConn = NULL;
   hb_ret( );
}

HB_FUNC( LETO_GETCURRENTCONNECTION )
{
   if( pCurrentConn )
      hb_retni( pCurrentConn - leto_getConnectionPool() + 1 );
   else
      hb_retni( -1 );
}

HB_FUNC( LETO_GETSERVERVERSION )
{
   LETOCONNECTION * pConnection = leto_getConnection( 2 );
   if( pConnection )
      hb_retc( HB_ISNIL( 1 ) ? pConnection->szVersion : pConnection->szVerHarbour );
   else
      hb_retc( "" );
}

HB_FUNC( LETO_GETSRVVER )
{
   LETOCONNECTION * pConnection = leto_getConnection( 2 );
   if( pConnection )
      hb_retni( pConnection->uiMajorVer*100 + pConnection->uiMinorVer );
   else
      hb_ret();
}

HB_FUNC( LETO_PATH )
{
   LETOCONNECTION * pConnection = leto_getConnection( 2 );
   if( pConnection )
   {
      hb_retc( pConnection->szPath ? pConnection->szPath : "");
      if( HB_ISCHAR(1) )
      {
         leto_SetPath( pConnection, hb_parc( 1 ) );
      }
   }
   else
      hb_retc( "" );
}

HB_FUNC( LETO_MGGETINFO )
{
   if( pCurrentConn )
   {
      if( leto_DataSendRecv( pCurrentConn, "mgmt;00;\r\n", 0 ) )
      {
         PHB_ITEM temp;
         PHB_ITEM aInfo;
         char szData[_POSIX_PATH_MAX + 1];
         char * ptr = leto_firstchar();
         int i;

         if( *(ptr-1) == '+' )
         {
            aInfo = hb_itemArrayNew( 17 );
            for( i=1; i<=17; i++ )
            {
               if( !leto_getCmdItem( &ptr, szData ) )
                  break;

               ptr ++;
               temp = hb_itemPutCL( NULL, szData, strlen(szData) );
               hb_itemArrayPut( aInfo, i, temp );
               hb_itemRelease( temp );
            }

            hb_itemReturn( aInfo );
            hb_itemRelease( aInfo );
         }
      }
   }
}

HB_FUNC( LETO_MGGETUSERS )
{
   char szData[_POSIX_PATH_MAX + 1];

   if( pCurrentConn )
   {
      if( HB_ISNIL(1) )
         hb_snprintf( szData, _POSIX_PATH_MAX + 1, "mgmt;01;\r\n" );
      else
         hb_snprintf( szData, _POSIX_PATH_MAX + 1, "mgmt;01;%s;\r\n", hb_parc(1) );
      if ( leto_DataSendRecv( pCurrentConn, szData, 0 ) )
      {
         int iUsers, i, j;
         char * ptr = leto_firstchar();
         PHB_ITEM pArray, pArrayItm;

         if( *(ptr-1) == '+' )
         {
            if( !leto_getCmdItem( &ptr, szData ) ) return; ptr ++;
            sscanf( szData, "%d", &iUsers );
            pArray = hb_itemArrayNew( iUsers );
            for ( i = 1; i <= iUsers; i++ )
            {
               pArrayItm = hb_arrayGetItemPtr( pArray, i );
               hb_arrayNew( pArrayItm, 5 );
               for( j=1; j<=5; j++ )
               {
                  if( !leto_getCmdItem( &ptr, szData ) ) return; ptr ++;
                  hb_itemPutC( hb_arrayGetItemPtr( pArrayItm, j ), szData );
               }
            }
            hb_itemReturn( pArray );
            hb_itemRelease( pArray );
         }
      }
   }
}

HB_FUNC( LETO_MGGETTABLES )
{
   char szData[_POSIX_PATH_MAX + 1];

   if( pCurrentConn )
   {
      if( HB_ISNIL(1) )
         hb_snprintf( szData, _POSIX_PATH_MAX + 1, "mgmt;02;\r\n" );
      else
         hb_snprintf( szData, _POSIX_PATH_MAX + 1, "mgmt;02;%s;\r\n", hb_parc(1) );
      if ( leto_DataSendRecv( pCurrentConn, szData, 0 ) )
      {
         int iTables, i, j;
         char * ptr = leto_firstchar();
         PHB_ITEM pArray, pArrayItm;

         if( *(ptr-1) == '+' )
         {
            if( !leto_getCmdItem( &ptr, szData ) ) return; ptr ++;
            sscanf( szData, "%d", &iTables );
            pArray = hb_itemArrayNew( iTables );
            for ( i = 1; i <= iTables; i++ )
            {
               pArrayItm = hb_arrayGetItemPtr( pArray, i );
               hb_arrayNew( pArrayItm, 2 );
               for( j=1; j<=2; j++ )
               {
                  if( !leto_getCmdItem( &ptr, szData ) ) return; ptr ++;
                  hb_itemPutC( hb_arrayGetItemPtr( pArrayItm, j ), szData );
               }
            }
            hb_itemReturn( pArray );
            hb_itemRelease( pArray );
         }
      }
   }
}

HB_FUNC( LETO_MGKILL )
{
   char szData[32];

   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) )
      {
         hb_snprintf( szData, 32, "mgmt;09;%s;\r\n", hb_parc(1) );
         leto_DataSendRecv( pCurrentConn, szData, 0 );
      }
   }
}

HB_FUNC( LETO_MGGETTIME )
{
   if( pCurrentConn )
   {
      if( leto_DataSendRecv( pCurrentConn, "mgmt;03;\r\n", 0 ) )
      {
         PHB_ITEM temp;
         PHB_ITEM aInfo;
         char szData[32];
         char * ptr = leto_firstchar();
         int i;

         if( *(ptr-1) == '+' )
         {
            aInfo = hb_itemArrayNew( 2 );
            for( i = 1; i <= 2; i++ )
            {
               if( !leto_getCmdItem( &ptr, szData ) )
               {
                  hb_itemReturn( aInfo );
                  hb_itemRelease( aInfo );
                  return;
               }
               ptr ++;
               if( i == 1 )
               {
                  int iOvf;
                  ULONG ulDate = hb_strValInt( szData, &iOvf );
                  temp = hb_itemPutDL( NULL, ulDate );
               }
               else
               {
                  temp = hb_itemPutND( NULL, hb_strVal( szData, 10 ) );
               }
               hb_itemArrayPut( aInfo, i, temp );
               hb_itemRelease( temp );
            }

            hb_itemReturn( aInfo );
            hb_itemRelease( aInfo );
         }
      }
   }
}

HB_FUNC( LETO_SETSKIPBUFFER )
{
   LETOAREAP pArea = (LETOAREAP) hb_rddGetCurrentWorkAreaPointer();

   if( leto_CheckArea( pArea ) )
   {
      if( !HB_ISNIL(1) )
      {
         LETOCONNECTION * pConnection = leto_getConnectionPool() + pArea->uiConnection;
         if( leto_CheckServerVer( pConnection, 206 ) )
         {
            pArea->uiSkipBuf = hb_parni(1);
         }
         else
         {
            char szData[32];
            hb_snprintf( szData, 32, "set;02;%lu;%d;\r\n", pArea->hTable, hb_parni(1) );
            leto_DataSendRecv( pConnection, szData, 0 );
         }
      }
      //hb_retni( pArea->Buffer.uiShoots );
   }
   else
      hb_retni( 0 );
}

HB_FUNC( LETO_USERADD )
{
   char szData[96];
   char szPass[54];
   const char * szAccess = ( HB_ISNIL(3) )? "" : hb_parc(3);
   ULONG ulLen;
   char szKey[LETO_MAX_KEYLENGTH+1];

   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) && !HB_ISNIL(2) )
      {
         if( ( ulLen = hb_parclen(2) ) > 0 )
         {
            USHORT uiKeyLen = strlen(LETO_PASSWORD);

            if( uiKeyLen > LETO_MAX_KEYLENGTH )
               uiKeyLen = LETO_MAX_KEYLENGTH;

            memcpy( szKey, LETO_PASSWORD, uiKeyLen );
            szKey[uiKeyLen] = '\0';
            if( pCurrentConn->cDopcode[0] )
            {
               szKey[0] = pCurrentConn->cDopcode[0];
               szKey[1] = pCurrentConn->cDopcode[1];
            }
            leto_encrypt( hb_parc(2), ulLen, szData, &ulLen, szKey );
            leto_byte2hexchar( szData, (int)ulLen, szPass );
            szPass[ulLen*2] = '\0';
         }

         hb_snprintf( szData, 96, "admin;uadd;%s;%s;%s;\r\n", hb_parc(1), szPass, szAccess );
         if( leto_DataSendRecv( pCurrentConn, szData, 0 ) )
         {
            char * ptr = leto_firstchar();
            hb_retl( *ptr == '+' );
            return;
         }
      }
   }
   hb_retl( 0 );
}

HB_FUNC( LETO_USERPASSWD )
{
   char szData[96];
   char szPass[54];
   ULONG ulLen;
   char szKey[LETO_MAX_KEYLENGTH+1];

   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) && !HB_ISNIL(2) )
      {
         if( ( ulLen = hb_parclen(2) ) > 0 )
         {
            USHORT uiKeyLen = strlen(LETO_PASSWORD);

            if( uiKeyLen > LETO_MAX_KEYLENGTH )
               uiKeyLen = LETO_MAX_KEYLENGTH;

            memcpy( szKey, LETO_PASSWORD, uiKeyLen );
            szKey[uiKeyLen] = '\0';
            if( pCurrentConn->cDopcode[0] )
            {
               szKey[0] = pCurrentConn->cDopcode[0];
               szKey[1] = pCurrentConn->cDopcode[1];
            }
            leto_encrypt( hb_parc(2), ulLen, szData, &ulLen, szKey );
            leto_byte2hexchar( szData, (int)ulLen, szPass );
            szPass[ulLen*2] = '\0';
         }

         hb_snprintf( szData, 96, "admin;upsw;%s;%s;\r\n", hb_parc(1), szPass );
         if( leto_DataSendRecv( pCurrentConn, szData, 0 ) )
         {
            char * ptr = leto_firstchar();
            hb_retl( *ptr == '+' );
            return;
         }
      }
   }
   hb_retl( 0 );
}

HB_FUNC( LETO_USERRIGHTS )
{
   char szData[96];

   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) && !HB_ISNIL(2) )
      {
         hb_snprintf( szData, 96, "admin;uacc;%s;%s;\r\n", hb_parc(1), hb_parc(2) );
         if( leto_DataSendRecv( pCurrentConn, szData, 0 ) )
         {
            char * ptr = leto_firstchar();
            hb_retl( *ptr == '+' );
            return;
         }
      }
   }
   hb_retl( 0 );
}

HB_FUNC( LETO_USERFLUSH )
{
   char szData[24];

   if( pCurrentConn )
   {
      hb_snprintf( szData, 24, "admin;flush;\r\n" );
      if( leto_DataSendRecv( pCurrentConn, szData, 0 ) )
      {
         char * ptr = leto_firstchar();
         hb_retl( *ptr == '+' );
         return;
      }
   }
   hb_retl( 0 );
}

HB_FUNC( LETO_USERGETRIGHTS )
{

   if( pCurrentConn )
   {
      hb_retclen( pCurrentConn->szAccess,3 );
   }
}

HB_FUNC( LETO_VARSET )
{
   char *pData, *ptr;
   char szLong[32], cType, cFlag1 = ' ', cFlag2 = ' ';
   ULONG ulLen;
   LONG lValue;
   BOOL bRes = 0;
   USHORT uiFlags = (HB_ISNIL(4))? 0 : hb_parni(4);
   BOOL bPrev = HB_ISBYREF( 5 );

   iFError = 0;
   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) && !HB_ISNIL(2) && !HB_ISNIL(3) )
      {
         ulLen = 24 + hb_parclen(1) + hb_parclen(2);
         if( HB_ISLOG(3) )
         {
            ulLen += 2;
            cType = '1';
         }
         else if( HB_ISNUM(3) )
         {
            hb_snprintf( szLong, 32, "%ld", hb_parnl(3) );
            ulLen += strlen( szLong );
            cType = '2';
         }
         else if( HB_ISCHAR(3) )
         {
            ulLen += hb_parclen(3);
            cType = '3';
         }
         else
         {
            hb_retl( 0 );
            return;
         }
         ptr = (char *) hb_parc(1);
         while( *ptr )
            if( *ptr++ == ';' )
            {
               hb_retl( 0 );
               return;
            }
         ptr = (char *) hb_parc(2);
         while( *ptr )
            if( *ptr++ == ';' )
            {
               hb_retl( 0 );
               return;
            }
/*
         if( hb_parclen(1) > 15 || hb_parclen(2) > 15 )
         {
            hb_retl( 0 );
            return;
         }
*/
         pData = ( char * ) hb_xgrab( ulLen );
         hb_snprintf( pData, ulLen, "var;set;%s;%s;", hb_parc(1), hb_parc(2) );
         ptr = pData + strlen( pData );
         if( cType == '1' )
            *ptr++ = ( ( hb_parl(3) )? '1' : '0' );
         else if( cType == '2' )
         {
            memcpy( ptr, szLong, strlen( szLong ) );
            ptr += strlen( szLong );
         }
         else
         {
            ULONG ul;
            memcpy( ptr, hb_parc(3), ulLen = hb_parclen(3) );
            for( ul=0; ul < ulLen; ul++, ptr++ )
               if( *ptr == ';' )
                  *ptr = '\1';
         }
         cFlag1 |= ( uiFlags & ( LETO_VCREAT | LETO_VOWN | LETO_VDENYWR | LETO_VDENYRD ) );
         if( bPrev )
            cFlag2 |= LETO_VPREVIOUS;
         *ptr++ = ';';
         *ptr++ = cType;
         *ptr++ = cFlag1;
         *ptr++ = cFlag2;
         *ptr++ = ';'; *ptr++ = '\r'; *ptr++ = '\n';
         *ptr++ = '\0';

         if( leto_DataSendRecv( pCurrentConn, pData, 0 ) )
         {
            ptr = leto_firstchar() - 1;
            bRes = ( *ptr == '+' );
            ptr++;
         }
         hb_xfree( pData );
         if( bRes && bPrev )
         {
            cType = *ptr;
            ptr += 2;
            if( cType == '1' )
               hb_storl( *ptr == '1', 5 );
            else if( cType == '2' )
            {
               sscanf( ptr, "%ld", &lValue );
               hb_stornl( lValue, 5 );
            }
            else if( cType == '3' )
            {
               pData = ptr;
               while( *ptr != ';' )
               {
                  if( *ptr == '\1' )
                     *ptr = ';';
                  ptr ++;
               }
               hb_storclen( pData, ptr-pData, 5 );
            }
            else
            {
                hb_stor( 5 );
            }
         }
         else if( !bRes )
            sscanf( ptr+1, "%u", &iFError );
      }
   }
   hb_retl( bRes );
}

HB_FUNC( LETO_VARGET )
{
   char *pData, *ptr;
   char cType;
   LONG lValue;
   ULONG ulMemSize;

   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) && !HB_ISNIL(2) )
      {
         ulMemSize = 16 + hb_parclen(1) + hb_parclen(2);
         pData = ( char * ) hb_xgrab( ulMemSize );
         hb_snprintf( pData, ulMemSize, "var;get;%s;%s;\r\n", hb_parc(1), hb_parc(2) );

         if( leto_DataSendRecv( pCurrentConn, pData, 0 ) )
         {
            hb_xfree( pData );
            ptr = leto_firstchar() - 1;
            if( *ptr == '+' )
            {
               cType = *(ptr+1);
               if( cType == '1' )
                  hb_retl( *(ptr+3) == '1' );
               else if( cType == '2' )
               {
                  sscanf( ptr+3, "%ld", &lValue );
                  hb_retnl( lValue );
               }
               else
               {
                  ptr += 3;
                  pData = ptr;
                  while( *ptr != ';' )
                  {
                     if( *ptr == '\1' )
                        *ptr = ';';
                     ptr ++;
                  }
                  hb_retclen( pData, ptr-pData );
               }
               return;
            }
            else
               sscanf( ptr+1, "%u", &iFError );
         }
         else
            hb_xfree( pData );
      }
   }
   hb_ret();
}

HB_FUNC( LETO_VARINCR )
{
   char *pData, *ptr;
   char cFlag1 = ' ';
   LONG lValue;
   USHORT uiFlags = (HB_ISNIL(3))? 0 : hb_parni(3);
   ULONG ulMemSize;

   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) && !HB_ISNIL(2) )
      {
         cFlag1 |= ( uiFlags & ( LETO_VCREAT | LETO_VOWN | LETO_VDENYWR | LETO_VDENYRD ) );
         if( uiFlags & LETO_VOWN )
            cFlag1 |= LETO_VOWN;

         ulMemSize = 24 + hb_parclen(1) + hb_parclen(2);
         pData = ( char * ) hb_xgrab( ulMemSize );
         hb_snprintf( pData, ulMemSize, "var;inc;%s;%s;2%c!;\r\n", hb_parc(1), hb_parc(2), cFlag1 );

         if( leto_DataSendRecv( pCurrentConn, pData, 0 ) )
         {
            hb_xfree( pData );
            ptr = leto_firstchar() - 1;
            if( *ptr == '+' )
            {
               sscanf( ptr+3, "%ld", &lValue );
               hb_retnl( lValue );
               return;
            }
            else
               sscanf( ptr+1, "%u", &iFError );
         }
         else
            hb_xfree( pData );
      }
   }
   hb_ret();
}

HB_FUNC( LETO_VARDECR )
{
   char *pData, *ptr;
   char cFlag1 = ' ';
   LONG lValue;
   USHORT uiFlags = (HB_ISNIL(3))? 0 : hb_parni(3);
   ULONG ulMemSize;

   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) && !HB_ISNIL(2) )
      {
         cFlag1 |= ( uiFlags & ( LETO_VCREAT | LETO_VOWN | LETO_VDENYWR | LETO_VDENYRD ) );
         if( uiFlags & LETO_VOWN )
            cFlag1 |= LETO_VOWN;

         ulMemSize = 24 + hb_parclen(1) + hb_parclen(2);
         pData = ( char * ) hb_xgrab( ulMemSize );
         hb_snprintf( pData, ulMemSize, "var;dec;%s;%s;2%c!;\r\n", hb_parc(1), hb_parc(2), cFlag1 );

         if( leto_DataSendRecv( pCurrentConn, pData, 0 ) )
         {
            hb_xfree( pData );
            ptr = leto_firstchar() - 1;
            if( *ptr == '+' )
            {
               sscanf( ptr+3, "%ld", &lValue );
               hb_retnl( lValue );
               return;
            }
            else
               sscanf( ptr+1, "%u", &iFError );
         }
         else
            hb_xfree( pData );
      }
   }
   hb_ret();
}

HB_FUNC( LETO_VARDEL )
{
   char *pData, *ptr;
   ULONG ulMemSize;

   if( pCurrentConn )
   {
      if( !HB_ISNIL(1) && !HB_ISNIL(2) )
      {
         ulMemSize = 16 + hb_parclen(1) + hb_parclen(2);
         pData = ( char * ) hb_xgrab( ulMemSize );
         hb_snprintf( pData, ulMemSize, "var;del;%s;%s;\r\n", hb_parc(1), hb_parc(2) );

         if( leto_DataSendRecv( pCurrentConn, pData, 0 ) )
         {
            hb_xfree( pData );
            ptr = leto_firstchar() - 1;
            if( *ptr == '+' )
            {
               hb_retl( 1 );
               return;
            }
            else
               sscanf( ptr+1, "%u", &iFError );
         }
         else
            hb_xfree( pData );
      }
   }
   hb_retl( 0 );
}

HB_FUNC( LETO_VARGETLIST )
{
   char *pData, *ptr;
   const char *pGroup = (HB_ISNIL(1))? NULL : hb_parc(1);
   char szData[24], cType;
   LONG lValue;
   USHORT uiItems = 0;
   USHORT uiMaxLen = (HB_ISNUM(2))? hb_parni(2) : 0;
   ULONG ulMemSize;

   if( pCurrentConn )
   {
      if( !pGroup || uiMaxLen > 999 )
         uiMaxLen = 0;
      ulMemSize = 32 + ( (pGroup)? hb_parclen(1) : 0 );
      pData = ( char * ) hb_xgrab( ulMemSize );
      hb_snprintf( pData, ulMemSize, "var;list;%s;;%u;\r\n", (pGroup)? pGroup : "", uiMaxLen );
      if( leto_DataSendRecv( pCurrentConn, pData, 0 ) )
      {
         PHB_ITEM temp;
         PHB_ITEM aInfo, aVar;

         hb_xfree( pData );
         ptr = pData = leto_firstchar();
         if( *(ptr-1) == '+' )
         {
            while( leto_getCmdItem( &ptr, szData ) )
            {
               ptr ++;
               uiItems ++;
            }
            if( pGroup && uiMaxLen )
               uiItems /= 3;
            aInfo = hb_itemArrayNew( uiItems );
            ptr = pData;
            uiItems = 1;
            while( leto_getCmdItem( &ptr, szData ) )
            {
               ptr ++;
               if( pGroup && uiMaxLen )
               {
                  aVar = hb_arrayGetItemPtr( aInfo, uiItems );
                  hb_arrayNew( aVar, 2 );

                  hb_itemPutC( hb_arrayGetItemPtr( aVar, 1 ), szData );

                  if( !leto_getCmdItem( &ptr, szData ) ) return; ptr ++;
                  cType = *szData;

                  if( !leto_getCmdItem( &ptr, szData ) ) return; ptr ++;
                  switch( cType )
                  {
                     case '1':
                     {
                        hb_itemPutL( hb_arrayGetItemPtr( aVar, 2 ), ( *szData == 1 ) );
                        break;
                     }
                     case '2':
                     {
                        sscanf( szData, "%ld", &lValue );
                        hb_itemPutNL( hb_arrayGetItemPtr( aVar, 2 ), lValue );
                        break;
                     }
                     case '3':
                     {
                        hb_itemPutC( hb_arrayGetItemPtr( aVar, 2 ), szData );
                        break;
                     }
                  }
               }
               else
               {
                  temp = hb_itemPutCL( NULL, szData, strlen(szData) );
                  hb_itemArrayPut( aInfo, uiItems, temp );
                  hb_itemRelease( temp );
               }
               uiItems ++;
            }
            hb_itemReturn( aInfo );
            hb_itemRelease( aInfo );
            return;
         }
         else
            sscanf( ptr, "%u", &iFError );
      }
      else
         hb_xfree( pData );
   }
   hb_ret();
}

HB_FUNC( LETO_GETLOCALIP )
{
   char szIP[24];

   *szIP = '\0';
   if( pCurrentConn )
   {
      hb_getLocalIP( pCurrentConn->hSocket, szIP );
   }
   hb_retc( szIP );
}

HB_FUNC( LETO_ADDCDPTRANSLATE )
{
   if( pCurrentConn && HB_ISCHAR(1) && HB_ISCHAR(2) )
   {
      PCDPSTRU pCdps;
      if( pCurrentConn->pCdpTable )
      {
         pCdps = pCurrentConn->pCdpTable;
         while( pCdps->pNext )
           pCdps = pCdps->pNext;
         pCdps = hb_xgrab( sizeof( CDPSTRU ) );
      }
      else
         pCdps = pCurrentConn->pCdpTable = hb_xgrab( sizeof( CDPSTRU ) );
      pCdps->szClientCdp = (char*) hb_xgrab( hb_parclen(1) + 1 );
      strcpy( pCdps->szClientCdp, hb_parc( 1 ) );
      pCdps->szServerCdp = (char*) hb_xgrab( hb_parclen(2) + 1 );
      strcpy( pCdps->szServerCdp, hb_parc( 2 ) );
      pCdps->pNext = NULL;
   }
}
