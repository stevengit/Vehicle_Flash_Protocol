/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义数据流数据处理函数
History:
	<author>	<time>		<desc>
************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ds_lib.h"
#include "../formula/formula_comply.h"
#include <assert.h>
#include "../command/command.h"
#include "../public/public.h"
#include "../public/protocol_config.h"
#include "../InitConfigFromXml/init_config_from_xml_lib.h"
#include "../formula_parse/interface.h"
#include "../protocol/iso_15765.h"
#include "../protocol/iso_14230.h"
#include "..\SpecialFunction\special_function.h"

//数据流命令ID关联缓存号结构体数组，局部变量
STRUCT_CMD_ID_ASSOCIATE_BUFFER_OFFSET s_stDSCmdIDAssociateBufferOffsetGroup[20] =
{
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
	{0, 0,},
};
//暂时保存数据流缓存及状态的数组，局部变量
STRUCT_DS_STORAGE_BUFFER s_stDSBufferTempGroup[20] =
{
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
	{false, 0, 0,},
};

STRUCT_SELECT_FUN stReadDataStreamFunGroup[] =
{
	{READ_GENERAL_DATA_STREAM, process_read_general_data_stream},
	{TOGETHER_READ_GENERAL_DATA_STREAM, process_together_read_general_data_stream},
};

/*************************************************
Description:	获取处理读取数据流函数
Input:
	cType		配置类型
Output:	保留
Return:	pf_general_function 函数指针
Others:
*************************************************/
pf_general_function get_read_data_stream_fun( byte cType )
{
	int i = 0;

	for( i = 0; i < sizeof( stReadDataStreamFunGroup ) / sizeof( stReadDataStreamFunGroup[0] ); i++ )
		if( cType == stReadDataStreamFunGroup[i].cType )
			return stReadDataStreamFunGroup[i].pFun;

	return 0;
}
/*************************************************
Description:	读数据流
Input:	pIn		输入参数
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_data_stream( void* pIn, void* pOut )
{
	pf_general_function pFun = NULL;

	pFun = get_read_data_stream_fun( g_p_stProcessFunConfig->cDSFunOffset );

	assert( pFun );

	pFun( pIn, pOut );

}


/*************************************************
Description: 收取15765协议ECU回复的数据流命令
Input:	piCmdIndex 发送的命令号
Output:	int 	接收状态
Return:	保留
Others:
*************************************************/
int process_DS_receive_cmd_by_iso_15765(const int* piCmdIndex )
{
	int i =0;
	byte cBufferOffset = 0;
    byte cReceiveBuffer[20] = {0};
	int iReceiveResult = TIME_OUT;
	for (i = 0; i < piCmdIndex[0]; i++)
	{

		cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]].cBufferOffset;
		//把当前数据流的缓存内容保存到临时的缓存中
		//iReceiveResult = process_CAN_DS_receive_Cmd( cBufferOffset, cReceiveBuffer );
        iReceiveResult = process_CAN_receive_Cmd( cBufferOffset, cReceiveBuffer );
		if( ( iReceiveResult != SUCCESS ) && ( iReceiveResult != NEGATIVE ) ) //如果状态既不是SUCCESS又不是NEGATIVE则认为出错
		{
			return  iReceiveResult;
		}
		if ( g_DSProrocess) // 数据流功能
		{
			s_stDSBufferTempGroup[i].iValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

			memcpy( s_stDSBufferTempGroup[i].cBuffer, g_stBufferGroup[cBufferOffset].cBuffer,
				g_stBufferGroup[cBufferOffset].iValidLen );

			if( iReceiveResult == NEGATIVE )
			{
				s_stDSBufferTempGroup[i].bNegativeFlag = true;
			}
			else
			{
				s_stDSBufferTempGroup[i].bNegativeFlag = false;
			}
		}
	}

	return SUCCESS;

}


