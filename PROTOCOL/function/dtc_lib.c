/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义故障码数据处理函数
History:
	<author>	<time>		<desc>

************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dtc_lib.h"
#include "../public/protocol_config.h"
#include "../formula/formula_comply.h"
#include "../command/command.h"
#include "../public/public.h"
#include "../interface/protocol_define.h"
#include <assert.h>
#include "../InitConfigFromXml/init_config_from_xml_lib.h"
#include "../SpecialFunction/special_function.h"

STRUCT_SELECT_FUN stReadCurrentDTCFunGroup[] =
{
	{GENERAL_CURRENT_DTC, process_general_read_current_Dtc},
	{SEA1939_CURRENT_DTC, process_SEA1939_read_current_Dtc},
};

/*************************************************
Description:	获取读当前码函数
Input:
	cType		配置类型
Output:	保留
Return:	pf_general_function 函数指针
Others:
*************************************************/
pf_general_function get_read_current_DTC_fun( byte cType )
{
	int i = 0;

	for( i = 0; i < sizeof( stReadCurrentDTCFunGroup ) / sizeof( stReadCurrentDTCFunGroup[0] ); i++ )
		if( cType == stReadCurrentDTCFunGroup[i].cType )
			return stReadCurrentDTCFunGroup[i].pFun;

	return 0;
}

/*************************************************
Description:	读取当前码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_current_Dtc( void* pIn, void* pOut )
{
	pf_general_function pFun = NULL;

	pFun = get_read_current_DTC_fun( g_p_stProcessFunConfig->cCurrentDTCFunOffset );

	assert( pFun );

	pFun( pIn, pOut );
}

STRUCT_SELECT_FUN stReadHistoryDTCFunGroup[] =
{
	{GENERAL_HISTORY_DTC, process_general_read_history_Dtc},
	{SEA1939_HISTORY_DTC, process_SEA1939_general_read_history_Dtc},
};
/*************************************************
Description:	获取读历史码函数
Input:
	cType		配置类型
Output:	保留
Return:	pf_general_function 函数指针
Others:
*************************************************/
pf_general_function get_read_history_DTC_fun( byte cType )
{
	int i = 0;

	for( i = 0; i < sizeof( stReadHistoryDTCFunGroup ) / sizeof( stReadHistoryDTCFunGroup[0] ); i++ )
		if( cType == stReadHistoryDTCFunGroup[i].cType )
			return stReadHistoryDTCFunGroup[i].pFun;

	return 0;
}

