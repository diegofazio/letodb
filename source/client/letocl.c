/* $Id: $ */

/*
 * Harbour Project source code:
 * Harbour Leto client
 *
 * Copyright 2013 Alexander S. Kresin <alex / at / kresin.ru>
 * www - http://www.kresin.ru
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

#if defined (_MSC_VER)
  #define _WINSOCKAPI_
#endif
#if defined(HB_OS_UNIX)
   #include <netinet/in.h>
   #include <arpa/inet.h>
#endif

#if !defined (SUCCESS)
#define SUCCESS            0
#define FAILURE            1
#endif

#define LETO_TIMEOUT      -1

#include "hbdefs.h"
#include "funcleto.h"
#include "rddleto.ch"
#include "letocl.h"

static char * s_szBuffer = NULL;
static unsigned long s_lBufferLen = 0;
static int s_iConnectRes = 1;
static int s_iError = 0;

static char * s_szModName = NULL;
static char * s_szDateFormat = NULL;
static char * s_szCdp = NULL;
static char s_cCentury = 'T';

static void( *pFunc )( void ) = NULL;

unsigned int uiConnCount = 0;
LETOCONNECTION * letoConnPool = NULL;
LETOCONNECTION * pCurrentConn = NULL;

void leto_ConnectionClose( LETOCONNECTION * pConnection );

char * leto_getRcvBuff( void )
{
   return s_szBuffer;
}

char * leto_firstchar( void )
{
   int iLenLen;

   if( ( iLenLen = (((int)*s_szBuffer) & 0xFF) ) < 10 )
      return (s_szBuffer+2+iLenLen);
   else
      return (s_szBuffer+1);
}

void leto_setCallback( void( *fn )( void ) )
{
   pFunc = fn;
}

void LetoSetDateFormat( const char * szDateFormat )
{
   unsigned int uiLen;

   if( s_szDateFormat )
      free( s_szDateFormat );

   uiLen = strlen( szDateFormat );
   s_szDateFormat = (char*) malloc( uiLen+1 );
   memcpy( s_szDateFormat, szDateFormat, uiLen );
   s_szDateFormat[uiLen] = '\0';
}

void LetoSetCentury( char cCentury )
{
   s_cCentury = cCentury;
}

void LetoSetCdp( const char * szCdp )
{
   unsigned int uiLen;

   if( s_szCdp )
      free( s_szCdp );

   uiLen = strlen( szCdp );
   s_szCdp = (char*) malloc( uiLen+1 );
   memcpy( s_szCdp, szCdp, uiLen );
   s_szCdp[uiLen] = '\0';
}

void LetoSetModName( char * szModule )
{
   unsigned int uiLen;

   if( s_szModName )
      free( s_szModName );

   uiLen = strlen( szModule );
   s_szModName = (char*) malloc( uiLen+1 );
   memcpy( s_szModName, szModule, uiLen );
   s_szModName[uiLen] = '\0';
}

void LetoSetPath( LETOCONNECTION * pConnection, const char * szPath )
{
   if( pConnection->szPath )
      free( pConnection->szPath );
   pConnection->szPath = (char*) malloc( strlen( szPath ) + 1 );
   strcpy( pConnection->szPath, szPath );
}

int LetoGetConnectRes( void )
{
   return s_iConnectRes;
}

int LetoGetFError( void )
{
   return s_iError;
}

void LetoSetFError( int iErr )
{
   s_iError = iErr;
}

char * LetoGetServerVer( LETOCONNECTION * pConnection )
{
   return pConnection->szVersion;
}

int LetoGetCmdItem( char ** pptr, char * szDest )
{
   char * ptr = *pptr;

   if( ptr )
   {
      while( *ptr && *ptr != ';' ) ptr++;
      if( *ptr )
      {
         if( ptr > *pptr )
            memcpy( szDest, *pptr, ptr-*pptr );
         szDest[ptr-*pptr] = '\0';
         *pptr = ptr;
         return 1;
      }
      else
         return 0;
   }
   else
      return 0;
}

void LetoInit( void )
{
   if( !s_szBuffer )
   {
      hb_ipInit();

      letoConnPool = malloc( sizeof( LETOCONNECTION ) );
      uiConnCount = 1;
      memset( letoConnPool, 0, sizeof(LETOCONNECTION) * uiConnCount );

      s_szBuffer = (char*) malloc(HB_SENDRECV_BUFFER_SIZE);
      s_lBufferLen = HB_SENDRECV_BUFFER_SIZE;
   }
}

void LetoExit( unsigned int uiFull )
{
   unsigned int i;

   if( letoConnPool )
   {
      for( i=0; i<uiConnCount; i++ )
      {
         if( (letoConnPool + i)->pAddr )
            LetoConnectionClose( letoConnPool + i );
      }
      free( letoConnPool );
      letoConnPool = NULL;
   }
   uiConnCount = 0;

   if( uiFull )
   {
      if( s_szBuffer )
         free( s_szBuffer );
      s_szBuffer = NULL;
      s_lBufferLen = 0;
      hb_ipCleanup();
   }

}

long int leto_RecvFirst( LETOCONNECTION * pConnection )
{
   HB_SOCKET_T hSocket = pConnection->hSocket;
   int iRet;
   char szRet[LETO_MSGSIZE_LEN+1], * ptr;
   unsigned long int ulMsgLen;

   ptr = s_szBuffer;
   while( hb_ipDataReady( hSocket,2 ) == 0 )
   {
   }

   if( ( iRet = hb_ipRecv( hSocket, szRet, LETO_MSGSIZE_LEN ) ) < LETO_MSGSIZE_LEN )
      return 0;

   if( !strncmp( szRet, "Leto", 4 ) )
   {
      pConnection->uiProto = 1;
      pConnection->uiTBufOffset = 10;

      memcpy( s_szBuffer, szRet, 4 );
      ptr += 4;
      do
      {
         while( hb_ipDataReady( hSocket,2 ) == 0 )
         {
         }
         if( (iRet = hb_ipRecv( hSocket, ptr, HB_SENDRECV_BUFFER_SIZE-(ptr-s_szBuffer) )) <= 0 )
            break;
         else
         {
            ptr += iRet;
            if( *(ptr-1) == '\n' && *(ptr-2) == '\r' )
               break;
         }
      }
      while( ptr - s_szBuffer < HB_SENDRECV_BUFFER_SIZE );
   }
   else
   {
      pConnection->uiProto = 2;
      pConnection->uiTBufOffset = 12;

      ulMsgLen = HB_GET_LE_UINT32( szRet );
      if( !ulMsgLen || (ulMsgLen + 1 > s_lBufferLen) )
         return 0;
      do
      {
         while( hb_ipDataReady( hSocket, 2 ) == 0 )
         {
         }
         if( (iRet = hb_ipRecv( hSocket, ptr, ulMsgLen )) <= 0 )
            break;
         else
         {
            ptr += iRet;
            ulMsgLen -= iRet;
         }
      }
      while( ulMsgLen > 0 );
   }

   if( (((int)*s_szBuffer) & 0xFF) >= 10 && (ptr - s_szBuffer) > 2 && *(ptr-1) == '\n' && *(ptr-2) == '\r' )
      *(ptr-2) = '\0';
   else
      *ptr = '\0';

   return (long int) (ptr - s_szBuffer);
}

long int leto_Recv( LETOCONNECTION * pConnection )
{
   HB_SOCKET_T hSocket = pConnection->hSocket;
   char * ptr = s_szBuffer;
   int iRet;

   if( pConnection->uiProto == 1 )
   {
      int iLenLen = 0;
      char szRet[HB_SENDRECV_BUFFER_SIZE];
      long int lLen = 0;

      do
      {
         while( hb_ipDataReady( hSocket,2 ) == 0 )
         {
         }
         iRet = hb_ipRecv( hSocket, szRet, HB_SENDRECV_BUFFER_SIZE );
         if( iRet <= 0 )
            break;
         else
         {
            if( ((ULONG)((ptr - s_szBuffer) + iRet)) > s_lBufferLen )
            {
               char * szTemp;
               s_lBufferLen += HB_SENDRECV_BUFFER_SIZE;
               szTemp = (char*) malloc( s_lBufferLen );
               memcpy( szTemp, s_szBuffer, ptr-s_szBuffer );
               ptr = szTemp + (ptr-s_szBuffer);
               free( s_szBuffer );
               s_szBuffer = szTemp;
            }
            memcpy( ptr, szRet, iRet );
            ptr += iRet;
            if( lLen == 0 && ( iLenLen = (((int)*s_szBuffer) & 0xFF) ) < 10 && (ptr-s_szBuffer) > iLenLen )
               lLen = leto_b2n( s_szBuffer+1, iLenLen );
            if( ( lLen > 0 && lLen+iLenLen+1 <= ptr-s_szBuffer ) ||
                  ( iLenLen >= 10 && *(ptr-1) == '\n' && *(ptr-2) == '\r' ) )
               break;
         }
      }
      while(1);

      if( iLenLen >= 10 )
         *(ptr-2) = '\0';
      else
         *ptr = '\0';
   }
   else
   {
      int iStep = 0;
      char szRet[LETO_MSGSIZE_LEN+1];
      unsigned long int lMsgLen;

      while( hb_ipDataReady( hSocket, 2 ) == 0 )
      {
         if( pFunc && ( ( iStep ++ ) % 100 == 0 ) )
            pFunc();
      }
      iRet = hb_ipRecv( hSocket, szRet, LETO_MSGSIZE_LEN );
      if( iRet != LETO_MSGSIZE_LEN )
         return 0;

      lMsgLen = HB_GET_LE_UINT32( szRet );
      if( ! lMsgLen )
         return 0;

      if( lMsgLen + 1 > s_lBufferLen )
      {
         s_lBufferLen = lMsgLen + 1;
         s_szBuffer = (char*) realloc( s_szBuffer, s_lBufferLen );
      }

      do
      {
         while( hb_ipDataReady( hSocket, 2 ) == 0 )
         {
         }
         iRet = hb_ipRecv( hSocket, ptr, lMsgLen );
         if( iRet <= 0 )
         {
            //leto_writelog( NULL,0,"!ERROR! leto_Recv! broken message" );
            break;
         }
         else
         {
            ptr += iRet;
            lMsgLen -= iRet;
         }
      }
      while( lMsgLen > 0 );

      if( (((int)*s_szBuffer) & 0xFF) >= 10 && (ptr - s_szBuffer) > 2 && *(ptr-1) == '\n' && *(ptr-2) == '\r' )
         *(ptr-2) = '\0';
      else
         *ptr = '\0';
   }

   return (long int) (ptr - s_szBuffer);
}

long int leto_DataSendRecv( LETOCONNECTION * pConnection, const char * sData, ULONG ulLen )
{
   unsigned int uiSizeLen = (pConnection->uiProto==1)? 0 : LETO_MSGSIZE_LEN;

   if( !ulLen )
      ulLen = strlen(sData);

   if( uiSizeLen + ulLen > s_lBufferLen )
   {
      s_lBufferLen = uiSizeLen + ulLen;
      s_szBuffer = (char*) realloc( s_szBuffer, s_lBufferLen );
   }

   HB_PUT_LE_UINT32( s_szBuffer, ulLen );
   memcpy( s_szBuffer + uiSizeLen, sData, ulLen );
   if ( hb_ipSend( pConnection->hSocket, s_szBuffer, uiSizeLen + ulLen, LETO_TIMEOUT ) <= 0 )
   {
      return 0;
   }
   return leto_Recv( pConnection );
}

LETOCONNECTION * leto_getConnectionPool( void )
{
   return letoConnPool;
}

LETOCONNECTION * leto_ConnectionFind( const char * szAddr, int iPort )
{
   unsigned int i;

   for( i=0; i<uiConnCount; i++ )
   {
      if( letoConnPool[i].pAddr && letoConnPool[i].iPort == iPort &&
              !strcmp(letoConnPool[i].pAddr,szAddr) )
         return letoConnPool + i;
   }
   return NULL;
}

const char * leto_GetServerCdp( LETOCONNECTION * pConnection, const char *szCdp )
{
   PCDPSTRU pCdps = pConnection->pCdpTable;

   while( szCdp && pCdps )
   {
      if( !strcmp( szCdp, pCdps->szClientCdp ) )
      {
         szCdp = pCdps->szServerCdp;
         break;
      }
      pCdps = pCdps->pNext;
   }
   return szCdp;
}

int LetoCheckServerVer( LETOCONNECTION * pConnection, USHORT uiVer )
{
   return (int) ( pConnection->uiMajorVer*100 + pConnection->uiMinorVer ) >= uiVer;
}

const char * leto_RemoveIpFromPath(const char * szPath)
{
   const char * ptr;
   if( strlen(szPath) > 2 && szPath[0]=='/'  && szPath[1]=='/' && (ptr=strchr(szPath+2, '/')) != NULL )
   {
      szPath = ptr + 1;
   }
   return szPath;
}

int leto_getIpFromPath( const char * sSource, char * szAddr, int * piPort, char * szPath, BOOL bFile )
{
   const char * ptrPort, * ptr = sSource;
   char * ptrw;

   while( strlen(ptr) >= 2 && (ptr[0] != '/' || ptr[1] != '/'))
   {
      if( ( ptrPort = strchr( ptr, ',' ) ) == NULL)
         ptrPort = strchr( ptr, ';' );
      if( ptrPort )
         ptr = ptrPort + 1;
      else
         return 0;
   }
   ptr ++;
   if( ( ptrPort = strchr( ptr,':' ) ) == NULL )
   {
      return 0;
   }

   ptr ++;
   memcpy( szAddr, ptr, ptrPort-ptr );
   szAddr[ptrPort-ptr] = '\0';
   ptrPort ++;
   sscanf( ptrPort, "%d", piPort );

   if( szPath )
   {
      do ptrPort ++; while( *ptrPort>='0' && *ptrPort<='9' );
      ptr = ptrPort;
      ptrPort = sSource + strlen(sSource);
      if( bFile )
      {
         while( *ptrPort != '/' && *ptrPort != '\\' ) ptrPort --;
      }
      if( ptrPort < ptr )
      {
         return 0;
      }
      else if( ptrPort >= ptr )
      {
         ptrPort ++;
         if( *(ptr+2) == ':' )
            ptr ++;
         memcpy( szPath, ptr, ptrPort-ptr );
      }
      szPath[ptrPort-ptr] = '\0';
      if( ( ptrw = strchr( szPath, ',' ) ) != NULL || ( ptrw = strchr( szPath, ';' ) ) != NULL)
         ptrw[0] = '\0';
      ptr = szPath;
      while( ( ptr = strchr( ptr,'.' ) ) != NULL )
      {
         ptr ++;
         if( *ptr == '.' )
            return 0;
      }
   }
   return 1;
}

void leto_getFileFromPath( const char * sSource, char * szFile )
{
   const char * ptr = sSource + strlen(sSource) - 1;
   while( ptr >= sSource && *ptr != '/' && *ptr != '\\' ) ptr --;
   ptr ++;
   strcpy( szFile, ptr );
}

char * leto_DecryptBuf( LETOCONNECTION * pConnection, const char * ptr, ULONG * pulLen )
{
   if( *pulLen > pConnection->ulBufCryptLen )
   {
      if( !pConnection->ulBufCryptLen )
         pConnection->pBufCrypt = (char*) malloc( *pulLen );
      else
         pConnection->pBufCrypt = (char*) realloc( pConnection->pBufCrypt, *pulLen );
      pConnection->ulBufCryptLen = *pulLen;
   }
   leto_decrypt( ptr, *pulLen, pConnection->pBufCrypt, pulLen, LETO_PASSWORD );
   return pConnection->pBufCrypt;
}

char * leto_DecryptText( LETOCONNECTION * pConnection, ULONG * pulLen )
{
   char * ptr = leto_getRcvBuff();
   USHORT iLenLen = ((int)*ptr++) & 0xFF;

   if( !iLenLen || iLenLen >= 10 )
      *pulLen = 0;
   else
   {
     *pulLen = leto_b2n( ptr, iLenLen );
     ptr += iLenLen + 1;
     if( pConnection->bCrypt )
     {
        ptr = leto_DecryptBuf( pConnection, ptr, pulLen );
     }
   }
   return ptr;
}

static char * leto_ReadMemoInfo( PLETOTABLE pTable, char * ptr )
{
   char szTemp[14];

   if( LetoGetCmdItem( &ptr, szTemp ) )
   {
      ptr ++;
      strcpy( pTable->szMemoExt, szTemp );
      pTable->bMemoType = ( *ptr - '0' );
      ptr += 2;
      LetoGetCmdItem( &ptr, szTemp ); ptr ++;
      sscanf( szTemp, "%d" , (int*) &pTable->uiMemoVersion );
   }

   return ptr;
}

char * leto_NetName( void )
{
#if defined(HB_OS_UNIX) || ( defined(HB_OS_OS2) && defined(__GNUC__) )

   #define MAXGETHOSTNAME 256

   char szValue[MAXGETHOSTNAME + 1], *szRet;
   unsigned int uiLen;

   szValue[ 0 ] = '\0';
   gethostname( szValue, MAXGETHOSTNAME );

#elif defined(HB_OS_WIN_32) || defined( HB_OS_WIN )

   DWORD uiLen = MAX_COMPUTERNAME_LENGTH+1;
   char szValue[MAX_COMPUTERNAME_LENGTH+1], *szRet;

   szValue[ 0 ] = '\0';
   GetComputerName( szValue, &uiLen );

#else

   return NULL;

#endif

   uiLen = strlen( szValue );
   szRet = (char*) malloc( uiLen+1 );
   memcpy( szRet, szValue, uiLen );
   szRet[uiLen] = '\0';
   return szRet;

}

LETOCONNECTION * LetoConnectionNew( const char * szAddr, int iPort, const char * szUser, const char * szPass, int iTimeOut )
{
   HB_SOCKET_T hSocket;
   char szData[300];
   char szBuf[36];
   char szKey[LETO_MAX_KEYLENGTH+1];
   unsigned int i;
   unsigned long int ulLen; 

   for( i=0; i<uiConnCount; i++ )
   {
      if( !letoConnPool[i].pAddr )
         break;
   }
   if( i == uiConnCount )
   {
      USHORT uiCurrentConn = 0;
      if( pCurrentConn )
         uiCurrentConn = pCurrentConn - letoConnPool;
      letoConnPool = realloc( letoConnPool, sizeof( LETOCONNECTION ) * (++uiConnCount) );
      memset( letoConnPool+i, 0, sizeof( LETOCONNECTION ) );
      if( pCurrentConn )
         pCurrentConn = letoConnPool + uiCurrentConn;
   }
   if( iTimeOut == 0 )
      iTimeOut = 5000;
   hSocket = hb_ipConnect( szAddr, htons( iPort ), iTimeOut );
   if( !hb_iperrorcode() )    
   {
      LETOCONNECTION * pConnection = letoConnPool + i;

      pConnection->hSocket = hSocket;
      pConnection->iPort = iPort;
      pConnection->iTimeOut = iTimeOut;
      pConnection->pAddr = (char*) malloc( strlen(szAddr)+1 );
      memcpy( pConnection->pAddr, szAddr, strlen(szAddr) );
      pConnection->pAddr[strlen(szAddr)] = '\0';

      pConnection->bTransActive = FALSE;
      pConnection->szTransBuffer = NULL;
      pConnection->ulTransBuffLen = 0;
      pConnection->ulTransDataLen = 0;
      pConnection->ulRecsInTrans = 0;
      pConnection->ulTransBlockLen = 0;

      pConnection->bRefreshCount = TRUE;

      pConnection->pBufCrypt = NULL;
      pConnection->ulBufCryptLen = 0;

      pConnection->uiBufRefreshTime = 100;

      if( leto_RecvFirst( pConnection ) )
      {
         char * ptr, * pName;
         unsigned int uiSizeLen = (pConnection->uiProto==1)? 0 : LETO_MSGSIZE_LEN;

         ptr = strchr( s_szBuffer,';' );
         if( ptr )
         {
            memcpy( pConnection->szVersion, s_szBuffer, ptr-s_szBuffer );
            pConnection->szVersion[ptr-s_szBuffer] = '\0';
            ptr ++;
            if( *ptr++ == 'Y' )
               pConnection->bCrypt = 1;
            else
               pConnection->bCrypt = 0;
            if( *ptr == ';' )
               pConnection->cDopcode[0] = '\0';
            else
            {
               pConnection->cDopcode[0] = *ptr++;
               pConnection->cDopcode[1] = *ptr;
            }
         }
         else
         {
            memcpy( pConnection->szVersion, s_szBuffer, strlen(s_szBuffer) );
            pConnection->szVersion[strlen(s_szBuffer)] = '\0';
            pConnection->cDopcode[0] = '\0';
         }
         if( ( ptr = strstr( pConnection->szVersion, "v." ) ) != NULL )
            sscanf( ptr+2, "%d.%d", &pConnection->uiMajorVer, &pConnection->uiMinorVer);
         else
            pConnection->uiMajorVer = pConnection->uiMinorVer = 0;

         pName = leto_NetName();
         ptr = szData + uiSizeLen;
         sprintf( ptr, "intro;%s;%s;%s;", 
                        HB_LETO_VERSION_STRING, ( (pName) ? pName : "" ),
                        ( (s_szModName)? s_szModName : "client" ) );
         ptr += strlen( ptr );
         if( pConnection->cDopcode[0] && szUser )
         {
            if( ( ulLen = strlen(szUser) ) > LETO_MAX_USERNAME )
               ulLen = LETO_MAX_USERNAME;
            memcpy( ptr, szUser, ulLen );
            ptr += ulLen;
         }
         *ptr++ = ';';
         if( pConnection->cDopcode[0] && szPass )
         {
            unsigned int uiKeyLen = strlen(LETO_PASSWORD);

            if( uiKeyLen > LETO_MAX_KEYLENGTH )
               uiKeyLen = LETO_MAX_KEYLENGTH;
            if( ( ulLen = strlen(szPass) ) > LETO_MAX_USERNAME )
               ulLen = LETO_MAX_USERNAME;

            memcpy( szKey, LETO_PASSWORD, uiKeyLen );
            szKey[uiKeyLen] = '\0';
            szKey[0] = pConnection->cDopcode[0];
            szKey[1] = pConnection->cDopcode[1];
            leto_encrypt( szPass, ulLen, szBuf, &ulLen, szKey );
            leto_byte2hexchar( szBuf, (int)ulLen, ptr );
            ptr += ulLen * 2;
         }
         *ptr++ = ';';
         sprintf( ptr, "%s;%s;%c\r\n", leto_GetServerCdp( pConnection, ( (s_szCdp)? s_szCdp : "EN" ) ),
                  ( (s_szDateFormat)? s_szDateFormat : "dd/mm/yy" ), s_cCentury );
         if( pName )
            free( pName );
         if( LetoCheckServerVer( pConnection, 100 ) )
         {
            ulLen = strlen(szData+LETO_MSGSIZE_LEN);
            HB_PUT_LE_UINT32( szData, ulLen );
            if ( hb_ipSend( hSocket, szData, LETO_MSGSIZE_LEN+ulLen, LETO_TIMEOUT ) <= 0 )
            {
               s_iConnectRes = LETO_ERR_SEND;
               return NULL;
            }
         }
         else
         {
            if ( hb_ipSend( hSocket, szData, strlen(szData), -1 ) <= 0 )
            {
               s_iConnectRes = LETO_ERR_SEND;
               return NULL;
            }
         }
         if( !leto_Recv( pConnection ) )
         {
            if( pConnection->cDopcode[0] )
               s_iConnectRes = LETO_ERR_LOGIN;
            else
               s_iConnectRes = LETO_ERR_RECV;
            return NULL;
         }
         ptr = leto_firstchar();
         if( !strncmp( ptr, "ACC", 3 ) )
            s_iConnectRes = LETO_ERR_ACCESS;
         else
         {
            pConnection->szVerHarbour[0] = '\0';
            if( ( pName = strchr(ptr, ';') ) != NULL && (pName - ptr) >= 3 )
            {
               memcpy( pConnection->szAccess, ptr, (pName-ptr>3)? 3 : pName-ptr );
               if( ( pName = strchr(pName+1, ';') ) != NULL )
               {
                  memcpy( pConnection->szVerHarbour, ptr, pName-ptr );
                  pConnection->szVerHarbour[pName-ptr] = '\0';
                  if( LetoCheckServerVer( pConnection, 211 ) && *( ptr = pName + 1 ) != '\0' )
                  {
                     int iDriver, iMemoType;
                     sscanf(ptr, "%d;%d", &iDriver, &iMemoType );
                     pConnection->uiDriver = iDriver;
                     pConnection->uiMemoType = iMemoType;
                  }
               }
            }
            s_iConnectRes = 0;
         }
         return pConnection;
      }        
   }
   else
   {
      s_iConnectRes = LETO_ERR_SOCKET;
   }
   return NULL;
}

int LetoCloseAll( LETOCONNECTION * pConnection )
{
   char szData[16];

   if( LetoCheckServerVer( pConnection, 100 ) )
      sprintf( szData,"close_all;\r\n" );
   else
      sprintf( szData,"close;00;\r\n" );

   if( leto_DataSendRecv( pConnection, szData, 0 ) )
      return 1;
   else
      return 0;
}

void LetoConnectionClose( LETOCONNECTION * pConnection )
{
   hb_ipclose( pConnection->hSocket );
   pConnection->hSocket = 0;
   free( pConnection->pAddr );
   pConnection->pAddr = NULL;
   if( pConnection->szPath )
      free( pConnection->szPath );
   pConnection->szPath = NULL;
   if( pConnection->pCdpTable )
   {
      PCDPSTRU pNext = pConnection->pCdpTable, pCdps;
      while( pNext )
      {
         pCdps = pNext;
         free( pCdps->szClientCdp );
         free( pCdps->szServerCdp );
         pNext = pCdps->pNext;
         free( pCdps );
      }
      pConnection->pCdpTable = NULL;
   }

   if( pConnection->szTransBuffer )
   {
      free( pConnection->szTransBuffer );
      pConnection->szTransBuffer = NULL;
   }

   if( pConnection->pBufCrypt )
   {
      free( pConnection->pBufCrypt );
      pConnection->pBufCrypt = NULL;
   }
   pConnection->ulBufCryptLen = 0;

}

PLETOTABLE LetoDbOpen( LETOCONNECTION * pConnection, char * szFile, char * szAlias, int iShared, int iReadOnly, char * szCdp, unsigned int uiArea )
{

   char szData[_POSIX_PATH_MAX + 16], * ptr, * ptrStart, szTemp[14];
   PLETOTABLE pTable;
   unsigned int uiFields, uiCount, uiLen;

   if( LetoCheckServerVer( pConnection, 100 ) )
      sprintf( szData,"open;%s;%s;%c%c;%s;\r\n", szFile,
          szAlias, (iShared)? 'T':'F', (iReadOnly)? 'T':'F', szCdp );
   else
      sprintf( szData,"open;01;%d;%s;%s;%c%c;%s;\r\n", uiArea, szFile,
          szAlias, (iShared)? 'T':'F', (iReadOnly)? 'T':'F', szCdp );

   if( !leto_DataSendRecv( pConnection, szData, 0 ) )
   {
      s_iError = 1000;
      return NULL;
   }
   ptr = leto_getRcvBuff();
   if( *ptr == '-' )
   {
      if( *(ptr+3) == '4' )
         s_iError = 103;
      else
         s_iError = 1021;
      return NULL;
   }

   pTable = (PLETOTABLE) malloc( sizeof(LETOTABLE) );
   ptr = leto_firstchar();

   LetoGetCmdItem( &ptr, szTemp ); ptr ++;
   sscanf( szTemp, "%lu" , &(pTable->hTable) );
   if( *ptr == '1' )
     pTable->uiDriver = LETO_NTX;
   ptr ++; ptr++;

   if( LetoCheckServerVer( pConnection, 100 ) )
   {
      // Read MEMOEXT, MEMOTYPE, MEMOVERSION
      ptr = leto_ReadMemoInfo( pTable, ptr );
      // for DBI_LASTUPDATE
      if( LetoCheckServerVer( pConnection, 208 ) )
      {
         LetoGetCmdItem( &ptr, szTemp ); ptr ++;
         //pArea->lLastUpdate = hb_dateEncStr( szTemp );
      }
   }
   // Read number of fields
   LetoGetCmdItem( &ptr, szTemp ); ptr ++;
   sscanf( szTemp, "%d" , &uiFields );
   pTable->uiFieldExtent = uiFields;
   ptrStart = ptr;
   for( uiCount = 1; uiCount <= uiFields; uiCount++ )
   {
      while( *ptr != ';' ) ptr++;
      while( *ptr != ';' ) ptr++;
      sscanf( ptr, "%d;" , &uiLen );
      while( *ptr != ';' ) ptr++;
      while( *ptr != ';' ) ptr++;
   }
   pTable->szFields = (char*) malloc( ptr - ptrStart );
   memcpy( pTable->szFields, ptrStart, ptr - ptrStart );

   return pTable;
}

char * LetoMgGetInfo( LETOCONNECTION * pConnection )
{
   if( leto_DataSendRecv( pConnection, "mgmt;00;\r\n", 0 ) )
   {
      return leto_firstchar();
   }
   else
      return NULL;
}

char * LetoMgGetUsers( LETOCONNECTION * pConnection, const char * szTable )
{
   char szData[_POSIX_PATH_MAX + 1];

   if( szTable )
      sprintf( szData, "mgmt;01;%s;\r\n", szTable );
   else
      sprintf( szData, "mgmt;01;\r\n" );

   if( leto_DataSendRecv( pConnection, szData, 0 ) )
   {
      return leto_firstchar();
   }
   else
      return NULL;
}

char * LetoMgGetTables( LETOCONNECTION * pConnection, const char * szUser )
{
   char szData[64];

   if( szUser )
      sprintf( szData, "mgmt;02;%s;\r\n", szUser );
   else
      sprintf( szData, "mgmt;02;\r\n" );

   if( leto_DataSendRecv( pConnection, szData, 0 ) )
   {
      return leto_firstchar();
   }
   else
      return NULL;
}

void LetoMgKillUser( LETOCONNECTION * pConnection, const char * szUserId )
{
   char szData[32];

   sprintf( szData, "mgmt;09;%s;\r\n", szUserId );
   leto_DataSendRecv( pConnection, szData, 0 );

}

char * LetoMgGetTime( LETOCONNECTION * pConnection )
{
   if( leto_DataSendRecv( pConnection, "mgmt;03;\r\n", 0 ) )
   {
      return leto_firstchar();
   }
   else
      return NULL;
}

int LetoVarSet( LETOCONNECTION * pConnection, char * szGroup, char * szVar, char cType, char * szValue, unsigned int uiFlags, char ** pRetValue )
{
   unsigned long ul, ulLen = 24 + strlen(szGroup) + strlen(szVar);
   char * pData, * ptr, cFlag1 = ' ', cFlag2 = ' ';
   unsigned int uiRes;

   s_iError = -1;
   ptr = szGroup;
   while( *ptr )
      if( *ptr++ == ';' )
         return 0;
   ptr = szVar;
   while( *ptr )
      if( *ptr++ == ';' )
         return 0;

   if( cType == '1' || cType == '2' || cType == '3' )
      ulLen += strlen( szValue );
   else
      return 0;

   cFlag1 |= ( uiFlags & ( LETO_VCREAT | LETO_VOWN | LETO_VDENYWR | LETO_VDENYRD ) );
   if( pRetValue )
      cFlag2 |= LETO_VPREVIOUS;

   pData = ( char * ) malloc( ulLen );
   memcpy( pData, "var;set;", 8 );
   ptr = pData + 8;
   memcpy( ptr, szGroup, ulLen = strlen( szGroup ) );
   ptr += ulLen;
   *ptr++ = ';';
   memcpy( ptr, szVar, ulLen = strlen( szVar ) );
   ptr += ulLen;
   *ptr++ = ';';
   memcpy( ptr, szValue, ulLen = strlen( szValue ) );
   if( cType == '3' )
   {
      for( ul=0; ul < ulLen; ul++, ptr++ )
         if( *ptr == ';' )
            *ptr = '\1';
   }
   else
      ptr += ulLen;
   *ptr++ = ';';
   *ptr++ = cType;
   *ptr++ = cFlag1;
   *ptr++ = cFlag2;
   *ptr++ = ';'; *ptr++ = '\r'; *ptr++ = '\n';
   *ptr = '\0';

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar() - 1;
      uiRes = ( *ptr == '+' );
      ptr++;
   }
   else
      return 0;

   if( uiRes )
   {
      s_iError = 0;
      if( pRetValue )
      {
         pData = ptr;
         ptr += 2;
         while( *ptr != ';' )
         {
            if( *ptr == '\1' )
               *ptr = ';';
            ptr ++;
         }
         *pRetValue = ( char * ) malloc( ulLen = (ptr - pData) );
         memcpy( *pRetValue, pData, ulLen );
         *(*pRetValue + ulLen) = '\0';
      }
   }
   else
      sscanf( ptr+1, "%u", &s_iError );

   return uiRes;
}

char * LetoVarGet( LETOCONNECTION * pConnection, char * szGroup, char * szVar )
{
   unsigned long ulLen = 24 + strlen(szGroup) + strlen(szVar);
   char * pData, * ptr;
   unsigned int uiRes;

   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "var;get;%s;%s;\r\n", szGroup, szVar );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar() - 1;
      uiRes = ( *ptr == '+' );
      ptr++;
   }
   else
      return NULL;

   if( uiRes )
   {
      char * szRetValue = ( char * ) malloc( (ulLen = strlen(ptr)) + 1 );
      memcpy( szRetValue, ptr, ulLen );
      *(szRetValue + ulLen) = '\0';
      ptr = szRetValue + 2;
      while( * ptr && *ptr != ';' )
      {
         if( *ptr == '\1' )
            *ptr = ';';
         ptr ++;
      }
      *ptr = '\0';
      s_iError = 0;
      return szRetValue;
   }

   sscanf( ptr, "%u", &s_iError );
   return NULL;

}

long LetoVarIncr( LETOCONNECTION * pConnection, char * szGroup, char * szVar, unsigned int uiFlags )
{
   unsigned long ulLen = 24 + strlen(szGroup) + strlen(szVar);
   char *pData, *ptr, cFlag1 = ' ';
   long lValue;
   unsigned int uiRes;

   s_iError = -1;

   cFlag1 |= ( uiFlags & ( LETO_VCREAT | LETO_VOWN | LETO_VDENYWR | LETO_VDENYRD ) );
   if( uiFlags & LETO_VOWN )
      cFlag1 |= LETO_VOWN;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "var;inc;%s;%s;2%c!;\r\n", szGroup, szVar, cFlag1 );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar() - 1;
      uiRes = ( *ptr == '+' );
      ptr++;
   }
   else
      return 0;
   if( uiRes )
   {
      sscanf( ptr+2, "%ld", &lValue );
      s_iError = 0;
      return lValue;
   }

   sscanf( ptr, "%u", &s_iError );
   return 0;
}

long LetoVarDecr( LETOCONNECTION * pConnection, char * szGroup, char * szVar, unsigned int uiFlags )
{
   unsigned long ulLen = 24 + strlen(szGroup) + strlen(szVar);
   char *pData, *ptr, cFlag1 = ' ';
   long lValue;
   unsigned int uiRes;

   s_iError = -1;

   cFlag1 |= ( uiFlags & ( LETO_VCREAT | LETO_VOWN | LETO_VDENYWR | LETO_VDENYRD ) );
   if( uiFlags & LETO_VOWN )
      cFlag1 |= LETO_VOWN;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "var;dec;%s;%s;2%c!;\r\n", szGroup, szVar, cFlag1 );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar() - 1;
      uiRes = ( *ptr == '+' );
      ptr++;
   }
   else
      return 0;
   if( uiRes )
   {
      sscanf( ptr+2, "%ld", &lValue );
      s_iError = 0;
      return lValue;
   }

   sscanf( ptr, "%u", &s_iError );
   return 0;
}

int LetoVarDel( LETOCONNECTION * pConnection, char * szGroup, char * szVar )
{
   unsigned long ulLen = 24 + strlen(szGroup) + strlen(szVar);
   char * pData, * ptr;
   unsigned int uiRes;

   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "var;del;%s;%s;\r\n", szGroup, szVar );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar() - 1;
      uiRes = ( *ptr == '+' );
      ptr++;
   }
   else
      return 0;

   if( uiRes )
   {
      s_iError = 0;
      return 1;
   }

   sscanf( ptr, "%u", &s_iError );
   return 0;

}

char * LetoVarGetList( LETOCONNECTION * pConnection, const char * szGroup, unsigned int uiMaxLen )
{
   unsigned long ulLen = 32 + ( (szGroup)? strlen(szGroup) : 0 );
   char * pData, * ptr;
   unsigned int uiRes;

   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "var;list;%s;;%u;\r\n", (szGroup)? szGroup : "", uiMaxLen );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar() - 1;
      uiRes = ( *ptr == '+' );
      ptr++;
   }
   else
      return NULL;

   if( uiRes )
   {
      char * szRetValue = ( char * ) malloc( (ulLen = strlen(ptr)) + 1 );
      memcpy( szRetValue, ptr, ulLen );
      *(szRetValue + ulLen) = '\0';
      s_iError = 0;
      return szRetValue;
   }

   sscanf( ptr, "%u", &s_iError );
   return NULL;

}

int LetoIsFileExist( LETOCONNECTION * pConnection, char * szFile )
{
   unsigned long ulLen = 24 + strlen(szFile);
   char * pData, * ptr;
   unsigned int uiRes;

   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "file;01;%s;\r\n", szFile );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' && *ptr=='T' )
      {
         s_iError = 0;
         return 1;
      }
   }

   return 0;

}

int LetoFileErase( LETOCONNECTION * pConnection, char * szFile )
{
   unsigned long ulLen = 24 + strlen(szFile);
   char * pData, * ptr;
   unsigned int uiRes;

   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "file;02;%s;\r\n", szFile );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' && (*ptr=='T') )
      {
         s_iError = 0;
         return 1;
      }
      sscanf( ptr+2, "%u", &s_iError );
   }

   return 0;

}

int LetoFileRename( LETOCONNECTION * pConnection, char * szFile, char * szFileNew )
{
   unsigned long ulLen = 24 + strlen(szFile) + strlen(szFileNew);
   char * pData, * ptr;
   unsigned int uiRes;

   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "file;03;%s;%s;\r\n", szFile, szFileNew );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' && (*ptr=='T') )
      {
         s_iError = 0;
         return 1;
      }
      sscanf( ptr+2, "%u", &s_iError );
   }

   return 0;

}

char * LetoMemoRead( LETOCONNECTION * pConnection, char * szFile )
{
   unsigned long ulLen = 24 + strlen(szFile);
   char * pData, * ptr;
   unsigned long ulRes;

   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "file;04;%s;\r\n", szFile );

   ulRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( ulRes )
   {
      ptr = leto_DecryptText( pConnection, &ulLen );

      if( ulLen )
      {
         pData = ( char * ) malloc( ulLen );
         memcpy( pData, ptr, ulLen - 1 );
         *( pData+ulLen ) = '\0';
         s_iError = 0;
         return pData;
      }
   }

   return NULL;

}

int LetoMemoWrite( LETOCONNECTION * pConnection, char * szFile, char * szValue, unsigned long ulLen )
{
   char * pData, * ptr;
   unsigned long ulRes;

   s_iError = -1;

   pData = ( char * ) malloc( 32 + strlen(szFile) + ulLen );
   sprintf( pData, "file;13;%s;;;", szFile );

   ulRes = strlen( pData );

   HB_PUT_LE_UINT32( pData+ulRes, ulLen );  ulRes += 4;
   memcpy( pData+ulRes, szValue, ulLen );
   ulRes += ulLen;

   pData[ulRes  ] = '\r';
   pData[ulRes+1] = '\n';
   pData[ulRes+2] = '0';
   ulRes += 2;

   ulRes = leto_DataSendRecv( pConnection, pData, ulRes );
   free( pData );
   if( ulRes )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' )
      {
         if( *ptr == 'T' )
         {
            s_iError = 0;
            return 1;
         }
         else
         {
            ptr += 2;
            sscanf( ptr, "%d;", &s_iError );
         }
      }
   }

   return 0;

}

char * LetoFileRead( LETOCONNECTION * pConnection, char * szFile, unsigned long ulStart, unsigned long * ulLen )
{
   char * pData, * ptr;
   unsigned long ulRes;

   s_iError = 0;

   pData = ( char * ) malloc( 64 + strlen(szFile) );
   sprintf( pData, "file;10;%s;%lu;%lu;\r\n", szFile, ulStart, *ulLen );

   ulRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if ( ulRes >= 5 )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' )
      {
         if( *ptr == 'T' )
         {
            ptr += 2;
            *ulLen = HB_GET_LE_UINT32( ptr );
            ptr += 4;

            if( *ulLen < ulRes )
            {
               pData = ( char * ) malloc( *ulLen );
               memcpy( pData, ptr, *ulLen );
               *( pData+*ulLen ) = '\0';
               return pData;
            }
            else
               s_iError = -2;
         }
         else
            sscanf( ptr+2, "%d;", &s_iError );
      }
   }
   else
      s_iError = -1;

   *ulLen = 0;
   return NULL;

}

int LetoFileWrite( LETOCONNECTION * pConnection, char * szFile, char * szValue, unsigned long ulStart, unsigned long ulLen )
{
   char * pData, * ptr;
   unsigned long ulRes;

   s_iError = 0;

   pData = ( char * ) malloc( 48 + strlen(szFile) + ulLen );
   sprintf( pData, "file;14;%s;%lu;%lu;", szFile, ulStart, ulLen );

   ulRes = strlen( pData );

   HB_PUT_LE_UINT32( pData+ulRes, ulLen );  ulRes += 4;
   memcpy( pData+ulRes, szValue, ulLen );
   ulRes += ulLen;

   pData[ulRes  ] = '\r';
   pData[ulRes+1] = '\n';
   pData[ulRes+2] = '0';
   ulRes += 2;

   ulRes = leto_DataSendRecv( pConnection, pData, ulRes );
   free( pData );
   if( ulRes )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' )
      {
         if( *ptr == 'T' )
            return 1;
         else
         {
            ptr += 2;
            sscanf( ptr, "%d;", &s_iError );
         }
      }
      else
         s_iError = -2;
   }
   else
      s_iError = -1;

   return 0;

}

long int LetoFileSize( LETOCONNECTION * pConnection, char * szFile )
{
   unsigned long ulLen = 24 + strlen(szFile);
   char * pData, * ptr;
   unsigned long ulRes;
  
   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "file;11;%s;\r\n", szFile );

   ulRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( ulRes )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' )
      {
         if( *ptr == 'T' )
         {
            sscanf( ptr+2, "%lu;", &ulLen );
            s_iError = 0;
            return ulLen;
         }
         else
            sscanf( ptr+2, "%d;", &s_iError );
      }
   }

   return -1;

}

char * LetoDirectory( LETOCONNECTION * pConnection, char * szDir, char * szAttr )
{
   unsigned long ulLen = 48 + strlen(szDir);
   char * pData, * ptr;
   unsigned long ulRes;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "file;12;%s;%s;\r\n", szDir, ((szAttr)? szAttr : "") );

   s_iError = -1;
   ulRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( ulRes )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' )
      {
         ptr += 2;
         ulLen = strlen( ptr );
         pData = ( char * ) malloc( ulLen + 1 );
         memcpy( pData, ptr, ulLen );
         *( pData+ulLen ) = '\0';
         s_iError = 0;
         return pData;
      }
      else
      {
         sscanf( ptr+2, "%d;", &s_iError );
         return NULL;
      }

   }

   return NULL;

}

int LetoMakeDir( LETOCONNECTION * pConnection, char * szFile )
{
   unsigned long ulLen = 24 + strlen(szFile);
   char * pData, * ptr;
   unsigned int uiRes;

   s_iError = -1;

   pData = ( char * ) malloc( ulLen );
   sprintf( pData, "file;05;%s;\r\n", szFile );

   uiRes = leto_DataSendRecv( pConnection, pData, 0 );
   free( pData );
   if( uiRes )
   {
      ptr = leto_firstchar();
      if( *(ptr-1) == '+' && *ptr=='T' )
      {
         s_iError = 0;
         return 1;
      }
      sscanf( ptr+2, "%u", &s_iError );
   }

   return 0;

}