/*************************************************
Description: 收取14230协议ECU回复的数据流命令
Input:	piCmdIndex 发送的命令号
Output:	int 	接收状态
Return:	保留
Others:
*************************************************/
int process_DS_receive_cmd_by_iso_14230(const int* piCmdIndex )
{
	int i =0;
	byte cBufferOffset = 0;
	byte cReceiveBuffer[512] = {0};
	int iReceiveResult = TIME_OUT;
	for (i = 0; i < piCmdIndex[0]; i++)
	{

		cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]].cBufferOffset;
		//把当前数据流的缓存内容保存到临时的缓存中
		iReceiveResult = process_KWP_receive_Cmd( cBufferOffset, cReceiveBuffer );
		if( ( iReceiveResult != SUCCESS ) && ( iReceiveResult != NEGATIVE ) ) //如果状态既不是SUCCESS又不是NEGATIVE则认为出错
		{
			return  iReceiveResult;
		}

		if (g_DSProrocess)
		{
			s_stDSBufferTempGroup[i].iValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

			memcpy( s_stDSBufferTempGroup[i].cBuffer, g_stBufferGroup[cBufferOffset].cBuffer,
				g_stBufferGroup[cBufferOffset].iValidLen );

			if( iReceiveResult == NEGATIVE )
			{
				s_stDSBufferTempGroup[i].bNegativeFlag = true;
			}
			else
			{
				s_stDSBufferTempGroup[i].bNegativeFlag = false;
			}

		}
	}

	return SUCCESS;

}

/*************************************************
Description: 收取ECU回复的数据流命令
Input:	piCmdIndex 发送的命令号
Output:	int 	接收状态
Return:	保留
Others:   小心： 数据流收取与多帧的收放到了一起 用g_DSProrocess 判断是不是数据流功能
*************************************************/
int process_DS_receive_Cmd(const int *piCmdIndex)
{
	int iReturnStatus = TIME_OUT;

	switch( g_stInitXmlGobalVariable.m_cProtocolConfigType ) //这里根据协议配置模板类型进行区分
	{
	case ISO15765_CONFIG://CAN
		iReturnStatus = process_DS_receive_cmd_by_iso_15765( piCmdIndex );
		break;

	case ISO14230_CONFIG://K线
		iReturnStatus = process_DS_receive_cmd_by_iso_14230( piCmdIndex );
		break;

	default:
		iReturnStatus = PROTOCOL_ERROR;
		break;
	}

	return iReturnStatus;
}
/*************************************************
Description: 发送15765协议数据流命令
Input:	piCmdIndex 发送的命令号
Output:	int 	接收状态
Return:	保留
Others:
*************************************************/

bool process_DS_send_cmd_by_iso_15765(const int*  piCmdIndex )
{
	STRUCT_CMD stCopySendCmd = {0};
	int i = 0;
	int iNumber = 0;
	bool bSendStatus = false;

	stCopySendCmd.iCmdLen = 5;
	for (i = 0; i < piCmdIndex[0]; i++)
	{
		stCopySendCmd.iCmdLen += g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]].iCmdLen + 1;
	}
	stCopySendCmd.pcCmd = ( byte* )malloc( sizeof( byte ) * (stCopySendCmd.iCmdLen+1) );


	stCopySendCmd.pcCmd[0] = 5;  //VDI 发送的帧间隔时间
	stCopySendCmd.pcCmd[1] = 6;  //帧属性  接收超时后不再发送下一帧
	stCopySendCmd.pcCmd[2] = 0x03;
	stCopySendCmd.pcCmd[3] = 0xe8;  //VDI 等待ECU回复超时时间  1S 

	stCopySendCmd.pcCmd[4] = piCmdIndex[0];
	iNumber = 5;
	for( i = 0; i < piCmdIndex[0]; i++ )   //组装命令
	{
		stCopySendCmd.pcCmd[iNumber] = g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]].iCmdLen; //命令字节数
		iNumber++;
		memcpy(stCopySendCmd.pcCmd+iNumber,g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]].pcCmd,stCopySendCmd.pcCmd[iNumber-1]);
		iNumber += stCopySendCmd.pcCmd[iNumber-1];
	}

	bSendStatus = package_and_send_frame( FRAME_HEAD_MULTI_FRAME, &stCopySendCmd, g_p_stISO15765Config->cReserved );
	if( NULL != stCopySendCmd.pcCmd )
	{
		free( stCopySendCmd.pcCmd );
		stCopySendCmd.pcCmd = NULL;
	}
	return bSendStatus;
}

