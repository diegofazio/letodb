/*
 * Harbour 3.2.0dev (r1909261630)
 * Microsoft Visual C++ 19.22.27905 (32-bit)
 * Generated C source from "source\common\common.prg"
 */

#include "hbvmpub.h"
#include "hbinit.h"


HB_FUNC( RDINI );
HB_FUNC_EXTERN( SPACE );
HB_FUNC_EXTERN( FOPEN );
HB_FUNC_EXTERN( LEN );
HB_FUNC_STATIC( RDSTR );
HB_FUNC_EXTERN( RIGHT );
HB_FUNC_EXTERN( LEFT );
HB_FUNC_EXTERN( EMPTY );
HB_FUNC_EXTERN( UPPER );
HB_FUNC_EXTERN( SUBSTR );
HB_FUNC_EXTERN( AT );
HB_FUNC_EXTERN( AADD );
HB_FUNC_EXTERN( RTRIM );
HB_FUNC_EXTERN( ALLTRIM );
HB_FUNC_EXTERN( FCLOSE );
HB_FUNC_EXTERN( FREAD );


HB_INIT_SYMBOLS_BEGIN( hb_vm_SymbolInit_COMMON )
{ "RDINI", {HB_FS_PUBLIC | HB_FS_FIRST | HB_FS_LOCAL}, {HB_FUNCNAME( RDINI )}, NULL },
{ "SPACE", {HB_FS_PUBLIC}, {HB_FUNCNAME( SPACE )}, NULL },
{ "FOPEN", {HB_FS_PUBLIC}, {HB_FUNCNAME( FOPEN )}, NULL },
{ "LEN", {HB_FS_PUBLIC}, {HB_FUNCNAME( LEN )}, NULL },
{ "RDSTR", {HB_FS_STATIC | HB_FS_LOCAL}, {HB_FUNCNAME( RDSTR )}, NULL },
{ "RIGHT", {HB_FS_PUBLIC}, {HB_FUNCNAME( RIGHT )}, NULL },
{ "LEFT", {HB_FS_PUBLIC}, {HB_FUNCNAME( LEFT )}, NULL },
{ "EMPTY", {HB_FS_PUBLIC}, {HB_FUNCNAME( EMPTY )}, NULL },
{ "UPPER", {HB_FS_PUBLIC}, {HB_FUNCNAME( UPPER )}, NULL },
{ "SUBSTR", {HB_FS_PUBLIC}, {HB_FUNCNAME( SUBSTR )}, NULL },
{ "AT", {HB_FS_PUBLIC}, {HB_FUNCNAME( AT )}, NULL },
{ "AADD", {HB_FS_PUBLIC}, {HB_FUNCNAME( AADD )}, NULL },
{ "RTRIM", {HB_FS_PUBLIC}, {HB_FUNCNAME( RTRIM )}, NULL },
{ "ALLTRIM", {HB_FS_PUBLIC}, {HB_FUNCNAME( ALLTRIM )}, NULL },
{ "FCLOSE", {HB_FS_PUBLIC}, {HB_FUNCNAME( FCLOSE )}, NULL },
{ "FREAD", {HB_FS_PUBLIC}, {HB_FUNCNAME( FREAD )}, NULL }
HB_INIT_SYMBOLS_EX_END( hb_vm_SymbolInit_COMMON, "source\\common\\common.prg", 0x0, 0x0003 )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup hb_vm_SymbolInit_COMMON
#elif defined( HB_DATASEG_STARTUP )
   #define HB_DATASEG_BODY    HB_DATASEG_FUNC( hb_vm_SymbolInit_COMMON )
   #include "hbiniseg.h"
#endif

