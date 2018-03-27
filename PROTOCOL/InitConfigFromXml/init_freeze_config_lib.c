/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义从xml获取冻结帧配置处理函数
History:
	<author>	<time>		<desc>

***********************************************************************/
#include "..\interface\protocol_define.h"
#include "..\public\protocol_config.h"
#include "init_config_from_xml_lib.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*************************************************
Description:	获取冻结帧故障码配置
Input:
	PIn				具体配置内容
Output:	保留
Return:	无
Others: 根据不同的配置模版进相应的分支
*************************************************/
void get_freeze_dtc_config_data( void* pIn )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	byte cConfigTemp[50] = {0};

	byte cDtcConfigType = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	get_cmd_config_content_data( cConfigTemp, pstParam->pcData, NO_LENGTH_LIMIT );

	cDtcConfigType	= cConfigTemp[1];

	switch( cDtcConfigType )
	{
	case UDS_FREEZE_DTC_CONFIG:
	{
		get_UDS_freeze_dtc_config_data( cConfigTemp );
	}
	break;

	default:
		break;
	}

}

/*************************************************
Description:	获取UDS冻结帧故障码配置
Input:
	PIn				具体配置内容
Output:	保留
Return:	无
Others:
*************************************************/
void get_UDS_freeze_dtc_config_data( const byte * pcSource )
{
	byte cConfigOffset = 0;

	cConfigOffset = pcSource[0];

	if( g_p_stUDSFreezeDtcConfigGroup[cConfigOffset] == NULL )
		g_p_stUDSFreezeDtcConfigGroup[cConfigOffset] = ( STRUCT_UDS_FREEZE_DTC_CONFIG * )malloc( sizeof( STRUCT_UDS_FREEZE_DTC_CONFIG ) );

	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cFreezeDtcStartOffset		= pcSource[2];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cFreezeRecordNumberOffset	= pcSource[3];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cDtcBytesInCmd			= pcSource[4];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cDtcBytesInDisplay		= pcSource[5];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cReadFreezeDsMode			= pcSource[6];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cSaveRecordNumberOffset	= pcSource[7];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cRecordNumberBytes		= pcSource[8];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cSaveDtcOffset			= pcSource[9];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cDtcBytes					= pcSource[10];
	g_p_stUDSFreezeDtcConfigGroup[cConfigOffset]->cFreezeDsStartOffset		= pcSource[11];


}

/*************************************************
Description:	选择故障码配置
Input:
	iConfigOffset		具体偏移
	g_cConfigType			故障码配置类型
Output:	保留
Return:	无
Others: 根据激活配置类型和配置偏移选择相应的配置
*************************************************/
void select_freeze_dtc_config( int iConfigOffset, const byte cConfigType )
{
	switch( cConfigType )
	{
	case UDS_FREEZE_DTC_CONFIG:
		g_p_stUDSFreezeDtcConfig = g_p_stUDSFreezeDtcConfigGroup[iConfigOffset];
		break;

	default:
		break;
	}
}

/*************************************************
Description:	释放存放冻结帧故障码配置的空间
Input:	无
Output:	保留
Return:	无
Others: 每添加一类配置就在该函数中添加相应的释放代码
并在quit_system_lib.c的free_xml_config_space
函数中调用该函数。
*************************************************/
void free_freeze_dtc_config_space( void )
{
	int i = 0;

	for( i = 0; i < sizeof( g_p_stUDSFreezeDtcConfigGroup ) / sizeof( g_p_stUDSFreezeDtcConfigGroup[0] ); i++ )
	{
		if( NULL != g_p_stUDSFreezeDtcConfigGroup[i] )
			free( g_p_stUDSFreezeDtcConfigGroup[i] );
	}

}

/*************************************************
Description:	获取冻结帧故障码配置
Input:
	PIn				具体配置内容
Output:	保留
Return:	无
Others: 根据不同的配置模版进相应的分支
*************************************************/
void get_freeze_ds_config_data( void* pIn )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	byte * pcTemp = NULL;
	byte cConfigTemp[15] = {0};
	byte cDSConfigType = 0;
	int iLen = 0;
	byte cConfigID = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	pcTemp = pstParam->pcData;

	iLen = get_command_one_block_config_data( cConfigTemp, pcTemp ); //获得ID
	pcTemp += iLen;

	cConfigID = cConfigTemp[0];

	iLen = get_command_one_block_config_data( cConfigTemp, pcTemp ); //获得模板号即配置类型
	pcTemp += iLen;

	cDSConfigType	= cConfigTemp[0];

	switch( cDSConfigType )
	{
	case UDS_FREEZE_DS_CONFIG:
	{
		get_UDS_freeze_ds_config_data( cConfigID, pcTemp );
	}
	break;

	default:
		break;
	}

}