/*************************************************
Description: 发送14230协议数据流命令
Input:	piCmdIndex 发送的命令号
Output:	int 	接收状态
Return:	保留
Others:
*************************************************/

bool process_DS_send_cmd_by_iso_14230(const int*  piCmdIndex )
{
	STRUCT_CMD stCopySendCmd = {0};
	int i = 0;
	int iNumber = 0;
	bool bSendStatus = false;

	stCopySendCmd.iCmdLen = 5;
	for (i = 0; i < piCmdIndex[0]; i++)
	{
		stCopySendCmd.iCmdLen += g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]].iCmdLen + 1;
	}
	stCopySendCmd.pcCmd = ( byte* )malloc( sizeof( byte ) * (stCopySendCmd.iCmdLen+1) );


	stCopySendCmd.pcCmd[0] = 70;  //VDI 发送的帧间隔时间
	stCopySendCmd.pcCmd[1] = 6;  //帧属性  接收超时后不再发送下一帧
	stCopySendCmd.pcCmd[2] = 0;
	stCopySendCmd.pcCmd[3] = 40;  //VDI ECU回复完成的时间  比字节间隔时间大即可

	stCopySendCmd.pcCmd[4] = piCmdIndex[0];
	iNumber = 5;
	for( i = 0; i < piCmdIndex[0]; i++ )   //组装命令
	{
		stCopySendCmd.pcCmd[iNumber] = g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]].iCmdLen; //命令字节数
		iNumber++;
		pre_process_cmd( &g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]] ); //计算校验和
		memcpy(stCopySendCmd.pcCmd+iNumber,g_stInitXmlGobalVariable.m_p_stCmdList[piCmdIndex[i + 1]].pcCmd,stCopySendCmd.pcCmd[iNumber-1]);
		iNumber += stCopySendCmd.pcCmd[iNumber-1];
	}

	bSendStatus = package_and_send_frame( FRAME_HEAD_MULTI_FRAME, &stCopySendCmd, g_p_stISO14230Config->cTimeBetweenBytes );
	if( NULL != stCopySendCmd.pcCmd )
	{
		free( stCopySendCmd.pcCmd );
		stCopySendCmd.pcCmd = NULL;
	}
	return bSendStatus;
}

/*************************************************
Description: 发送数据流命令
Input:	piCmdIndex 发送的命令号
Output:	int 	接收状态
Return:	保留
Others:
*************************************************/
bool process_DS_send_Cmd(const int *piCmdIndex)
{
	bool bSendStatus = false;
	switch( g_stInitXmlGobalVariable.m_cProtocolConfigType ) //这里根据协议配置模板类型进行区分
	{
	case ISO15765_CONFIG://CAN
		bSendStatus = process_DS_send_cmd_by_iso_15765( piCmdIndex );
		break;

	case ISO14230_CONFIG://K线
		bSendStatus = process_DS_send_cmd_by_iso_14230( piCmdIndex );
		break;

	default:
		bSendStatus = false;
		break;
	}

	return bSendStatus;

}


/*************************************************
Description: 选择重发次数
Input:	void
Output:	byte 	重发次数
Return:	保留
Others:
*************************************************/
byte select_ResendTime_config()
{
	byte	cRetransTime = 0;
	switch( g_stInitXmlGobalVariable.m_cProtocolConfigType ) //这里根据协议配置模板类型进行区分
	{
	case ISO15765_CONFIG://CAN
		cRetransTime			= g_p_stISO15765Config->cRetransTime;
		break;

	case ISO14230_CONFIG://K线
		cRetransTime			= g_p_stISO14230Config->cRetransTime;
		break;

	default:
		cRetransTime = 0xff;
		break;
	}
	return cRetransTime;
}

