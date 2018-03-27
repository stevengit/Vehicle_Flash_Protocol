/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义激活处理函数
History:
	<author>	<time>		<desc>
************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include "active_ecu_lib.h"
//#include "..\protocol\sae_1939.h"
#include "..\protocol\iso_15765.h"
#include "..\public\public.h"
#include "..\command\command.h"
#include "..\public\protocol_config.h"
#include "..\interface\protocol_interface.h"
#include "..\InitConfigFromXml\init_config_from_xml_lib.h"
#include <assert.h>
#include "..\SpecialFunction\special_function.h"
#include "idle_link_lib.h"
#include "quit_system_lib.h"
#include <string.h>


int g_iActiveECUStatus;

STRUCT_SELECT_FUN stActiveECUFunGroup[] =
{
	{ACTIVE_ECU_BY_GENERAL_CAN, process_active_ECU_by_general_CAN},
	{ACTIVE_ECU_BY_KWP, process_active_ECU_by_kwp},
	{ACTIVE_ECU_BY_5_BAUD_ADDRESS, process_active_ECU_by_5_baud_address},
};

/*************************************************
Description:	获取处理激活函数
Input:
	cType		配置类型
Output:	保留
Return:	pf_general_function 函数指针
Others:
*************************************************/
pf_general_function get_active_ECU_fun( byte cType )
{
	int i = 0;

	for( i = 0; i < sizeof( stActiveECUFunGroup ) / sizeof( stActiveECUFunGroup[0] ); i++ )
		if( cType == stActiveECUFunGroup[i].cType )
			return stActiveECUFunGroup[i].pFun;

	return 0;
}
/*************************************************
Description:	激活处理函数
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/

void process_active_ECU( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	byte *pcOutTemp = ( byte * )pOut;

	int * piActivateCmdIndex = NULL;
	int iActiveCmdSum = 0;
	int i = 0;
	pf_general_function pFun = NULL;
	byte cConfigType = 0;
	int iConfigOffset = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得处理函数配置ID

	pstParam = pstParam->pNextNode;
	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得处理函数配置模板号

	select_process_fun_config( iConfigOffset, cConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得安全进入配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得安全进入配置模板号

	init_security_access_config( iConfigOffset, cConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData );//获得空闲链接配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得空闲链接配置模板号

	select_idle_link_config( iConfigOffset, cConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );
	//获取空闲链接命令
	g_stInitXmlGobalVariable.m_iIdleLinkCmdGetFromXmlSum =
	    get_command_config_data( pstParam, &( g_stInitXmlGobalVariable.m_p_stIdleLinkCmdList ) );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iConfigOffset = atoi( pstParam->pcData ); //获得激活配置ID

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	cConfigType = ( byte )atoi( pstParam->pcData ); //获得激活配置模板号

	select_active_config( iConfigOffset, cConfigType );

	pstParam = pstParam->pNextNode;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );
	//获取激活命令条数、命令内容
	iActiveCmdSum = get_command_config_data( pstParam, &( g_stInitXmlGobalVariable.m_p_stCmdList ) );
	g_stInitXmlGobalVariable.m_iSpecificCmdGetFromXmlSum = iActiveCmdSum;

	if( 0 == iActiveCmdSum )
	{
		general_return_status( SUCCESS, NULL, 0, pcOutTemp );
		pcOutTemp[2] = 0;//不提示

		free_specific_command_config_space();

		return ;
	}

	piActivateCmdIndex = ( int * )malloc( sizeof( int ) * ( iActiveCmdSum + 1 ) );
	//存放激活命令索引内容
	piActivateCmdIndex[0] = iActiveCmdSum;

	for( i = 1; i < iActiveCmdSum + 1; i++ )
		piActivateCmdIndex[i] = i - 1;

	pFun = get_active_ECU_fun( g_p_stProcessFunConfig->cActiveECUFunOffset );

	assert( pFun );

	pFun( piActivateCmdIndex, pOut );

	g_iActiveECUStatus = ACTIVE_ECU_FAIL;

	if( 1 == *( byte * )pOut ) //如果激活成功
	{
		g_iActiveECUStatus = ACTIVE_ECU_SUCCESS;

		//第一次按照xml来配置空闲状态
		if( !set_idle_link( g_p_stIdleLinkConfig->cIdleLinkStatus ) )
		{
			//提示设置空闲链接失败
			special_return_status( PROCESS_OK | NO_JUMP | HAVE_TIP, NULL, "ID_STR_SET_IDLE_LINK_FAIL", 0, pOut );
		}
	}
	else
	{
		free_xml_config_space(); //系统激活失败释放内存。
	}

	if( piActivateCmdIndex != NULL )
	{
		free( piActivateCmdIndex );
	}
}

/*************************************************
Description:	普通CAN激活处理函数
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/

void process_active_ECU_by_general_CAN( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_OUTPUT* pstOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );

	byte *pcOutTemp = ( byte * )pOut;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度

	int * piActiveCmdIndex = ( int * )pIn;

	byte cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ piActiveCmdIndex[1] ].cBufferOffset;

	iReceiveResult = send_and_receive_cmd( piActiveCmdIndex );

	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

	switch( iReceiveResult )
	{
	case SUCCESS:
	{
		process_general_response( g_stBufferGroup[cBufferOffset].cBuffer, iReceiveValidLen, pstOut );
		copy_data_to_out( pOut, pstOut );
		free_param_out_data_space( pstOut );
	}
	break;

	case NEGATIVE:
	case FRAME_TIME_OUT:
	case TIME_OUT:
	default:
		general_return_status( iReceiveResult, g_stBufferGroup[cBufferOffset].cBuffer, 3, pOut );
		pcOutTemp[2] = 0;//不提示

		break;

	}


	free( pstOut );
}

/*************************************************
Description:	kwp协议激活处理函数
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
激活方式:0x01 高低电平（25ms），0x04
*************************************************/
void process_active_ECU_by_kwp( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_OUTPUT* pstOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );

	bool bSendStatus = false;

	byte *pcOutTemp = ( byte * )pOut;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度
	int * piActiveCmdIndex = ( int * )pIn;

	byte cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ piActiveCmdIndex[1] ].cBufferOffset;
	byte cRetransTime = 0;
	byte cRetransTimeTemp = 0;
	uint16 u16ECUResTimeout = 0;

	u16ECUResTimeout	= g_p_stISO14230Config->u16ECUResTimeout;
	cRetransTime		= g_p_stISO14230Config->cRetransTime;

	cRetransTimeTemp = cRetransTime;
	//暂时修改重发次数为1
	g_p_stISO14230Config->cRetransTime = 1;

	while( cRetransTimeTemp-- )
	{
		if( g_p_stGeneralActiveEcuConfig->cActiveMode != 0x00 ) /* 不发送C2命令 */
		{
			//发送激活设置命令
			bSendStatus = package_and_send_active_config();

			if( !bSendStatus )
			{
				general_return_status( FAILE, 0, 0, pOut );
				pcOutTemp[2] = 0;//不提示
				free( pstOut );
				//还原重发次数和帧超时时间
				g_p_stISO14230Config->cRetransTime = cRetransTime;
				return;
			}
		}

		iReceiveResult = send_and_receive_cmd( piActiveCmdIndex );

		if( iReceiveResult != SUCCESS ) //如果状态不是SUCCESS则重发
			continue;

		break;
	}

	iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

	switch( iReceiveResult )
	{
	case SUCCESS:
	{
		process_general_response( g_stBufferGroup[cBufferOffset].cBuffer, iReceiveValidLen, pstOut );
		copy_data_to_out( pOut, pstOut );
		free_param_out_data_space( pstOut );
	}
	break;

	case NEGATIVE:
	case FRAME_TIME_OUT:
	case TIME_OUT:
	default:
		general_return_status( iReceiveResult, g_stBufferGroup[cBufferOffset].cBuffer, 3, pOut );
		pcOutTemp[2] = 0;//不提示

		break;
	}

	//还原重发次数和帧超时时间
	g_p_stISO14230Config->cRetransTime = cRetransTime;
	free( pstOut );

}