/*************************************************
Description:	获取UDS冻结帧数据流配置
Input:
	cConfigID		配置ID
	pcSource		配置内容
Output:	保留
Return:	无
Others:
*************************************************/
void get_UDS_freeze_ds_config_data( byte cConfigID, byte * pcSource )
{
	byte cConfigOffset = 0;
	int iLen = 0;
	byte cConfigTemp[100] = {0};
	byte * pcTemp = pcSource;
	byte cDIDSum = 0;
	byte i = 0;
	int iCmdSum = 0;

	iLen = get_command_one_block_config_data( cConfigTemp, pcTemp ); //获得DID总数，一个DID一个配置组
	pcTemp += iLen;

	cDIDSum = cConfigTemp[0];
	g_stInitXmlGobalVariable.m_cFreezeDSDIDSum = cDIDSum;

	cConfigOffset = cConfigID;

	if( g_p_stUDSFreezeDSConfigGroup[cConfigOffset] == NULL )
		g_p_stUDSFreezeDSConfigGroup[cConfigOffset] = ( STRUCT_UDS_FREEZE_DS_CONFIG * )malloc( sizeof( STRUCT_UDS_FREEZE_DS_CONFIG ) * cDIDSum );

	for( i = 0; i < cDIDSum; i++ )
	{
		memcpy( cConfigTemp, pcTemp, 4 );
		cConfigTemp[4] = '\0';

		iLen = 4;

		iLen += strtol( cConfigTemp, NULL, 16 );

		iCmdSum = get_cmd_config_content_data( cConfigTemp, pcTemp, iLen );

		g_p_stUDSFreezeDSConfigGroup[cConfigOffset][i].cDIDHighByte	= cConfigTemp[0];
		g_p_stUDSFreezeDSConfigGroup[cConfigOffset][i].cDIDLowByte	= cConfigTemp[1];
		g_p_stUDSFreezeDSConfigGroup[cConfigOffset][i].cNeedByteSum	= cConfigTemp[2];
		g_p_stUDSFreezeDSConfigGroup[cConfigOffset][i].cDSItemSum	= cConfigTemp[3];
		//一组配置命令总数减去上面4个即为具体DID与支持的数据流项间规则
		g_p_stUDSFreezeDSConfigGroup[cConfigOffset][i].pcSpecificDIDRule = ( byte* )malloc( sizeof( byte ) * ( iCmdSum - 4 ) );

		memcpy( g_p_stUDSFreezeDSConfigGroup[cConfigOffset][i].pcSpecificDIDRule, &cConfigTemp[4], iCmdSum - 4 );

		pcTemp += iLen;
	}

}

/*************************************************
Description:	选择故障码配置
Input:
	iConfigOffset		具体偏移
	g_cConfigType			故障码配置类型
Output:	保留
Return:	无
Others: 根据激活配置类型和配置偏移选择相应的配置
*************************************************/
void select_freeze_ds_config( int iConfigOffset, const byte cConfigType )
{
	switch( cConfigType )
	{
	case UDS_FREEZE_DS_CONFIG:
		g_p_stUDSFreezeDSConfig = g_p_stUDSFreezeDSConfigGroup[iConfigOffset];
		break;

	default:
		break;
	}
}

/*************************************************
Description:	释放存放冻结帧数据流配置的空间
Input:	无
Output:	保留
Return:	无
Others: 这里目前默认一个系统只会有一个配置
每添加一类配置就在该函数中添加相应的释放代码
并在quit_system_lib.c的free_xml_config_space
函数中调用该函数。
*************************************************/
void free_freeze_ds_config_space( void )
{
	byte i = 0;
	byte j = 0;

	for( i = 0; i < sizeof( g_p_stUDSFreezeDSConfigGroup ) / sizeof( g_p_stUDSFreezeDSConfigGroup[0] ); i++ )
	{
		if( NULL != g_p_stUDSFreezeDSConfigGroup[i] )
		{
			for( j = 0; j < g_stInitXmlGobalVariable.m_cFreezeDSDIDSum; j++ )
			{
				if( NULL != g_p_stUDSFreezeDSConfigGroup[i][j].pcSpecificDIDRule )
					free( g_p_stUDSFreezeDSConfigGroup[i][j].pcSpecificDIDRule );
			}

			free( g_p_stUDSFreezeDSConfigGroup[i] );
		}

	}

}