/*************************************************
Description:	protocol把一屏数据流全部发给VDI，VDI把放回的结果一起上传
Input:	pIn		输入参数
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_together_read_general_data_stream( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_OUTPUT* pstDataStreamOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );
	int iReceiveResult = TIME_OUT;
	int * piCmdIndex = NULL;
	byte cStartOffset = 0;
	int i = 0;
	byte cItemSum = 0;
	int iDSCmdIndexTemp[2] = {1, 0};
	byte cBufferOffset = 0;
	byte CmdSum = 0;
	bool bSendStatus = false;
	byte	cRetransTime			= 0;

	cRetransTime = select_ResendTime_config();
	if (cRetransTime == 0xff)
	{
		free( pstDataStreamOut );
		//special_return_status(PROCESS_FAIL|NO_JUMP|HAVE_TIP,NULL,"Multi_frame_Resend_Time_False",NULL,0,ORIGINAL,NULL,pOut);
		general_load_tipID( "Multi_frame_Resend_Time_False", pOut );
		return;
	}
    g_DSProrocess = true;

	//获取当前屏的数据流配置
	cItemSum = get_current_screen_DS_config( pIn );

	piCmdIndex = ( int * )malloc( sizeof( int ) * ( cItemSum + 1 ) );

	for( i = 0; i < cItemSum; i++ )
	{
		piCmdIndex[i + 1] = g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig[i].iDSCmdID;
	}

	piCmdIndex[0] = remove_duplicate_int_element( piCmdIndex + 1, cItemSum );


	for( i = 0; i < piCmdIndex[0]; i++ )
	{
		s_stDSCmdIDAssociateBufferOffsetGroup[i].iCmdID = piCmdIndex[i + 1];
		s_stDSCmdIDAssociateBufferOffsetGroup[i].cBufferOffset = i;
	}

	if( piCmdIndex[0] > 20 ) /* 防止上位机传下来的数据使数组越界 */
	{
		CmdSum = 20;
	}
	else
	{
		CmdSum = piCmdIndex[0];
	}

	while(1)
	{
		bSendStatus = process_DS_send_Cmd(piCmdIndex);
		if( !bSendStatus )
		{
			g_DSProrocess = false;
			iReceiveResult =  FAILE;
			free( pstDataStreamOut );			
			general_return_status( iReceiveResult, NULL, 0, pOut );
			if (piCmdIndex != NULL)
			{
				free(piCmdIndex);
				piCmdIndex = NULL;
			}
			return;
		}

		iReceiveResult = process_DS_receive_Cmd(piCmdIndex);
		if( ( iReceiveResult != SUCCESS ) && ( iReceiveResult != NEGATIVE ) ) //如果状态既不是SUCCESS又不是NEGATIVE则认为出错
		{
			if (( --cRetransTime ) == 0)
			{
				g_DSProrocess = false;
				free( pstDataStreamOut );			
				general_return_status( iReceiveResult, NULL, 0, pOut );
				if (piCmdIndex != NULL)
				{
					free(piCmdIndex);
					piCmdIndex = NULL;
				}
				return;
			} 
			else
			{
				continue;
			}			
		}
		else
		{
			break;
		}
	}


	process_general_data_stream_data( ( STRUCT_DS_FORMULA_GROUP_CONFIG* )&g_stGeneralDSFormulaGroupConfig, cItemSum,
		pstDataStreamOut );
	copy_data_to_out( pOut, pstDataStreamOut );
	free_param_out_data_space( pstDataStreamOut );
    g_DSProrocess = false;
	free( pstDataStreamOut );
	if (piCmdIndex != NULL)
	{
		free(piCmdIndex);
		piCmdIndex = NULL;
	}
}
/*************************************************
Description:	处理读取普通据流函数
Input:	pIn		输入参数
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_general_data_stream( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_OUTPUT* pstDataStreamOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );

	int iReceiveResult = TIME_OUT;
	int * piCmdIndex = NULL;
	byte cStartOffset = 0;
	int i = 0;
	byte cItemSum = 0;
	int iDSCmdIndexTemp[2] = {1, 0};
	byte cBufferOffset = 0;
	byte CmdSum = 0;

	//获取当前屏的数据流配置
	cItemSum = get_current_screen_DS_config( pIn );

	piCmdIndex = ( int * )malloc( sizeof( int ) * ( cItemSum + 1 ) );

	for( i = 0; i < cItemSum; i++ )
	{
		piCmdIndex[i + 1] = g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig[i].iDSCmdID;
	}

	piCmdIndex[0] = remove_duplicate_int_element( piCmdIndex + 1, cItemSum );


	for( i = 0; i < piCmdIndex[0]; i++ )
	{
		s_stDSCmdIDAssociateBufferOffsetGroup[i].iCmdID = piCmdIndex[i + 1];
		s_stDSCmdIDAssociateBufferOffsetGroup[i].cBufferOffset = i;
	}

	if( piCmdIndex[0] > 20 ) /* 防止上位机传下来的数据使数组越界 */
	{
		CmdSum = 20;
	}
	else
	{
		CmdSum = piCmdIndex[0];
	}

	for( i = 0; i < CmdSum; i++ )
	{
		iDSCmdIndexTemp[1] = piCmdIndex[i + 1];

		iReceiveResult = send_and_receive_cmd( iDSCmdIndexTemp );

		if( ( iReceiveResult != SUCCESS ) && ( iReceiveResult != NEGATIVE ) )
		{
			free( pstDataStreamOut );			
			general_return_status( iReceiveResult, NULL, 0, pOut );
			if (piCmdIndex != NULL)
			{
				free(piCmdIndex);
				piCmdIndex = NULL;
			}
			return;
		}

		cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[iDSCmdIndexTemp[1]].cBufferOffset;
		//把当前数据流的缓存内容保存到临时的缓存中
		s_stDSBufferTempGroup[i].iValidLen = g_stBufferGroup[cBufferOffset].iValidLen;

		memcpy( s_stDSBufferTempGroup[i].cBuffer, g_stBufferGroup[cBufferOffset].cBuffer,
		        g_stBufferGroup[cBufferOffset].iValidLen );

		if( iReceiveResult == NEGATIVE )
		{
			s_stDSBufferTempGroup[i].bNegativeFlag = true;
		}
		else
		{
			s_stDSBufferTempGroup[i].bNegativeFlag = false;
		}

	}

	process_general_data_stream_data( ( STRUCT_DS_FORMULA_GROUP_CONFIG* )&g_stGeneralDSFormulaGroupConfig, cItemSum,
	                                  pstDataStreamOut );
	copy_data_to_out( pOut, pstDataStreamOut );
	free_param_out_data_space( pstDataStreamOut );

	free( pstDataStreamOut );
	if (piCmdIndex != NULL)
	{
		free(piCmdIndex);
		piCmdIndex = NULL;
	}
}