/*************************************************
Description:	读取历史码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_history_Dtc( void* pIn, void* pOut )
{
	pf_general_function pFun = NULL;

	pFun = get_read_history_DTC_fun( g_p_stProcessFunConfig->cHistoryDTCFunOffset );

	assert( pFun );

	pFun( pIn, pOut );
}

/*************************************************
Description:	读取当前故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_general_read_current_Dtc( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	STRUCT_CHAIN_DATA_OUTPUT* pstDtcOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );
	byte *pcOutTemp = ( byte * )pOut;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度

	int iProcessStatus;//数据理状态

	byte cDtcMask = 0;

	byte cBufferOffset = 0;
	int iDTCConfigOffset = 0;
	int iCmdSum = 0;
	byte cConfigTemp[4] = {0};
	uint32 u32CmdTemp[6] = {0};
	int * piCmdIndex = NULL;
	int i = 0;
	byte cDtcConfigType = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iDTCConfigOffset = atoi( pstParam->pcData );//获得故障码配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cDtcConfigType = ( byte )atoi( pstParam->pcData ); //获得故障码配置模板号

	select_dtc_config( iDTCConfigOffset, cDtcConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	//获取读码命令条数、命令内容
	iCmdSum = get_string_type_data_to_uint32( u32CmdTemp, pstParam->pcData, pstParam->iLen );

	if( 0 == iCmdSum )
	{
		general_return_status( SUCCESS, NULL, 0, pcOutTemp );
		pcOutTemp[2] = 0;//不提示

		free( pstDtcOut );

		return ;
	}

	piCmdIndex = ( int * )malloc( sizeof( int ) * ( iCmdSum + 1 ) );

	piCmdIndex[0] = iCmdSum;

	for( i = 0; i < iCmdSum; i++ )
	{
		piCmdIndex[i + 1] = ( int )u32CmdTemp[i];
	}

	iReceiveResult = send_and_receive_cmd( piCmdIndex );
	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ piCmdIndex[iCmdSum] ].cBufferOffset;
	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;
	cDtcMask = g_p_stGeneralReadDtcConfig->cCurrentDtcMask;

	switch( iReceiveResult )
	{
	case SUCCESS:
	{
		iProcessStatus = process_Dtc_data( g_stBufferGroup[cBufferOffset].cBuffer, iReceiveValidLen,
		                                   pstDtcOut, cDtcMask , 1);

		if( ( iProcessStatus == NO_HISTORY_DTC ) || ( iProcessStatus == NO_CURRENT_DTC ) )
		{
			general_return_status( iProcessStatus, NULL, 0, pOut );
		}
		else
		{
			copy_data_to_out( pOut, pstDtcOut );
			free_param_out_data_space( pstDtcOut );
		}

	}
	break;

	case NEGATIVE:
	case FRAME_TIME_OUT:
	case TIME_OUT:
	default:
		general_return_status( iReceiveResult, g_stBufferGroup[cBufferOffset].cBuffer, 3, pOut );
		break;

	}


	free( piCmdIndex );
	free( pstDtcOut );
}



/*************************************************
Description:	读取当前故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_SEA1939_read_current_Dtc( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	STRUCT_CHAIN_DATA_OUTPUT* pstDtcOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );
	byte *pcOutTemp = ( byte * )pOut;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度

	int iProcessStatus;//数据理状态

	byte cDtcMask = 0;

	byte cBufferOffset = 0;
	int iDTCConfigOffset = 0;
	int iCmdSum = 0;
	byte cConfigTemp[4] = {0};
	uint32 u32CmdTemp[6] = {0};
	int * piCmdIndex = NULL;
	int i = 0;
	byte cDtcConfigType = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iDTCConfigOffset = atoi( pstParam->pcData );//获得故障码配置ID  

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cDtcConfigType = ( byte )atoi( pstParam->pcData ); //获得故障码配置模板号

	select_dtc_config( iDTCConfigOffset, cDtcConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	//获取读码命令条数、命令内容
	iCmdSum = get_string_type_data_to_uint32( u32CmdTemp, pstParam->pcData, pstParam->iLen );

	if( 0 == iCmdSum )
	{
		general_return_status( SUCCESS, NULL, 0, pcOutTemp );
		pcOutTemp[2] = 0;//不提示

		free( pstDtcOut );

		return ;
	}

	piCmdIndex = ( int * )malloc( sizeof( int ) * ( iCmdSum + 1 ) );

	piCmdIndex[0] = iCmdSum;

	for( i = 0; i < iCmdSum; i++ )
	{
		piCmdIndex[i + 1] = ( int )u32CmdTemp[i];
	}

	iReceiveResult = send_and_receive_cmd( piCmdIndex );
	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ piCmdIndex[iCmdSum] ].cBufferOffset;
	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;
	cDtcMask = g_p_stGeneralReadDtcConfig->cCurrentDtcMask;
    

	switch( iReceiveResult )
	{
	case SUCCESS:
		{
			iProcessStatus = process_SEA_1939_Dtc_data( g_stBufferGroup[cBufferOffset].cBuffer, iReceiveValidLen,
				pstDtcOut, cDtcMask );

			if( ( iProcessStatus == NO_HISTORY_DTC ) || ( iProcessStatus == NO_CURRENT_DTC ) )
			{
				general_return_status( iProcessStatus, NULL, 0, pOut );
			}
			else
			{
				copy_data_to_out( pOut, pstDtcOut );
				free_param_out_data_space( pstDtcOut );
			}

		}
		break;

	case NEGATIVE:
	case FRAME_TIME_OUT:
	case TIME_OUT:
	default:
		general_return_status( iReceiveResult, g_stBufferGroup[cBufferOffset].cBuffer, 3, pOut );
		break;

	}


	free( piCmdIndex );
	free( pstDtcOut );
}


/*************************************************
Description:	读取1939历史故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_SEA1939_general_read_history_Dtc( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	STRUCT_CHAIN_DATA_OUTPUT* pstDtcOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );
	byte *pcOutTemp = ( byte * )pOut;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度

	int iProcessStatus;//数据理状态

	byte cDtcMask = 0;

	byte cBufferOffset = 0;
	int iDTCConfigOffset = 0;
	int iCmdSum = 0;
	byte cConfigTemp[4] = {0};
	uint32 u32CmdTemp[6] = {0};
	int *piCmdIndex  = NULL;
	int i = 0;
	byte cDtcConfigType = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iDTCConfigOffset = atoi( pstParam->pcData );//获得故障码配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cDtcConfigType = ( byte )atoi( pstParam->pcData ); //获得故障码配置模板号

	select_dtc_config( iDTCConfigOffset, cDtcConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	//获取读码命令条数、命令内容
	iCmdSum = get_string_type_data_to_uint32( u32CmdTemp, pstParam->pcData, pstParam->iLen );

	if( 0 == iCmdSum )
	{
		general_return_status( SUCCESS, NULL, 0, pcOutTemp );
		pcOutTemp[2] = 0;//不提示

		free( pstDtcOut );

		return ;
	}

	piCmdIndex = ( int * )malloc( sizeof( int ) * ( iCmdSum + 1 ) );

	piCmdIndex[0] = iCmdSum;

	for( i = 0; i < iCmdSum; i++ )
	{
		piCmdIndex[i + 1] = ( int )u32CmdTemp[i];
	}

	iReceiveResult = send_and_receive_cmd( piCmdIndex );

	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ piCmdIndex[iCmdSum] ].cBufferOffset;
	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;
	cDtcMask = g_p_stGeneralReadDtcConfig->cHistoryDtcMask;

	switch( iReceiveResult )
	{
	case SUCCESS:
		{
			iProcessStatus = process_SEA_1939_Dtc_data( g_stBufferGroup[cBufferOffset].cBuffer, iReceiveValidLen,
				pstDtcOut, cDtcMask );

			if( ( iProcessStatus == NO_HISTORY_DTC ) || ( iProcessStatus == NO_CURRENT_DTC ) )
			{
				general_return_status( iProcessStatus, NULL, 0, pOut );
			}
			else
			{
				copy_data_to_out( pOut, pstDtcOut );
				free_param_out_data_space( pstDtcOut );
			}

		}
		break;

	case NEGATIVE:
	case FRAME_TIME_OUT:
	case TIME_OUT:
	default:
		general_return_status( iReceiveResult, g_stBufferGroup[cBufferOffset].cBuffer, 3, pOut );
		break;

	}

	free( piCmdIndex );
	free( pstDtcOut );
}

/*************************************************
Description:	读取历史故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_general_read_history_Dtc( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	STRUCT_CHAIN_DATA_OUTPUT* pstDtcOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );
	byte *pcOutTemp = ( byte * )pOut;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度

	int iProcessStatus;//数据理状态

	byte cDtcMask = 0;

	byte cBufferOffset = 0;
	int iDTCConfigOffset = 0;
	int iCmdSum = 0;
	byte cConfigTemp[4] = {0};
	uint32 u32CmdTemp[6] = {0};
	int *piCmdIndex  = NULL;
	int i = 0;
	byte cDtcConfigType = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iDTCConfigOffset = atoi( pstParam->pcData );//获得故障码配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cDtcConfigType = ( byte )atoi( pstParam->pcData ); //获得故障码配置模板号

	select_dtc_config( iDTCConfigOffset, cDtcConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	//获取读码命令条数、命令内容
	iCmdSum = get_string_type_data_to_uint32( u32CmdTemp, pstParam->pcData, pstParam->iLen );

	if( 0 == iCmdSum )
	{
		general_return_status( SUCCESS, NULL, 0, pcOutTemp );
		pcOutTemp[2] = 0;//不提示

		free( pstDtcOut );

		return ;
	}

	piCmdIndex = ( int * )malloc( sizeof( int ) * ( iCmdSum + 1 ) );

	piCmdIndex[0] = iCmdSum;

	for( i = 0; i < iCmdSum; i++ )
	{
		piCmdIndex[i + 1] = ( int )u32CmdTemp[i];
	}

	iReceiveResult = send_and_receive_cmd( piCmdIndex );

	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ piCmdIndex[iCmdSum] ].cBufferOffset;
	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;
	cDtcMask = g_p_stGeneralReadDtcConfig->cHistoryDtcMask;

	switch( iReceiveResult )
	{
	case SUCCESS:
	{
		iProcessStatus = process_Dtc_data( g_stBufferGroup[cBufferOffset].cBuffer, iReceiveValidLen,
		                                   pstDtcOut, cDtcMask, 1);

		if( ( iProcessStatus == NO_HISTORY_DTC ) || ( iProcessStatus == NO_CURRENT_DTC ) )
		{
			general_return_status( iProcessStatus, NULL, 0, pOut );
		}
		else
		{
			copy_data_to_out( pOut, pstDtcOut );
			free_param_out_data_space( pstDtcOut );
		}

	}
	break;

	case NEGATIVE:
	case FRAME_TIME_OUT:
	case TIME_OUT:
	default:
		general_return_status( iReceiveResult, g_stBufferGroup[cBufferOffset].cBuffer, 3, pOut );
		break;

	}

	free( piCmdIndex );
	free( pstDtcOut );
}

/*
typedef struct
{
byte cDtcStartOffset;	//有效回复中保存DTC起始偏移，从SID回复开始
byte cDtcBytesInCmd;	//命令中几个字节表示一个故障码
byte cStatusOffset;		//故障码状态在一个完整故障码中的偏移
byte cDtcBytesInDisplay;//一个故障码需要显示几个字节
byte cCurrentDtcMask;	//当前故障码mask
byte cHistoryDtcMask;	//历史故障码mask
}STRUCT_READ_DTC_CONFIG;
*/
/*************************************************
Description:	处理故障码数据
Input:
	pcDctData	故障码回复命令存储地址
	iValidLen	有效长度
	cDtcMask	故障码状态

Output:	pstDtc	输出链表指针
Return:	int		读码结果（无当前码、无历史码
							、成功）
Others:
*************************************************/
int process_Dtc_data( byte* pcDctData, int iValidLen, STRUCT_CHAIN_DATA_OUTPUT* pstDtc, byte cDtcMask, bool bFunction)
{
	int i = 0;
	int j = 0;
	int iDtcNum = 0;
	byte *pcDtcStart = NULL;//故障码起始存放地址
	byte cDtcStatusCache[256] = {0};
	byte cDtcStartOffset, cDtcBytesInCmd, cStatusOffset, cDtcBytesInDisplay;
	bool bDtcdisplay = false;
	byte DtcHead = 0;
	
	STRUCT_CHAIN_DATA_NODE *phead  = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp1 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp2 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp3 = NULL;

	pstDtc->cKeyByte[0] = 1;
	pstDtc->cKeyByte[1] = 0;
	pstDtc->cKeyByte[2] = 0;
	pstDtc->stJump.cLenHighByte = 0;
	pstDtc->stJump.cLenLowByte = 0;
	pstDtc->stTip.cLenHighByte = 0;
	pstDtc->stTip.cLenLowByte = 0;
	pstDtc->pstData = NULL;
	if (bFunction)
	{
		cDtcStartOffset	    = g_p_stGeneralReadDtcConfig->cDtcStartOffset;	//有效回复中保存DTC起始偏移，从SID回复开始
		cDtcBytesInCmd		= g_p_stGeneralReadDtcConfig->cDtcBytesInCmd;	//命令中几个字节表示一个故障码
		cStatusOffset		= g_p_stGeneralReadDtcConfig->cStatusOffset;	//故障码状态在一个完整故障码中的偏移
		cDtcBytesInDisplay  = g_p_stGeneralReadDtcConfig->cDtcBytesInDisplay;//一个故障码需要显示几个字节	
		if (iValidLen >= cDtcStartOffset)//有效数据比偏移大
		{
			iDtcNum = ( iValidLen -  cDtcStartOffset ) / cDtcBytesInCmd;

		}
		else
		{
			iDtcNum = 0;
		}

		if( 0 == iDtcNum ) //没有故障码
		{
			if( cDtcMask == g_p_stGeneralReadDtcConfig->cCurrentDtcMask )
				return NO_CURRENT_DTC;
			else if( cDtcMask == g_p_stGeneralReadDtcConfig->cHistoryDtcMask )
				return NO_HISTORY_DTC;
		}
		if (g_p_stGeneralReadDtcConfig->bDTCMaskDisplay)
		{
			bDtcdisplay = true;
		}
	}
	else
	{
		cDtcStartOffset	= g_p_stUDSFreezeDtcConfig->cFreezeDtcStartOffset;	//有效回复中保存快照DTC起始偏移，从SID回复开始
		cDtcBytesInCmd			= g_p_stUDSFreezeDtcConfig->cDtcBytesInCmd;	//命令中几个字节表示一个故障码
		cDtcBytesInDisplay		= g_p_stUDSFreezeDtcConfig->cDtcBytesInDisplay;//一个故障码需要显示几个字节
		if (iValidLen >= cDtcStartOffset)//有效数据比偏移大
		{
			iDtcNum = ( iValidLen -  cDtcStartOffset ) / cDtcBytesInCmd;

		}
		else
		{
			iDtcNum = 0;
		}
		
		if (0 == iDtcNum)
		{
			return NO_FREEZE_DTC;
		}
	}

	pcDtcStart = pcDctData + cDtcStartOffset;
		
	for( i = 0; i < iDtcNum; i++ )
	{
		ptemp2 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) ); //dtc id
		ptemp2->cLenHighByte = 0;
		ptemp2->cLenLowByte = cDtcBytesInDisplay * 2 + 1;//一个字节对应的字符长度为2
		ptemp2->pcData = ( byte * )malloc( ( cDtcBytesInDisplay * 2 + 1 + 1) * sizeof( byte ) );

		if (pcDtcStart[i * cDtcBytesInCmd] <= 0X3F)
		{
			ptemp2->pcData[0] = 'P';
			DtcHead = 0;
		} 
		else if (pcDtcStart[i * cDtcBytesInCmd] <= 0X7F)
		{
			ptemp2->pcData[0] = 'C';
			DtcHead = 0X40;
		}
		else if (pcDtcStart[i * cDtcBytesInCmd] <= 0XBF)
		{
			ptemp2->pcData[0] = 'B';
			DtcHead = 0X80;
		}
		else
		{
			ptemp2->pcData[0] = 'U';
			DtcHead = 0XC0;
		}

		for( j = 0; j < cDtcBytesInDisplay; j++ )
		{
			if (j == 0)
			{
				sprintf( &( ptemp2->pcData[j * 2 + 1] ), "%02X", pcDtcStart[j + i * cDtcBytesInCmd]-DtcHead);
			}
			else
			{
				sprintf( &( ptemp2->pcData[j * 2 + 1] ), "%02X", pcDtcStart[j + i * cDtcBytesInCmd]);
			}			
		}

		ptemp3 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) ); //dtc status

		if (bDtcdisplay)
		{	
			get_Dtc_status( pcDtcStart[cStatusOffset + i * cDtcBytesInCmd], cDtcStatusCache, cDtcMask );
			ptemp3->cLenHighByte = 0;
			ptemp3->cLenLowByte = ( byte )strlen( cDtcStatusCache );
			ptemp3->pcData = ( byte * )malloc( strlen( cDtcStatusCache ) * sizeof( byte ) );
			memcpy( ptemp3->pcData, cDtcStatusCache, strlen( cDtcStatusCache ) );			
		}
		else
		{
			ptemp3->cLenHighByte = 0;
			ptemp3->cLenLowByte = 0;
			ptemp3->pcData = 0;
		}

		ptemp2->pNextNode = ptemp3;

		if( i == 0 )
		{
			phead = ptemp2;
			ptemp1 = ptemp3;
		}
		else
		{
			ptemp1->pNextNode = ptemp2;
			ptemp1 = ptemp3;
		}

	}

	ptemp1->pNextNode = NULL;

	pstDtc->pstData = phead;

	return SUCCESS;
}


