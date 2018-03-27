/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义冻结帧故障码和数据流数据处理函数
History:
	<author>	<time>		<desc>

************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freeze_lib.h"
#include "../public/protocol_config.h"
#include "../command/command.h"
#include "../formula/formula_comply.h"
#include "../public/public.h"
#include "../interface/protocol_define.h"
#include <assert.h>
#include "../InitConfigFromXml/init_config_from_xml_lib.h"
#include "../formula_parse/interface.h"
#include "ds_lib.h"
#include "../SpecialFunction/special_function.h"
#include "../function/dtc_lib.h"
int s_iUDSReadFreezeDTCCmdIndex = 0;//UDS读取冻结帧故障码命令索引，局部变量，本文件有效，为使得xml更简洁

STRUCT_SELECT_FUN stReadFreezeDTCFunGroup[] =
{
	{READ_FREEZE_DTC_BY_ISO14229, process_read_freeze_frame_DTC_by_ISO14229},
	{READ_FREEZE_DTC_BY_SAE_1939, process_read_freeze_frame_DTC_by_SAE1939},
};

/*************************************************
Description:	获取处理冻结帧读码函数
Input:
	cType		配置类型
Output:	保留
Return:	pf_general_function 函数指针
Others:
*************************************************/
pf_general_function get_read_freeze_DTC_fun( byte cType )
{
	int i = 0;

	for( i = 0; i < sizeof( stReadFreezeDTCFunGroup ) / sizeof( stReadFreezeDTCFunGroup[0] ); i++ )
		if( cType == stReadFreezeDTCFunGroup[i].cType )
			return stReadFreezeDTCFunGroup[i].pFun;

	return 0;
}

/*************************************************
Description:	冻结帧读码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_freeze_frame_DTC( void* pIn, void* pOut )
{
	pf_general_function pFun = NULL;

	pFun = get_read_freeze_DTC_fun( g_p_stProcessFunConfig->cFreezeDTCFunOffset );

	assert( pFun );

	pFun( pIn, pOut );
}

STRUCT_SELECT_FUN stReadFreezeDSFunGroup[] =
{
	{READ_FREEZE_DS_BY_ISO14229,					process_read_freeze_frame_DS_by_ISO14229},
	{READ_ONE_MEMORY_REQUEST_ALL_DATA_BY_ISO14230,	process_read_one_memory_request_all_data_DS_by_ISO14230},
	{READ_FREEZE_ACCORD_XML,						process_read_freeze_frame_DS_by_xml},
	{READ_FREEZE_DS_BY_SAE1939,                     process_read_freeze_frame_DS_by_SAE1939},

};
/*************************************************
Description:	获取处理冻结帧数据流函数
Input:
	cType		配置类型
Output:	保留
Return:	pf_general_function 函数指针
Others:
*************************************************/
pf_general_function get_read_freeze_DS_fun( byte cType )
{
	int i = 0;

	for( i = 0; i < sizeof( stReadFreezeDSFunGroup ) / sizeof( stReadFreezeDSFunGroup[0] ); i++ )
		if( cType == stReadFreezeDSFunGroup[i].cType )
			return stReadFreezeDSFunGroup[i].pFun;

	return 0;
}