/*************************************************
Description:	处理动态数据流数据
Input:
	iDsId		数据流项ID
	pcDsSource	取值地址

Output:	pstDataStreamOut	输出链表指针
Return:	void
Others:
*************************************************/

void process_general_data_stream_data( STRUCT_DS_FORMULA_GROUP_CONFIG * pstDSGroupConfig, byte cItemSum,
                                       STRUCT_CHAIN_DATA_OUTPUT* pstDataStreamOut )

{
	int i = 0;
	int j = 0;
	byte cContentCache[128] = {0};

	STRUCT_CHAIN_DATA_NODE *phead  = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp1 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp2 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp3 = NULL;

	int	 iDisplayID = 0;
	byte cStartOffset = 0;
	byte cValidByteLen = 0;
	int  iDSCmdID = 0;
	byte cBufferOffset = 0;//缓存偏移

	byte cNegtiveDisplay[20] = {"Neg.$"};

	pstDataStreamOut->cKeyByte[0] = 1;
	pstDataStreamOut->cKeyByte[1] = 0;
	pstDataStreamOut->cKeyByte[2] = 0;
	pstDataStreamOut->stJump.cLenHighByte = 0;
	pstDataStreamOut->stJump.cLenLowByte = 0;
	pstDataStreamOut->stTip.cLenHighByte = 0;
	pstDataStreamOut->stTip.cLenLowByte = 0;

	for( i = 0; i < cItemSum; i++ )
	{
		iDisplayID			= pstDSGroupConfig->pstDSFormulaConfig[i].iDSID;
		cStartOffset		= pstDSGroupConfig->pstDSFormulaConfig[i].cValidByteOffset;
		cValidByteLen		= pstDSGroupConfig->pstDSFormulaConfig[i].cValidByteNumber;
		iDSCmdID			= pstDSGroupConfig->pstDSFormulaConfig[i].iDSCmdID;
		//得到缓存偏移
		cBufferOffset		= get_DS_buffer_offset( iDSCmdID );

		assert( cBufferOffset != 255 );

		ptemp2 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) );
		//一个字节对应的字符长度为2
		ptemp2->cLenHighByte = 0;
		ptemp2->cLenLowByte = 4;
		ptemp2->pcData = ( byte * )malloc( ( 4 + 1 ) * sizeof( byte ) );

		sprintf( ptemp2->pcData, "%04d", iDisplayID ); //整型打印数据流ID

		ptemp3 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) );

		memset( cContentCache, 0, sizeof( cContentCache ) );

		if( !s_stDSBufferTempGroup[cBufferOffset].bNegativeFlag )
		{
			if( PROTOCOL_PARSER == pstDSGroupConfig->pstDSFormulaConfig[i].cFormulaType )
			{
				//调用protocol的计算公式
				process_normal_ds_calculate(
				    pstDSGroupConfig->pstDSFormulaConfig[i].iDSID,
				    s_stDSBufferTempGroup[cBufferOffset].cBuffer + cStartOffset,
				    cContentCache
				);
			}
			else
			{
				//调用公式解析器
				calculate(
				    s_stDSBufferTempGroup[cBufferOffset].cBuffer + cStartOffset,
				    pstDSGroupConfig->pstDSFormulaConfig[i].cValidByteNumber,
				    pstDSGroupConfig->pstDSFormulaConfig[i].pcFormula,
				    pstDSGroupConfig->pstDSFormulaConfig[i].iFormulaLen,
				    pstDSGroupConfig->pstDSFormulaConfig[i].pStrFormat,
				    cContentCache
				);
			}

			ptemp3->cLenHighByte = 0;
			ptemp3->cLenLowByte = ( byte )strlen( cContentCache );
			ptemp3->pcData = ( byte * )malloc( strlen( cContentCache ) * sizeof( byte ) );
			memcpy( ptemp3->pcData, cContentCache, strlen( cContentCache ) );
		}
		else
		{
			for( j = 0; j < 3; j++ )
			{
				sprintf( &( cNegtiveDisplay[j * 2 + 5] ), "%02X", s_stDSBufferTempGroup[cBufferOffset].cBuffer[j] );
			}

			cNegtiveDisplay[11] = '\0';

			ptemp3->cLenHighByte = 0;
			ptemp3->cLenLowByte = ( byte )strlen( cNegtiveDisplay );
			ptemp3->pcData = ( byte * )malloc( strlen( cNegtiveDisplay ) * sizeof( byte ) );
			memcpy( ptemp3->pcData, cNegtiveDisplay, strlen( cNegtiveDisplay ) );
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

	pstDataStreamOut->pstData = phead;
}