/*************************************************
Description:	SEA1939处理故障码数据
Input:
	pcDctData	故障码回复命令存储地址
	iValidLen	有效长度
	cDtcMask	故障码状态

Output:	pstDtc	输出链表指针
Return:	int		读码结果（无当前码、无历史码
							、成功）
Others:
*************************************************/
int process_SEA_1939_Dtc_data( byte* pcDctData, int iValidLen, STRUCT_CHAIN_DATA_OUTPUT* pstDtc, byte cDtcMask )
{
	int i = 0;
	int j = 0;
	int iDtcNum = 0;
	byte *pcDtcStart = NULL;//故障码起始存放地址
	byte cDtcStatusCache[256] = {0};
    UNN_2WORD_4BYTE DTCByte;
	byte DTC_SPN[50] = {0};
	byte DTC_FMI[10] = {0};

	byte cDtcStartOffset	= g_p_stGeneralReadDtcConfig->cDtcStartOffset;	//有效回复中保存DTC起始偏移，从SID回复开始
	byte cDtcBytesInCmd		= g_p_stGeneralReadDtcConfig->cDtcBytesInCmd;	//命令中几个字节表示一个故障码
	byte cStatusOffset		= g_p_stGeneralReadDtcConfig->cStatusOffset;	//故障码状态在一个完整故障码中的偏移
	byte cDtcBytesInDisplay = g_p_stGeneralReadDtcConfig->cDtcBytesInDisplay;//一个故障码需要显示几个字节

	STRUCT_CHAIN_DATA_NODE *phead  = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp1 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp2 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp3 = NULL;

	iDtcNum = ( iValidLen -  cDtcStartOffset ) / cDtcBytesInCmd;
	pcDtcStart = pcDctData + cDtcStartOffset;

	pstDtc->cKeyByte[0] = 1;
	pstDtc->cKeyByte[1] = 0;
	pstDtc->cKeyByte[2] = 0;
	pstDtc->stJump.cLenHighByte = 0;
	pstDtc->stJump.cLenLowByte = 0;
	pstDtc->stTip.cLenHighByte = 0;
	pstDtc->stTip.cLenLowByte = 0;
	pstDtc->pstData = NULL;

   if((iValidLen == 8)&&(pcDtcStart[0] == 0)&&(pcDtcStart[1] == 0)&&(pcDtcStart[2] == 0))
   {
	   iDtcNum = 0;
   }
	

	if( 0 == iDtcNum ) //没有故障码
	{
		if( cDtcMask == g_p_stGeneralReadDtcConfig->cCurrentDtcMask )
			return NO_CURRENT_DTC;
		else if( cDtcMask == g_p_stGeneralReadDtcConfig->cHistoryDtcMask )
			return NO_HISTORY_DTC;
	}

	for( i = 0; i < iDtcNum; i++ )
	{
		ptemp2 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) ); //dtc id
		ptemp2->cLenHighByte = 0;
        DTCByte.u32Bit = 0;
		DTCByte.u8Bit[2] = (pcDtcStart[i * cDtcBytesInCmd+2]>>5)&0x07;
		DTCByte.u8Bit[1] = pcDtcStart[i * cDtcBytesInCmd+1];
        DTCByte.u8Bit[0] = pcDtcStart[i * cDtcBytesInCmd+0];
        j = sprintf( DTC_SPN, "%d", DTCByte.u32Bit );
		cDtcBytesInDisplay = j;
		DTCByte.u8Bit[3] = pcDtcStart[i * cDtcBytesInCmd+2]&0x1f;
        j = sprintf( DTC_FMI, "%d", DTCByte.u8Bit[3] );
		ptemp2->cLenLowByte = cDtcBytesInDisplay +j+1;
        ptemp2->pcData = ( byte * )malloc( (ptemp2->cLenLowByte + 1 ) * sizeof( byte ) );
        memcpy( &( ptemp2->pcData[0] ),DTC_SPN,cDtcBytesInDisplay);
        ptemp2->pcData[cDtcBytesInDisplay] = '/';
        memcpy( &( ptemp2->pcData[cDtcBytesInDisplay+1] ),DTC_FMI,j);
        ptemp2->pcData[ptemp2->cLenLowByte] = 0;
       

		ptemp3 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) ); //dtc status
		
		if (g_p_stGeneralReadDtcConfig->bDTCMaskDisplay)
		{			
			ptemp3->cLenHighByte = 0;
			ptemp3->cLenLowByte = 0;
			ptemp3->pcData = 0;
		}
		else
		{
			get_Dtc_status( pcDtcStart[cStatusOffset + i * cDtcBytesInCmd], cDtcStatusCache, cDtcMask );
			ptemp3->cLenHighByte = 0;
			ptemp3->cLenLowByte = ( byte )strlen( cDtcStatusCache );
			ptemp3->pcData = ( byte * )malloc( strlen( cDtcStatusCache ) * sizeof( byte ) );
			memcpy( ptemp3->pcData, cDtcStatusCache, strlen( cDtcStatusCache ) );
		}
		
		ptemp2->pNextNode = ptemp3;

		if( i == 0 )
		{
			phead = ptemp2;
			ptemp1 = ptemp3;
		}
		else
		{
			ptemp1->pNextNode = ptemp2;
			ptemp1 = ptemp3;
		}

	}

	ptemp1->pNextNode = NULL;

	pstDtc->pstData = phead;

	return SUCCESS;
}