/*************************************************
Description:	以5波特率地址码激活处理函数
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_active_ECU_by_5_baud_address( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_OUTPUT* pstOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );

	bool bSendStatus = false;
	bool bReceiveStatus	= false;

	byte *pcOutTemp = ( byte * )pOut;
	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度

	int * piActiveCmdIndex = ( int * )pIn;

	byte cBufferOffset = 0;

	byte cReceiveKeyByte[3] = {0};
	int i = 0;

	uint16	u16ECUResByteTimeout = g_p_stGeneralActiveEcuConfig->u16ECUResByteTimeout;

	STRUCT_CMD	stActiveCmdTemp = {0};

	//发送激活设置命令
	bSendStatus = package_and_send_active_config();

	if( !bSendStatus )
	{
		general_return_status( FAILE, 0, 0, pOut );
		pcOutTemp[2] = 0;//不提示
		free( pstOut );

		return;
	}

	if( g_p_stGeneralActiveEcuConfig->cActiveMode == 0x02 ) /* 普通的5bps激活，protocol取反 */
	{
		//接收关键字节，当激自检模式为[不自检]时VCI上传0x55 0xMM 0xNN，否则上传0xMM 0xNN
		bReceiveStatus = ( bool )receive_cmd( cReceiveKeyByte, g_p_stGeneralActiveEcuConfig->cReceiveKeyByte[0]
		                                      , u16ECUResByteTimeout * 3 );

		if( !bReceiveStatus )
		{
			general_return_status( TIME_OUT, 0, 0, pOut );
			pcOutTemp[2] = 0;//不提示
			free( pstOut );

			return;
		}

		//判断接收到的关键字节跟设定的是否相同
		for( ; i < g_p_stGeneralActiveEcuConfig->cReceiveKeyByte[0]; i++ )
		{
			if( cReceiveKeyByte[i] != g_p_stGeneralActiveEcuConfig->cReceiveKeyByte[i + 1] )
			{
				general_return_status( FAILE, 0, 0, pOut );
				pcOutTemp[2] = 0;//不提示
				free( pstOut );

				return;
			}

			//也可以进行其他操作
		}

		stActiveCmdTemp.cBufferOffset = 0;
		stActiveCmdTemp.cReserved = 0;
		stActiveCmdTemp.iCmdLen = 1;
		stActiveCmdTemp.pcCmd = ( byte* )malloc( sizeof( byte ) );
		stActiveCmdTemp.pcCmd[0] = ~cReceiveKeyByte[1];//修改激活命令，该命令仅一个字节

		//发送取反字节,对于03取反的可把取反的字节当做激活命令使用放到command.xml中
		if( !package_and_send_frame( FRAME_HEAD_NORMAL_FRAME, ( STRUCT_CMD* )&stActiveCmdTemp, 0xaa ) )
		{
			general_return_status( FAILE, 0, 0, pOut );
			pcOutTemp[2] = 0;//不提示
			free( pstOut );
			free( stActiveCmdTemp.pcCmd );
			return;
		}
	}
	else if( g_p_stGeneralActiveEcuConfig->cActiveMode == 0x05 ||  \
	         g_p_stGeneralActiveEcuConfig->cActiveMode == 0x06 ) /* 0x05 VDI取反 激活系统, 0x06 VDI不取反 */
	{
		bReceiveStatus = ( bool )receive_cmd( cReceiveKeyByte, g_p_stGeneralActiveEcuConfig->cReceiveKeyByte[0]
		                                      , u16ECUResByteTimeout * 3 );

	}

	if( piActiveCmdIndex == NULL )
	{
		if( !bReceiveStatus )
		{
			general_return_status( TIME_OUT, 0, 0, pOut );
			pcOutTemp[2] = 0;//不提示
			free( pstOut );
			return;
		}
		else
		{
			general_return_status( iReceiveResult, 0, 0, pOut );
			pcOutTemp[2] = 0;//不提示
		}
	}
	else
	{
		cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ piActiveCmdIndex[1] ].cBufferOffset;
		iReceiveResult = send_and_receive_cmd( piActiveCmdIndex );

		iReceiveValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

		switch( iReceiveResult )
		{
		case SUCCESS:
		{
			process_general_response( g_stBufferGroup[cBufferOffset].cBuffer, iReceiveValidLen, pstOut );
			copy_data_to_out( pOut, pstOut );
			free_param_out_data_space( pstOut );
		}
		break;

		case NEGATIVE:
		case FRAME_TIME_OUT:
		case TIME_OUT:
		default:
			general_return_status( iReceiveResult, g_stBufferGroup[cBufferOffset].cBuffer, 3, pOut );
			pcOutTemp[2] = 0;//不提示

			break;
		}
	}

	free( pstOut );
	free( stActiveCmdTemp.pcCmd );

}