/*************************************************
Description:	获取数据流配置数据
Input:
	pstDSConfig		指向数据流配置的指针
	pIn				输入内容

Output:	无
Return:	int 当前配置所占空间大小
Others: 注意:在调用本函数时需使用calloc函数申请
		STRUCT_DS_FORMULA_CONFIG类型变量的内存，
		以使内容在申请时即被初始化。
*************************************************/
int get_ds_config( STRUCT_DS_FORMULA_CONFIG * pstDSConfig, const byte * pcSource )
{

	byte cTemp[15] = {0};
	const byte * pcTemp = NULL;
	const byte * pcHead = NULL;
	byte * pcFormulaType = NULL;
	int iCmdSum = 0;
	int iLen = 0;
	uint32 u32Temp = 0;

	if( NULL != pstDSConfig->pcFormula )
		free( pstDSConfig->pcFormula );

	if( NULL != pstDSConfig->pStrFormat )
		free( pstDSConfig->pStrFormat );

	pcTemp = pcSource;
	pcHead = pcTemp;

	memcpy( cTemp, pcTemp, 4 );
	cTemp[4] = '\0';

	iLen = 4;

	iLen += strtol( cTemp, NULL, 16 );

	get_config_data( &u32Temp, pcTemp, iLen );

	pstDSConfig->iDSID = ( int )u32Temp; //获得DS的ID

	pcTemp += iLen;

	memcpy( cTemp, pcTemp, 4 );
	cTemp[4] = '\0';

	iLen = 4;

	iLen += strtol( cTemp, NULL, 16 );

	get_config_data( &u32Temp, pcTemp, iLen );

	pstDSConfig->iDSCmdID = ( int )u32Temp; //获得命令偏移

	pcTemp += iLen;

	iLen = get_command_one_block_config_data( cTemp, pcTemp );

	pstDSConfig->cValidByteOffset = cTemp[0]; //获得有效字节起始偏移

	pcTemp += iLen;

	iLen = get_command_one_block_config_data( cTemp, pcTemp );

	pstDSConfig->cValidByteNumber = cTemp[0]; //获得有效字节个数

	pcTemp += iLen;

	memcpy( cTemp, pcTemp, 4 );
	cTemp[4] = '\0';

	iLen = 4;

	iLen += strtol( cTemp, NULL, 16 );

	pcFormulaType = ( byte * )malloc( sizeof( byte ) * ( iLen - 4 + 1 ) );

	memcpy( pcFormulaType, pcTemp + 4, iLen - 4 ); //获得公式类型

	pcFormulaType[iLen - 4] = '\0';

	pstDSConfig->cFormulaType = FORMULA_PARSER;//默认为解析器处理

	if( 0 == strcmp( pcFormulaType, "FORMULA_PARSER" ) )
		pstDSConfig->cFormulaType = FORMULA_PARSER;

	if( 0 == strcmp( pcFormulaType, "PROTOCOL_PARSER" ) )
		pstDSConfig->cFormulaType = PROTOCOL_PARSER;

	free( pcFormulaType );
	pcTemp += iLen;

	memcpy( cTemp, pcTemp, 4 );
	cTemp[4] = '\0';

	iLen = 4;

	iLen += strtol( cTemp, NULL, 16 );

	pstDSConfig->pcFormula = ( byte * )malloc( sizeof( byte ) * ( iLen - 4 + 1 ) );

	memcpy( pstDSConfig->pcFormula, pcTemp + 4, iLen - 4 ); //获得公式内容
	pstDSConfig->iFormulaLen = iLen - 4;//获得公式内容的长度

	pstDSConfig->pcFormula[iLen - 4] = '\0';

	pcTemp += iLen;

	memcpy( cTemp, pcTemp, 4 );
	cTemp[4] = '\0';

	iLen = 4;

	iLen += strtol( cTemp, NULL, 16 );

	pstDSConfig->pStrFormat = ( byte * )malloc( sizeof( byte ) * ( iLen - 4 + 1 ) );

	memcpy( pstDSConfig->pStrFormat, pcTemp + 4, iLen - 4 ); //获得显示控制

	pstDSConfig->pStrFormat[iLen - 4] = '\0';

	pcTemp += iLen;

	return ( int )( pcTemp - pcHead );

}