HB_FUNC( RDINI )
{
	static const HB_BYTE pcode[] =
	{
		13,8,1,36,56,0,176,1,0,93,0,4,12,1,
		80,8,93,1,4,80,9,36,58,0,176,2,0,95,
		1,92,64,12,2,165,80,2,92,255,69,29,108,1,
		36,59,0,4,0,0,80,7,36,60,0,106,1,0,
		80,4,36,62,0,176,3,0,176,4,0,95,2,96,
		8,0,96,9,0,93,0,4,12,4,165,80,3,12,
		1,121,5,32,50,1,36,65,0,176,5,0,95,3,
		122,12,2,106,2,38,0,8,28,27,36,66,0,96,
		4,0,176,6,0,95,3,176,3,0,95,3,12,1,
		122,49,12,2,135,25,183,36,69,0,176,7,0,95,
		4,12,1,31,12,36,70,0,95,4,95,3,72,80,
		3,36,72,0,106,1,0,80,4,36,75,0,176,6,
		0,95,3,122,12,2,106,2,91,0,8,28,56,36,
		76,0,176,8,0,176,9,0,95,3,92,2,176,10,
		0,106,2,93,0,95,3,12,2,92,2,49,12,3,
		12,1,80,3,36,77,0,176,11,0,95,7,95,3,
		4,0,0,4,2,0,20,2,26,82,255,36,78,0,
		176,6,0,95,3,122,12,2,106,2,59,0,69,29,
		63,255,36,79,0,176,10,0,106,2,61,0,95,3,
		12,2,80,5,36,80,0,95,5,121,69,29,37,255,
		36,81,0,176,7,0,95,7,12,1,28,25,36,82,
		0,176,11,0,95,7,106,5,77,65,73,78,0,4,
		0,0,4,2,0,20,2,36,84,0,176,12,0,176,
		6,0,95,3,95,5,122,49,12,2,12,1,80,6,
		36,85,0,176,13,0,176,9,0,95,3,95,5,122,
		72,12,2,12,1,80,3,36,86,0,176,11,0,95,
		7,176,3,0,95,7,12,1,1,92,2,1,176,8,
		0,95,6,12,1,95,3,4,2,0,20,2,26,180,
		254,36,90,0,176,14,0,95,2,20,1,36,93,0,
		95,7,110,7
	};

	hb_vmExecute( pcode, symbols );
}

HB_FUNC_STATIC( RDSTR )
{
	static const HB_BYTE pcode[] =
	{
		13,4,4,36,96,0,106,1,0,80,5,36,97,0,
		95,3,80,7,36,98,0,176,10,0,106,2,10,0,
		176,9,0,95,2,95,3,12,2,12,2,80,3,36,
		99,0,95,3,121,5,29,192,0,36,100,0,95,1,
		100,69,28,122,36,101,0,96,5,0,176,9,0,95,
		2,95,7,12,2,135,36,102,0,176,15,0,95,1,
		96,2,0,95,4,12,3,80,6,36,103,0,95,6,
		121,5,28,10,36,104,0,106,1,0,110,7,36,105,
		0,95,6,95,4,35,28,27,36,106,0,176,9,0,
		95,2,122,95,6,12,3,106,2,10,0,72,106,2,
		13,0,72,80,2,36,108,0,176,10,0,106,2,10,
		0,95,2,12,2,80,3,36,109,0,96,5,0,176,
		9,0,95,2,122,95,3,12,3,135,25,91,36,111,
		0,96,5,0,176,12,0,176,9,0,95,2,95,7,
		12,2,12,1,135,36,112,0,95,7,176,3,0,95,
		5,12,1,72,80,3,36,113,0,176,3,0,95,5,
		12,1,121,8,28,10,36,114,0,106,1,0,110,7,
		25,31,36,118,0,96,5,0,176,9,0,95,2,95,
		7,95,3,12,3,135,36,119,0,96,3,0,95,7,
		122,49,135,36,121,0,174,3,0,36,122,0,176,3,
		0,95,5,12,1,80,8,36,123,0,95,8,92,2,
		15,28,64,176,5,0,95,5,122,12,2,106,3,13,
		10,0,24,28,48,36,124,0,176,9,0,95,5,95,
		8,122,49,122,12,3,106,3,13,10,0,24,28,8,
		36,125,0,173,8,0,36,127,0,176,9,0,95,5,
		122,95,8,122,49,12,3,80,5,36,129,0,95,5,
		110,7
	};

	hb_vmExecute( pcode, symbols );
}