/*************************************************
Description:	冻结帧读数据流
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_freeze_frame_DS( void* pIn, void* pOut )
{
	pf_general_function pFun = NULL;

	pFun = get_read_freeze_DS_fun( g_p_stProcessFunConfig->cFreezeDSFunOffset );

	assert( pFun );

	pFun( pIn, pOut );
}
/*************************************************
Description:	读冻结帧故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_freeze_frame_DTC_by_ISO14229( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	STRUCT_CHAIN_DATA_OUTPUT* pstDtcOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度
	int iProcessStatus = 0;//处理状态

	byte cBufferOffset = 0;
	byte cConfigType = 0;
	int iConfigOffset = 0;
	int iReadFreezeFrameDtcCmdIndex[2] = {1, 0};

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得读冻结帧故障码配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得读冻结帧故障码配置模板号

	select_freeze_dtc_config( iConfigOffset, cConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iReadFreezeFrameDtcCmdIndex[1] = atoi( pstParam->pcData ); //获得读冻结帧故障码命令索引
	s_iUDSReadFreezeDTCCmdIndex = iReadFreezeFrameDtcCmdIndex[1];

	iReceiveResult = send_and_receive_cmd( iReadFreezeFrameDtcCmdIndex );

	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDtcCmdIndex[1] ].cBufferOffset;

	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

	switch( iReceiveResult )
	{
	case SUCCESS:
	{
		iProcessStatus = process_Dtc_data( g_stBufferGroup[cBufferOffset].cBuffer,
		                 iReceiveValidLen, pstDtcOut, 0, 0);

		if( NO_FREEZE_DTC == iProcessStatus )
		{
			general_return_status( NO_FREEZE_DTC, NULL, 0, pOut );
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

	free( pstDtcOut );

}

/*************************************************
Description:	SAE1939读冻结帧故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_freeze_frame_DTC_by_SAE1939( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	STRUCT_CHAIN_DATA_OUTPUT* pstDtcOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度
	int iProcessStatus = 0;//处理状态

	byte cBufferOffset = 0;
	byte cConfigType = 0;
	int iConfigOffset = 0;
	int iReadFreezeFrameDtcCmdIndex[2] = {1, 0};

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得读冻结帧故障码配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得读冻结帧故障码配置模板号

	select_freeze_dtc_config( iConfigOffset, cConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iReadFreezeFrameDtcCmdIndex[1] = atoi( pstParam->pcData ); //获得读冻结帧故障码命令索引
	s_iUDSReadFreezeDTCCmdIndex = iReadFreezeFrameDtcCmdIndex[1];

	iReceiveResult = send_and_receive_cmd( iReadFreezeFrameDtcCmdIndex );

	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDtcCmdIndex[1] ].cBufferOffset;

	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;
    
    if ((g_stBufferGroup[cBufferOffset].cBuffer[1] ==0x00)&&(g_stBufferGroup[cBufferOffset].cBuffer[2] ==0x00)&&(g_stBufferGroup[cBufferOffset].cBuffer[3] ==0x00)&&(g_stBufferGroup[cBufferOffset].cBuffer[4] ==0x00))
    {
		special_return_status( PROCESS_FAIL| NO_JUMP | HAVE_TIP, NULL, "ID_WITHOUT_FREZZE", 0, pOut);
		free( pstDtcOut );
		return ;
    }
	switch( iReceiveResult )
	{
	case SUCCESS:
		{
			iProcessStatus = process_freeze_DTC_data_by_SAE1939( g_stBufferGroup[cBufferOffset].cBuffer,
				iReceiveValidLen, pstDtcOut);

			if( NO_FREEZE_DTC == iProcessStatus )
			{
				general_return_status( NO_FREEZE_DTC, NULL, 0, pOut );
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

	free( pstDtcOut );

}

/*************************************************
Description:	读冻结帧数据流
Input:	pIn		列表中DTC的ID
Output:	pOut	输出数据地址
Return:	保留
Others:	根据ISO14229协议
*************************************************/
void process_read_freeze_frame_DS_by_ISO14229( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度
	int iDtcID = 0;

	byte cReadFreezeDsMode			= 0;
	byte cFreezeDtcStartOffset		= 0;
	byte cFreezeRecordNumberOffset	= 0;
	byte cDtcBytesInCmd				= 0;
	byte cSaveRecordNumberOffset	= 0;
	byte cRecordNumberBytes			= 0;
	byte cSaveDtcOffset				= 0;
	byte cDtcBytes					= 0;

	byte cBufferOffset = 0;
	int iProcessStatus;//冻结帧数据流处理状态
	byte cErrorDID[] = {0, 0};
	byte cConfigType = 0;
	int iConfigOffset = 0;
	int iReadFreezeFrameDSCmdIndex[2] = {1, 0};

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iDtcID = atoi( pstParam->pcData ); //获得列表中DTC的ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得读冻结帧数据流配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得读冻结帧数据流配置模板号

	select_freeze_ds_config( iConfigOffset, cConfigType );

	cReadFreezeDsMode			= g_p_stUDSFreezeDtcConfig->cReadFreezeDsMode;//读取冻结帧数据流模式0：根据故障码 1：根据存储号
	cFreezeDtcStartOffset		= g_p_stUDSFreezeDtcConfig->cFreezeDtcStartOffset;//回复中DTC起始偏移，从SID回复开始
	cFreezeRecordNumberOffset	= g_p_stUDSFreezeDtcConfig->cFreezeRecordNumberOffset;
	cDtcBytesInCmd				= g_p_stUDSFreezeDtcConfig->cDtcBytesInCmd;//命令中几个字节表示一个故障码
	cSaveRecordNumberOffset		= g_p_stUDSFreezeDtcConfig->cSaveRecordNumberOffset;
	cRecordNumberBytes			= g_p_stUDSFreezeDtcConfig->cRecordNumberBytes;
	cSaveDtcOffset				= g_p_stUDSFreezeDtcConfig->cSaveDtcOffset;
	cDtcBytes					= g_p_stUDSFreezeDtcConfig->cDtcBytes;

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iReadFreezeFrameDSCmdIndex[1] = atoi( pstParam->pcData ); //获得读冻结帧数据流命令索引

	//获得存储冻结帧故障码或存储号的缓存偏移
	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ s_iUDSReadFreezeDTCCmdIndex ].cBufferOffset;

	if( cReadFreezeDsMode == 0 ) //判断读取冻结帧数据流方式 根据故障码号或存储号
	{
		memcpy( ( g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDSCmdIndex[1] ].pcCmd + cSaveDtcOffset ),
		        ( g_stBufferGroup[cBufferOffset].cBuffer + cFreezeDtcStartOffset + iDtcID * cDtcBytesInCmd ), cDtcBytes );
	}
	else if( cReadFreezeDsMode == 1 )
	{
		memcpy( ( g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDSCmdIndex[1] ].pcCmd + cSaveRecordNumberOffset ),
		        ( g_stBufferGroup[cBufferOffset].cBuffer + cFreezeRecordNumberOffset + iDtcID * cDtcBytesInCmd ), cRecordNumberBytes );
	}

	//获得存储冻结帧数据流的缓存偏移
	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDSCmdIndex[1] ].cBufferOffset;

	iReceiveResult = send_and_receive_cmd( iReadFreezeFrameDSCmdIndex );

	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

	switch( iReceiveResult )
	{
	case SUCCESS:
	{
		iProcessStatus = process_freeze_data_stream_by_ISO14229( g_stBufferGroup[cBufferOffset].cBuffer,
		                 iReceiveValidLen, pOut );

		if( iProcessStatus == MATCH_ERROR )
		{
			cErrorDID[0]	= *( byte* )pOut;
			cErrorDID[1]	= *( ( byte* )pOut + 1 );
			general_return_status( MATCH_ERROR, cErrorDID, 2, pOut );
		}
		else if( iProcessStatus == NO_FREEZE_DS )
		{
			general_return_status( NO_FREEZE_DS, NULL, 0, pOut );
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
}


/*************************************************
Description:	读冻结帧数据流
Input:	pIn		列表中DTC的ID
Output:	pOut	输出数据地址
Return:	保留
Others:	根据SAE1939协议
*************************************************/
void process_read_freeze_frame_DS_by_SAE1939( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度
	int iDtcID = 0;

	byte cReadFreezeDsMode			= 0;
	byte cFreezeDtcStartOffset		= 0;
	byte cFreezeRecordNumberOffset	= 0;
	byte cDtcBytesInCmd				= 0;
	byte cSaveRecordNumberOffset	= 0;
	byte cRecordNumberBytes			= 0;
	byte cSaveDtcOffset				= 0;
	byte cDtcBytes					= 0;

	byte cBufferOffset = 0;
	int iProcessStatus;//冻结帧数据流处理状态
	byte cErrorDID[] = {0, 0};
	byte cConfigType = 0;
	int iConfigOffset = 0;
	int iReadFreezeFrameDSCmdIndex[2] = {1, 0};

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iDtcID = atoi( pstParam->pcData ); //获得列表中DTC的ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得读冻结帧数据流配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得读冻结帧数据流配置模板号

	select_freeze_ds_config( iConfigOffset, cConfigType );

	cReadFreezeDsMode			= g_p_stUDSFreezeDtcConfig->cReadFreezeDsMode;//读取冻结帧数据流模式0：根据故障码 1：根据存储号
	cFreezeDtcStartOffset		= g_p_stUDSFreezeDtcConfig->cFreezeDtcStartOffset;//回复中DTC起始偏移，从SID回复开始
	cFreezeRecordNumberOffset	= g_p_stUDSFreezeDtcConfig->cFreezeRecordNumberOffset;
	cDtcBytesInCmd				= g_p_stUDSFreezeDtcConfig->cDtcBytesInCmd;//命令中几个字节表示一个故障码
	cSaveRecordNumberOffset		= g_p_stUDSFreezeDtcConfig->cSaveRecordNumberOffset;
	cRecordNumberBytes			= g_p_stUDSFreezeDtcConfig->cRecordNumberBytes;
	cSaveDtcOffset				= g_p_stUDSFreezeDtcConfig->cSaveDtcOffset;
	cDtcBytes					= g_p_stUDSFreezeDtcConfig->cDtcBytes;

	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ s_iUDSReadFreezeDTCCmdIndex ].cBufferOffset;



	iReceiveResult = SUCCESS;
	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

	switch( iReceiveResult )
	{
	case SUCCESS:
		{
			iProcessStatus = process_freeze_data_stream_by_SAE1939( g_stBufferGroup[cBufferOffset].cBuffer,
				iReceiveValidLen, pOut );

			if( iProcessStatus == MATCH_ERROR )
			{
				cErrorDID[0]	= *( byte* )pOut;
				cErrorDID[1]	= *( ( byte* )pOut + 1 );
				general_return_status( MATCH_ERROR, cErrorDID, 2, pOut );
			}
			else if( iProcessStatus == NO_FREEZE_DS )
			{
				general_return_status( NO_FREEZE_DS, NULL, 0, pOut );
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
}

/*************************************************
Description:	读冻结帧数据流
Input:	pIn
Output:	pOut	输出数据地址
Return:	保留
Others: 根据ISO14230协议，用存储号读取一块存储区
的所有数据。
*************************************************/
void process_read_one_memory_request_all_data_DS_by_ISO14230( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度
	int iDtcID = 0;

	byte cBufferOffset = 0;
	int iProcessStatus;//冻结帧数据流处理状态
	byte cErrorDID[] = {0, 0};
	byte cConfigType = 0;
	int iConfigOffset = 0;
	int iReadFreezeFrameDSCmdIndex[2] = {1, 0};

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得读冻结帧数据流配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得读冻结帧数据流配置模板号

	select_freeze_ds_config( iConfigOffset, cConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iReadFreezeFrameDSCmdIndex[1] = atoi( pstParam->pcData ); //获得读冻结帧数据流命令索引

	//获得存储冻结帧数据流的缓存偏移
	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDSCmdIndex[1] ].cBufferOffset;

	iReceiveResult = send_and_receive_cmd( iReadFreezeFrameDSCmdIndex );

	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

	switch( iReceiveResult )
	{
	case SUCCESS:
	case NEGATIVE://有可能在不支持冻结帧数据流时ECU回复消极相应
	{
		iProcessStatus =
		    process_one_memory_request_all_data_freeze_data_stream_by_ISO14230( g_stBufferGroup[cBufferOffset].cBuffer,
		            iReceiveValidLen, pOut );

		if( iProcessStatus == MATCH_ERROR )
		{
			cErrorDID[0]	= *( byte* )pOut;
			cErrorDID[1]	= *( ( byte* )pOut + 1 );
			general_return_status( MATCH_ERROR, cErrorDID, 2, pOut );
		}
		else if( iProcessStatus == NO_FREEZE_DS )
		{
			general_return_status( NO_FREEZE_DS, NULL, 0, pOut );
		}

	}
	break;

	case FRAME_TIME_OUT:
	case TIME_OUT:
	default:
		general_return_status( iReceiveResult, g_stBufferGroup[cBufferOffset].cBuffer, 3, pOut );
		break;
	}
}

/*
typedef struct _STRUCT_UDS_FREEZE_DTC_CONFIG
{
byte cFreezeDtcStartOffset;	//有效回复中保存快照DTC起始偏移，从SID回复开始
byte cDtcBytesInCmd;	//命令中几个字节表示一个故障码
byte cDtcBytesInDisplay;//一个故障码需要显示几个字节
byte cReadFreezeDsMode; //读取冻结帧数据流模式 0：根据故障码 1：根据存储号

}STRUCT_UDS_FREEZE_DTC_CONFIG;
*/

/*************************************************
Description:	请求支持冻结帧的故障码或故障码存储号
Input:
	pcDctData	故障码回复命令存储地址
	iValidLen	有效长度

Output:	pstDtc	输出链表指针
Return:	int 处理状态
Others:	本函数按照SAE1939协议处理，读取命令
		为：
*************************************************/
int process_freeze_DTC_data_by_SAE1939( byte* pcDctData, int iValidLen, STRUCT_CHAIN_DATA_OUTPUT* pstDtc )
{
	int i = 0;
	int j = 0;
	int iDtcNum = 0;
	byte *pcDtcStart = NULL;//故障码起始存放地址
	UNN_2WORD_4BYTE DTCByte;
	byte DTC_SPN[50] = {0};
	byte DTC_FMI[10] = {0};

	byte cFreezeDtcStartOffset	= g_p_stUDSFreezeDtcConfig->cFreezeDtcStartOffset;	//有效回复中保存快照DTC起始偏移，从SID回复开始
	byte cDtcBytesInCmd			= g_p_stUDSFreezeDtcConfig->cDtcBytesInCmd;	//命令中几个字节表示一个故障码
	byte cDtcBytesInDisplay		= g_p_stUDSFreezeDtcConfig->cDtcBytesInDisplay;//一个故障码需要显示几个字节
	byte cSaveRecordNumberOffset = g_p_stUDSFreezeDtcConfig->cSaveRecordNumberOffset;

	STRUCT_CHAIN_DATA_NODE *phead  = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp1 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp2 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp3 = NULL;

	//iDtcNum = ( iValidLen -  cFreezeDtcStartOffset ) / cDtcBytesInCmd;
     if (pcDctData[0] == 0)
     {
		 iDtcNum = 0;
     }
	 else
	 {
		 iDtcNum = 1;
	 }

	pcDtcStart = pcDctData + cFreezeDtcStartOffset;

	pstDtc->cKeyByte[0] = 1;
	pstDtc->cKeyByte[1] = 0;
	pstDtc->cKeyByte[2] = 0;
	pstDtc->stJump.cLenHighByte = 0;
	pstDtc->stJump.cLenLowByte = 0;
	pstDtc->stTip.cLenHighByte = 0;
	pstDtc->stTip.cLenLowByte = 0;
	pstDtc->pstData = NULL;

	if( 0 == iDtcNum ) //没有冻结帧故障码
		return NO_FREEZE_DTC;

	for( i = 0; i < iDtcNum; i++ )
	{
// 		ptemp2 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) ); //dtc id
// 		ptemp2->cLenHighByte = 0;
// 		ptemp2->cLenLowByte = cDtcBytesInDisplay * 2;//一个字节对应的字符长度为2
// 		ptemp2->pcData = ( byte * )malloc( ( cDtcBytesInDisplay * 2 + 1 ) * sizeof( byte ) );
// 
// 		for( j = 0; j < cDtcBytesInDisplay; j++ )
// 		{
// 			/*sprintf_s( &( ptemp2->pcData[j * 2] ), ( cDtcBytesInDisplay * 2 + 1 - j * 2 ), "%02X", pcDtcStart[j + i * cDtcBytesInCmd] );*/
// 			sprintf( &( ptemp2->pcData[j * 2] ), "%02X", pcDtcStart[j + i * cDtcBytesInCmd] );
// 		}


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

		ptemp3->cLenHighByte = 0;	//冻结帧故障码没有状态
		ptemp3->cLenLowByte = 0;
		ptemp3->pcData = NULL;

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
Description:	处理冻结帧的数据流
Input:
	pcDctData	故障码回复命令存储地址
	iValidLen	有效长度

Output:	pOut	输出数据地址（出现匹配错误时
				存放出错时的DID）
Return:	void
Others:	本函数按照IS014229协议处理，读取命令
		为：19 04/05
*************************************************/
int process_freeze_data_stream_by_ISO14229( byte* pcDsData, int iValidLen,  void* pOut )
{
	byte *pcOutTemp = ( byte* )pOut;
	byte *pcHead = pcOutTemp;
	byte i = 0;
	byte j = 0;
	byte k = 0;
	byte m = 0;
	byte cDataValueCache[128] = {0};
	byte *pcFreezeDsData = NULL;
	byte cFreezeDsStartOffset = g_p_stUDSFreezeDtcConfig->cFreezeDsStartOffset;
	byte cDtcSupportDIDSum = pcDsData[cFreezeDsStartOffset];//得到该DTC冻结帧支持的DID总数
	byte cSysSupportDIDSum  = g_stInitXmlGobalVariable.m_cFreezeDSDIDSum;//得到该系统冻结帧支持的DID总数
	STRUCT_UDS_FREEZE_DS_CONFIG *pstFreezeConfig = NULL;
	int iAddressOffset = 0;
	int iDsID = 0;
	bool bMatchIDStatus = false;

	UNN_2WORD_4BYTE uDataLen;
	uDataLen.u32Bit = 0;

	pcFreezeDsData = pcDsData + cFreezeDsStartOffset + 1;//指向第一个DID

	pcOutTemp[0] = 1;
	pcOutTemp[1] = 0;
	pcOutTemp[2] = 0;

	pcOutTemp[3] = 0;
	pcOutTemp[4] = 0;
	pcOutTemp[5] = 0;
	pcOutTemp[6] = 0;
	pcOutTemp[7] = 0;//无跳步
	pcOutTemp[8] = 0;//无跳步
	pcOutTemp[9] = 0;//无提示
	pcOutTemp[10] = 0;//无提示

	uDataLen.u32Bit += 11;

	pcOutTemp = &pcOutTemp[11];//从这开始存放数据流ID长度、数据流ID、数据流值长度、数据流值

	if( iValidLen <= 6 ) //若有效长度小于等于6则认为无冻结帧数据流,参见ISO14229相关部分
		return NO_FREEZE_DS;

	for( i = 0; i < cDtcSupportDIDSum;i++)   //数据流总数
	{
		bMatchIDStatus = false;

		for( j = 0; j < cSysSupportDIDSum; j++ )
		{
			if( pcFreezeDsData[0] == g_p_stUDSFreezeDSConfig[j].cDIDHighByte
			        && pcFreezeDsData[1] == g_p_stUDSFreezeDSConfig[j].cDIDLowByte )
			{
				pstFreezeConfig = &g_p_stUDSFreezeDSConfig[j];
				bMatchIDStatus = true;
				break;
			}
		}

		if( !bMatchIDStatus )
		{
			pcHead[0] = pcFreezeDsData[0];//保存当前比照的DID
			pcHead[1] = pcFreezeDsData[1];

			return MATCH_ERROR;//匹配失败，返回
		}

		iDsID = 0;

		for( k = 0; k < pstFreezeConfig->cDSItemSum; k++ )
		{
			iDsID = pstFreezeConfig->pcSpecificDIDRule[0 + k * 4];
			iDsID <<= 8;
			iDsID += pstFreezeConfig->pcSpecificDIDRule[1 + k * 4];
			/*iAddressOffset = sprintf_s( pcOutTemp + 1, 10, "%04d", iDsID ); //整型打印数据流ID*/
			iAddressOffset = sprintf( pcOutTemp + 2, "%04d", iDsID ); //整型打印数据流ID
			*pcOutTemp = 0;
			pcOutTemp++;
			*pcOutTemp = iAddressOffset;
			pcOutTemp += iAddressOffset + 1;
			uDataLen.u32Bit += iAddressOffset + 2;

			memset( cDataValueCache, 0, sizeof( cDataValueCache ) );

			for (m=0; m<g_p_stFreezeDSFormulaConfig->cItemSum; m++)
			{
				if (iDsID == g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iDSID)   //判断冻结帧数据流ID
				{
					if( FORMULA_PARSER == g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cFormulaType )
					{
						//调用公式解析器
						calculate(	pcFreezeDsData + 2 + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cValidByteNumber,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pcFormula,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iFormulaLen,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pStrFormat,
							cDataValueCache
							);
					}
					else
					{
						process_freeze_ds_calculate( iDsID, pcFreezeDsData + 2 + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							cDataValueCache );
					}
					break;
				}
			}
			*pcOutTemp = 0;
			pcOutTemp++;
			*pcOutTemp = ( byte )strlen( cDataValueCache );
			pcOutTemp++;
			uDataLen.u32Bit += 2;
			memcpy( pcOutTemp, cDataValueCache, strlen( cDataValueCache ) ); //存放计算后的值
			pcOutTemp += ( byte )strlen( cDataValueCache );
			uDataLen.u32Bit += ( byte )strlen( cDataValueCache );
		}

		pcFreezeDsData += 2 + pstFreezeConfig->cNeedByteSum;
	}

	pcHead[3] = uDataLen.u8Bit[3];
	pcHead[4] = uDataLen.u8Bit[2];
	pcHead[5] = uDataLen.u8Bit[1];
	pcHead[6] = uDataLen.u8Bit[0];

	return SUCCESS;
}



/*************************************************
Description:	处理冻结帧的数据流
Input:
	pcDctData	故障码回复命令存储地址
	iValidLen	有效长度

Output:	pOut	输出数据地址（出现匹配错误时
				存放出错时的DID）
Return:	void
Others:	本函数按照SAE1939协议处理，读取命令
		为：
*************************************************/
int process_freeze_data_stream_by_SAE1939( byte* pcDsData, int iValidLen,  void* pOut )
{
	byte *pcOutTemp = ( byte* )pOut;
	byte *pcHead = pcOutTemp;
	byte i = 0;
	byte j = 0;
	byte k = 0;
	byte m = 0;
	byte cDataValueCache[128] = {0};
	byte *pcFreezeDsData = NULL;
	byte cFreezeDsStartOffset = g_p_stUDSFreezeDtcConfig->cFreezeDsStartOffset;
	byte cDtcSupportDIDSum = 6;  //pcDsData[cFreezeDsStartOffset];//得到该DTC冻结帧支持的DID总数
	byte cSysSupportDIDSum  = 6; //g_stInitXmlGobalVariable.m_cFreezeDSDIDSum;//得到该系统冻结帧支持的DID总数
	STRUCT_UDS_FREEZE_DS_CONFIG *pstFreezeConfig = NULL;
	int iAddressOffset = 0;
	int iDsID = 0;
	bool bMatchIDStatus = false;

	UNN_2WORD_4BYTE uDataLen;
	uDataLen.u32Bit = 0;

	pcFreezeDsData = pcDsData + cFreezeDsStartOffset + 1;//指向第一个DID

	pcOutTemp[0] = 1;
	pcOutTemp[1] = 0;
	pcOutTemp[2] = 0;

	pcOutTemp[3] = 0;
	pcOutTemp[4] = 0;
	pcOutTemp[5] = 0;
	pcOutTemp[6] = 0;
	pcOutTemp[7] = 0;//无跳步
	pcOutTemp[8] = 0;//无跳步
	pcOutTemp[9] = 0;//无提示
	pcOutTemp[10] = 0;//无提示

	uDataLen.u32Bit += 11;

	pcOutTemp = &pcOutTemp[11];//从这开始存放数据流ID长度、数据流ID、数据流值长度、数据流值

	if( iValidLen <= 13 ) //若有效长度小于等于6则认为无冻结帧数据流,参见ISO14229相关部分
		return NO_FREEZE_DS;

	for( i = 0; i < cDtcSupportDIDSum;i++)   //数据流总数
	{
// 		bMatchIDStatus = false;
// 
// 		for( j = 0; j < cSysSupportDIDSum; j++ )
// 		{
// 			if( pcFreezeDsData[0] == g_p_stUDSFreezeDSConfig[j].cDIDHighByte
// 			        && pcFreezeDsData[1] == g_p_stUDSFreezeDSConfig[j].cDIDLowByte )
// 			{
// 				pstFreezeConfig = &g_p_stUDSFreezeDSConfig[j];
// 				bMatchIDStatus = true;
// 				break;
// 			}
// 		}
// 
// 		if( !bMatchIDStatus )
// 		{
// 			pcHead[0] = pcFreezeDsData[0];//保存当前比照的DID
// 			pcHead[1] = pcFreezeDsData[1];
// 
// 			return MATCH_ERROR;//匹配失败，返回
// 		}

		pstFreezeConfig = &g_p_stUDSFreezeDSConfig[i];

		iDsID = 0;

		for( k = 0; k < pstFreezeConfig->cDSItemSum; k++ )
		{
			iDsID = pstFreezeConfig->pcSpecificDIDRule[0 + k * 4];
			iDsID <<= 8;
			iDsID += pstFreezeConfig->pcSpecificDIDRule[1 + k * 4];
			/*iAddressOffset = sprintf_s( pcOutTemp + 1, 10, "%04d", iDsID ); //整型打印数据流ID*/
			iAddressOffset = sprintf( pcOutTemp + 2, "%04d", iDsID ); //整型打印数据流ID
			*pcOutTemp = 0;
			pcOutTemp++;
			*pcOutTemp = iAddressOffset;
			pcOutTemp += iAddressOffset + 1;
			uDataLen.u32Bit += iAddressOffset + 2;

			memset( cDataValueCache, 0, sizeof( cDataValueCache ) );

			for (m=0; m<g_p_stFreezeDSFormulaConfig->cItemSum; m++)
			{
				if (iDsID == g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iDSID)   //判断冻结帧数据流ID
				{
					if( FORMULA_PARSER == g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cFormulaType )
					{
						//调用公式解析器
						/*calculate(	pcFreezeDsData + 2 + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cValidByteNumber,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pcFormula,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iFormulaLen,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pStrFormat,
							cDataValueCache
							);*/
						calculate(	pcFreezeDsData + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cValidByteNumber,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pcFormula,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iFormulaLen,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pStrFormat,
							cDataValueCache
							);
					}
					else
					{
						process_freeze_ds_calculate( iDsID, pcFreezeDsData + 2 + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							cDataValueCache );
					}
					break;
				}
			}
			*pcOutTemp = 0;
			pcOutTemp++;
			*pcOutTemp = ( byte )strlen( cDataValueCache );
			pcOutTemp++;
			uDataLen.u32Bit += 2;
			memcpy( pcOutTemp, cDataValueCache, strlen( cDataValueCache ) ); //存放计算后的值
			pcOutTemp += ( byte )strlen( cDataValueCache );
			uDataLen.u32Bit += ( byte )strlen( cDataValueCache );
		}

		pcFreezeDsData +=  pstFreezeConfig->cNeedByteSum;
	}

	pcHead[3] = uDataLen.u8Bit[3];
	pcHead[4] = uDataLen.u8Bit[2];
	pcHead[5] = uDataLen.u8Bit[1];
	pcHead[6] = uDataLen.u8Bit[0];

	return SUCCESS;
}


/*************************************************
Description:	处理冻结帧的数据流
Input:
	pcDctData	故障码回复命令存储地址
	iValidLen	有效长度

Output:	pOut	输出数据地址（出现匹配错误时
				存放出错时的DID）
Return:	void
Others:	本函数按照IS014230协议处理，读取命令
		为：12 xx 00
*************************************************/
int process_one_memory_request_all_data_freeze_data_stream_by_ISO14230( byte* pcDsData, int iValidLen,  void* pOut )
{
	byte *pcOutTemp = ( byte* )pOut;
	byte *pcHead = pcOutTemp;
	byte i = 0;
	byte j = 0;
	byte k = 0;
	byte m = 0;
	byte cDataValueCache[128] = {0};
	byte *pcFreezeDsData = NULL;
	byte cFreezeDsStartOffset = 2;
	byte cGroupValidDataSum = ( byte )( iValidLen - 2 - 1 ); //得到该组有效数据的总数，即ID+关键字节
	byte cSysSupportDIDSum  = g_stInitXmlGobalVariable.m_cFreezeDSDIDSum;//得到该系统冻结帧支持的DID总数
	STRUCT_UDS_FREEZE_DS_CONFIG *pstFreezeConfig = NULL;
	int iAddressOffset = 0;
	int iDsID = 0;
	bool bMatchIDStatus = false;

	UNN_2WORD_4BYTE uDataLen;
	uDataLen.u32Bit = 0;

	pcFreezeDsData = pcDsData + cFreezeDsStartOffset;//指向第一个DID

	pcOutTemp[0] = 1;
	pcOutTemp[1] = 0;
	pcOutTemp[2] = 0;

	pcOutTemp[3] = 0;
	pcOutTemp[4] = 0;
	pcOutTemp[5] = 0;
	pcOutTemp[6] = 0;
	pcOutTemp[7] = 0;//无跳步
	pcOutTemp[8] = 0;//无跳步
	pcOutTemp[9] = 0;//无提示
	pcOutTemp[10] = 0;//无提示

	uDataLen.u32Bit += 11;

	pcOutTemp = &pcOutTemp[11];//从这开始存放数据流ID长度、数据流ID、数据流值长度、数据流值

	//若有效长度小于等于3则认为无冻结帧数据流,参见ISO14230相关部分,这里包含了消极相应的情况
	if( iValidLen <= 3 )
		return NO_FREEZE_DS;

	for( i = 0; i < cGroupValidDataSum;)
	{
		bMatchIDStatus = false;

		for( j = 0; j < cSysSupportDIDSum; j++ )
		{
			if( pcFreezeDsData[0] == g_p_stUDSFreezeDSConfig[j].cDIDHighByte
			        && pcFreezeDsData[1] == g_p_stUDSFreezeDSConfig[j].cDIDLowByte )
			{
				pstFreezeConfig = &g_p_stUDSFreezeDSConfig[j];
				bMatchIDStatus = true;
				break;
			}
		}

		if( !bMatchIDStatus )
		{
			pcHead[0] = pcFreezeDsData[0];//保存当前比照的DID
			pcHead[1] = pcFreezeDsData[1];

			return MATCH_ERROR;//匹配失败，返回
		}

		iDsID = 0;

		for( k = 0; k < pstFreezeConfig->cDSItemSum; k++ )
		{
			iDsID = pstFreezeConfig->pcSpecificDIDRule[0 + k * 4];
			iDsID <<= 8;
			iDsID += pstFreezeConfig->pcSpecificDIDRule[1 + k * 4];
			/*iAddressOffset = sprintf_s( pcOutTemp + 1, 10, "%04d", iDsID ); //整型打印数据流ID*/
			iAddressOffset = sprintf( pcOutTemp + 2, "%04d", iDsID ); //整型打印数据流ID
			*pcOutTemp = 0;
			pcOutTemp++;
			*pcOutTemp = iAddressOffset;
			pcOutTemp += iAddressOffset + 1;
			uDataLen.u32Bit += iAddressOffset + 2;

			memset( cDataValueCache, 0, sizeof( cDataValueCache ) );

			for (m=0; m<g_p_stFreezeDSFormulaConfig->cItemSum; m++)
			{
				if (iDsID == g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iDSID)   //判断冻结帧数据流ID
				{
					if( FORMULA_PARSER == g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cFormulaType )
					{
						//调用公式解析器
						calculate(	pcFreezeDsData + 2 + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cValidByteNumber,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pcFormula,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iFormulaLen,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pStrFormat,
							cDataValueCache
							);
					}
					else
					{
						process_freeze_ds_calculate( iDsID, pcFreezeDsData + 2 + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							cDataValueCache );
					}
					break;
				}
			}

			*pcOutTemp = 0;
			pcOutTemp++;
			*pcOutTemp = ( byte )strlen( cDataValueCache );
			pcOutTemp++;
			uDataLen.u32Bit += 2;
			memcpy( pcOutTemp, cDataValueCache, strlen( cDataValueCache ) ); //存放计算后的值
			pcOutTemp += ( byte )strlen( cDataValueCache );
			uDataLen.u32Bit += ( byte )strlen( cDataValueCache );
		}

		pcFreezeDsData += 2 + pstFreezeConfig->cNeedByteSum;

		i += 2 + pstFreezeConfig->cNeedByteSum;

	}

	pcHead[3] = uDataLen.u8Bit[3];
	pcHead[4] = uDataLen.u8Bit[2];
	pcHead[5] = uDataLen.u8Bit[1];
	pcHead[6] = uDataLen.u8Bit[0];

	return SUCCESS;
}
/*************************************************
Description:	处理冻结帧的数据流（数据流是固定的，不通过ID查找，直接显示XML中的配置项）
Input:
pcDctData	故障码回复命令存储地址
iValidLen	有效长度

Output:	pOut	输出数据地址（出现匹配错误时
存放出错时的DID）
Return:	void
*************************************************/
int process_freeze_data_stream_by_xml( byte* pcDsData, int iValidLen,  void* pOut )
{
	byte *pcOutTemp = ( byte* )pOut;
	byte *pcHead = pcOutTemp;
	byte i = 0;
	byte j = 0;
	byte k = 0;
	byte m = 0;
	byte cDataValueCache[128] = {0};
	byte *pcFreezeDsData = NULL;
	byte cFreezeDsStartOffset = g_p_stUDSFreezeDtcConfig->cFreezeDsStartOffset;
	byte cDtcSupportDIDSum = pcDsData[cFreezeDsStartOffset];//得到该DTC冻结帧支持的DID总数
	byte cSysSupportDIDSum  = g_stInitXmlGobalVariable.m_cFreezeDSDIDSum;//得到该系统冻结帧支持的DID总数
	STRUCT_UDS_FREEZE_DS_CONFIG *pstFreezeConfig = NULL;
	int iAddressOffset = 0;
	int iDsID = 0;
	bool bMatchIDStatus = false;

	UNN_2WORD_4BYTE uDataLen;
	uDataLen.u32Bit = 0;

	pcFreezeDsData = pcDsData;//指向第一个DID

	pcOutTemp[0] = 1;
	pcOutTemp[1] = 0;
	pcOutTemp[2] = 0;

	pcOutTemp[3] = 0;
	pcOutTemp[4] = 0;
	pcOutTemp[5] = 0;
	pcOutTemp[6] = 0;
	pcOutTemp[7] = 0;//无跳步
	pcOutTemp[8] = 0;//无跳步
	pcOutTemp[9] = 0;//无提示
	pcOutTemp[10] = 0;//无提示

	uDataLen.u32Bit += 11;

	pcOutTemp = &pcOutTemp[11];//从这开始存放数据流ID长度、数据流ID、数据流值长度、数据流值

	if( iValidLen <= 6 ) //若有效长度小于等于6则认为无冻结帧数据流,参见ISO14229相关部分
		return NO_FREEZE_DS;

	for( i = 0; i < g_p_stFreezeDSFormulaConfig->cItemSum; i++ )
	{
		bMatchIDStatus = false;

		for( j = 0; j < cSysSupportDIDSum; j++ )
		{
			if( 0 == g_p_stUDSFreezeDSConfig[j].cDIDHighByte
				&& i == g_p_stUDSFreezeDSConfig[j].cDIDLowByte )
			{
				pstFreezeConfig = &g_p_stUDSFreezeDSConfig[j];
				bMatchIDStatus = true;
				break;
			}
		}

		if( !bMatchIDStatus )
		{
			pcHead[0] = pcFreezeDsData[0];//保存当前比照的DID
			pcHead[1] = pcFreezeDsData[1];

			return MATCH_ERROR;//匹配失败，返回
		}

		for( k = 0; k < pstFreezeConfig->cDSItemSum; k++ )
		{
			iDsID = pstFreezeConfig->pcSpecificDIDRule[0 + k * 4];
			iDsID <<= 8;
			iDsID += pstFreezeConfig->pcSpecificDIDRule[1 + k * 4];
			/*iAddressOffset = sprintf_s( pcOutTemp + 1, 10, "%04d", iDsID ); //整型打印数据流ID*/
			iAddressOffset = sprintf( pcOutTemp + 2, "%04d", iDsID ); //整型打印数据流ID
			*pcOutTemp = 0;
			pcOutTemp++;
			*pcOutTemp = iAddressOffset;
			pcOutTemp += iAddressOffset + 1;
			uDataLen.u32Bit += iAddressOffset + 2;

			memset( cDataValueCache, 0, sizeof( cDataValueCache ) );

			for( m = 0; m < g_p_stFreezeDSFormulaConfig->cItemSum; m++ )
			{
				if( iDsID == g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iDSID )  //判断冻结帧数据流ID
				{
					if( FORMULA_PARSER == g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cFormulaType )
					{
						//调用公式解析器
						calculate(	pcFreezeDsData + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].cValidByteNumber,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pcFormula,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].iFormulaLen,
							g_p_stFreezeDSFormulaConfig->pstDSFormulaConfig[m].pStrFormat,
							cDataValueCache
							);
					}
					else
					{
						process_freeze_ds_calculate( iDsID, pcFreezeDsData + pstFreezeConfig->pcSpecificDIDRule[2 + k * 4],
							cDataValueCache );
					}

					break;
				}
			}

			*pcOutTemp = 0;
			pcOutTemp++;
			*pcOutTemp = ( byte )strlen( cDataValueCache );
			pcOutTemp++;
			uDataLen.u32Bit += 2;
			memcpy( pcOutTemp, cDataValueCache, strlen( cDataValueCache ) ); //存放计算后的值
			pcOutTemp += ( byte )strlen( cDataValueCache );
			uDataLen.u32Bit += ( byte )strlen( cDataValueCache );
		}
	}

	pcHead[3] = uDataLen.u8Bit[3];
	pcHead[4] = uDataLen.u8Bit[2];
	pcHead[5] = uDataLen.u8Bit[1];
	pcHead[6] = uDataLen.u8Bit[0];

	return SUCCESS;
}

/*************************************************
Description:	读冻结帧数据流（数据流是固定的，不通过ID查找，直接显示XML中的配置项）
Input:	pIn		列表中DTC的ID
Output:	pOut	输出数据地址
Return:	保留
Others:	根据ISO14229协议
*************************************************/
void process_read_freeze_frame_DS_by_xml( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度
	int iDtcID = 0;

	byte cReadFreezeDsMode			= 0;
	byte cFreezeDtcStartOffset		= 0;
	byte cFreezeRecordNumberOffset	= 0;
	byte cDtcBytesInCmd				= 0;
	byte cSaveRecordNumberOffset	= 0;
	byte cRecordNumberBytes			= 0;
	byte cSaveDtcOffset				= 0;
	byte cDtcBytes					= 0;

	byte cBufferOffset = 0;
	int iProcessStatus;//冻结帧数据流处理状态
	byte cErrorDID[] = {0, 0};
	byte cConfigType = 0;
	int iConfigOffset = 0;
	int iReadFreezeFrameDSCmdIndex[2] = {1, 0};

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iDtcID = atoi( pstParam->pcData ); //获得列表中DTC的ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得读冻结帧数据流配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得读冻结帧数据流配置模板号

	select_freeze_ds_config( iConfigOffset, cConfigType );

	cReadFreezeDsMode			= g_p_stUDSFreezeDtcConfig->cReadFreezeDsMode;//读取冻结帧数据流模式0：根据故障码 1：根据存储号
	cFreezeDtcStartOffset		= g_p_stUDSFreezeDtcConfig->cFreezeDtcStartOffset;//回复中DTC起始偏移，从SID回复开始
	cFreezeRecordNumberOffset	= g_p_stUDSFreezeDtcConfig->cFreezeRecordNumberOffset;
	cDtcBytesInCmd				= g_p_stUDSFreezeDtcConfig->cDtcBytesInCmd;//命令中几个字节表示一个故障码
	cSaveRecordNumberOffset		= g_p_stUDSFreezeDtcConfig->cSaveRecordNumberOffset;
	cRecordNumberBytes			= g_p_stUDSFreezeDtcConfig->cRecordNumberBytes;
	cSaveDtcOffset				= g_p_stUDSFreezeDtcConfig->cSaveDtcOffset;
	cDtcBytes					= g_p_stUDSFreezeDtcConfig->cDtcBytes;

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iReadFreezeFrameDSCmdIndex[1] = atoi( pstParam->pcData ); //获得读冻结帧数据流命令索引

	//获得存储冻结帧故障码或存储号的缓存偏移
	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ s_iUDSReadFreezeDTCCmdIndex ].cBufferOffset;

	if( cReadFreezeDsMode == 0 ) //判断读取冻结帧数据流方式 根据故障码号或存储号
	{
		memcpy( ( g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDSCmdIndex[1] ].pcCmd + cSaveDtcOffset ),
			( g_stBufferGroup[cBufferOffset].cBuffer + cFreezeDtcStartOffset + iDtcID * cDtcBytesInCmd ), cDtcBytes );
	}
	else if( cReadFreezeDsMode == 1 )
	{
		memcpy( ( g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDSCmdIndex[1] ].pcCmd + cSaveRecordNumberOffset ),
			( g_stBufferGroup[cBufferOffset].cBuffer + cFreezeRecordNumberOffset + iDtcID * cDtcBytesInCmd ), cRecordNumberBytes );
	}

	//获得存储冻结帧数据流的缓存偏移
	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ iReadFreezeFrameDSCmdIndex[1] ].cBufferOffset;

	iReceiveResult = send_and_receive_cmd( iReadFreezeFrameDSCmdIndex );

	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

	switch( iReceiveResult )
	{
	case SUCCESS:
		{
			iProcessStatus = process_freeze_data_stream_by_xml( g_stBufferGroup[cBufferOffset].cBuffer,
				iReceiveValidLen, pOut );

			if( iProcessStatus == MATCH_ERROR )
			{
				cErrorDID[0]	= *( byte* )pOut;
				cErrorDID[1]	= *( ( byte* )pOut + 1 );
				general_return_status( MATCH_ERROR, cErrorDID, 2, pOut );
			}
			else if( iProcessStatus == NO_FREEZE_DS )
			{
				general_return_status( NO_FREEZE_DS, NULL, 0, pOut );
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
}