/*************************************************
Description:	获取当前屏数据流配置数据
Input:
	pIn				输入内容

Output:	无
Return:	byte		当前屏数据流个数
Others: 输入内容为数据流总数+具体数据流配置。
注意这里g_stGeneralDSFormulaGroupConfig.cItemSum
保存的是最大值情况，而非每次的总数。
*************************************************/
byte get_current_screen_DS_config( void* pIn )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	byte cTemp	= 0;
	byte * pcTemp = NULL;

	byte j = 0;

	int iLen = 0;

	byte cItemSum = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	pcTemp = pstParam->pcData;

	iLen = get_command_one_block_config_data( &cTemp, pcTemp );
	cItemSum = cTemp;//获得当前屏数据流总数
	pcTemp += iLen;

	if( g_stGeneralDSFormulaGroupConfig.cItemSum < cItemSum ) //如果当前数据流条数小于需要的则重新分配
	{
		free_general_DS_formula_config_space();//释放已分配的

		g_stGeneralDSFormulaGroupConfig.cItemSum = cItemSum;

		g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig =
		    ( STRUCT_DS_FORMULA_CONFIG * )calloc( cItemSum, sizeof( STRUCT_DS_FORMULA_CONFIG ) );
	}

	for( j = 0; j < cItemSum; j++ )
	{
		iLen = get_ds_config( ( STRUCT_DS_FORMULA_CONFIG * ) & ( g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig[j] ), pcTemp );
		pcTemp += iLen;
	}

	return cItemSum;
}

/*************************************************
Description:	释放存放普通数据流公式配置的空间
Input:	无
Output:	保留
Return:	无
Others: 每添加一类配置就在该函数中添加相应的释放代码
		并在quit_system_lib.c的free_xml_config_space
		函数中调用该函数。
*************************************************/
void free_general_DS_formula_config_space( void )
{
	byte j = 0;

	if( NULL != g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig )
	{
		for( j = 0; j < g_stGeneralDSFormulaGroupConfig.cItemSum; j++ )
		{
			if( NULL != g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig[j].pcFormula )
				free( g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig[j].pcFormula );

			if( NULL != g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig[j].pStrFormat )
				free( g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig[j].pStrFormat );
		}

		free( g_stGeneralDSFormulaGroupConfig.pstDSFormulaConfig );
	}

}
/*************************************************
Description:	获取数据流缓存偏移
Input:	iDSCmdID 数据流命令ID
Output:	保留
Return:	byte	缓存偏移
Others:
*************************************************/
byte get_DS_buffer_offset( int iDSCmdID )
{
	int i = 0;

	for( i = 0; i < sizeof( s_stDSCmdIDAssociateBufferOffsetGroup ) / sizeof( s_stDSCmdIDAssociateBufferOffsetGroup[0] ); i++ )
		if( iDSCmdID == s_stDSCmdIDAssociateBufferOffsetGroup[i].iCmdID )
			return s_stDSCmdIDAssociateBufferOffsetGroup[i].cBufferOffset;

	return 255;
}






/*************************************************
Description:	发送多帧数据发送接收函数
Input:  piCmdIndex:  命令号1 + 命令号2 + ......
        CmdSum:  命令总个数
Output:	
Return:	bool
Others:
*************************************************/
bool process_Multi_frame_cmd_To_VDI_processing(int CmdSum,byte* piCmd,void* pOut)
{
	int iReceiveResult = TIME_OUT;
	bool bSendStatus = false;
	int piCmdIndex[300] = {0};
	int i = 0;
	byte	cRetransTime			= 0;

	cRetransTime = select_ResendTime_config();
	if (cRetransTime == 0xff)
	{
		//special_return_status(PROCESS_FAIL|NO_JUMP|HAVE_TIP,NULL,"Multi_frame_Resend_Time_False",NULL,0,ORIGINAL,NULL,pOut);
		general_load_tipID( "Multi_frame_Resend_Time_False", pOut );
		return false;
	}

	piCmdIndex[0] = CmdSum;
	for (i = 0 ; i < CmdSum; i++)
	{
		piCmdIndex[i+1] = piCmd[i];
	}
	while(1)
	{
		bSendStatus = process_DS_send_Cmd(piCmdIndex);
		if( !bSendStatus )
		{
			iReceiveResult =  FAILE;			
			general_return_status( iReceiveResult, NULL, 0, pOut );
			return false;
		}

		iReceiveResult = process_DS_receive_Cmd(piCmdIndex);
		if( ( iReceiveResult != SUCCESS ) && ( iReceiveResult != NEGATIVE ) ) //如果状态既不是SUCCESS又不是NEGATIVE则认为出错
		{
			if (( --cRetransTime ) == 0)
			{			
				general_return_status( iReceiveResult, NULL, 0, pOut );
				return false;
			} 
			else
			{
				continue;
			}			
		}
		else
		{
			break;
		}
	}
	return true;
}


