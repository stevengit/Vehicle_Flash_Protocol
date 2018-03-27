/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义版本信息数据处理函数
History:
	<author>	<time>		<desc>

***********************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "infor_lib.h"
#include "../formula/formula_comply.h"
#include "../command/command.h"
#include <assert.h>
#include "../public/public.h"
#include "../InitConfigFromXml/init_config_from_xml_lib.h"
#include "../public/protocol_config.h"
#include "../formula_parse/interface.h"
#include "ds_lib.h"

STRUCT_SELECT_FUN stReadInformationFunGroup[] =
{
	{GENERAL_READ_INFORMATION, process_read_general_ECU_information},
};

/*************************************************
Description:	获取处理读取版本信息函数
Input:
	cType		配置类型
Output:	保留
Return:	pf_general_function 函数指针
Others:
*************************************************/
pf_general_function get_read_information_fun( byte cType )
{
	int i = 0;

	for( i = 0; i < sizeof( stReadInformationFunGroup ) / sizeof( stReadInformationFunGroup[0] ); i++ )
		if( cType == stReadInformationFunGroup[i].cType )
			return stReadInformationFunGroup[i].pFun;

	return 0;
}

/*************************************************
Description:	读版本信息
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
void process_read_ECU_information( void* pIn, void* pOut )
{
	pf_general_function pFun = NULL;

	pFun = get_read_information_fun( g_p_stProcessFunConfig->cInforFunOffset );

	assert( pFun );

	pFun( pIn, pOut );
}
/*************************************************
Description:	普通读版本信息
Input:	pIn		输入内容
Output:	pOut	输出数据地址
Return:	保留
Others:
1. 发送完一条处理后直接保存起来。
2. 

*************************************************/
void process_read_general_ECU_information( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_OUTPUT* pstInforOut = ( STRUCT_CHAIN_DATA_OUTPUT* )malloc( sizeof( STRUCT_CHAIN_DATA_OUTPUT ) );
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;

	int iReceiveResult = TIME_OUT;
	int iReceiveValidLen = 0;//接收到的有效字节长度

	STRUCT_INFORM* pstInformGroup = NULL;
	byte cItemSum = 0;//要显示项总数
	int iGroupID = 0;
	int *pCmdIndex = NULL;
	int SendCmd[] = {1,0};
	byte cBufferOffset = 0;
	int i = 0;
	int j = 0;
	byte cContentCache[128] = {0};
	bool bCommonFlag = false;

	STRUCT_CHAIN_DATA_NODE *phead  = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp1 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp2 = NULL;
	STRUCT_CHAIN_DATA_NODE *ptemp3 = NULL;

	int  iDisplayID = 0;
	byte cStartOffset = 0;
	byte cValidByteLen = 0;
	int  iInformCmdOffset = 0;
 
	byte cNegtiveDisplay[20] = {"Neg.$"};

	pstInforOut->cKeyByte[0] = 1;
	pstInforOut->cKeyByte[1] = 0;
	pstInforOut->cKeyByte[2] = 0;
	pstInforOut->stJump.cLenHighByte = 0;
	pstInforOut->stJump.cLenLowByte = 0;
	pstInforOut->stTip.cLenHighByte = 0;
	pstInforOut->stTip.cLenLowByte = 0;



	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iGroupID = atoi( pstParam->pcData );
	cItemSum = g_p_stInformationGroupConfigGroup[iGroupID]->cItemSum;

	pCmdIndex = ( int * )malloc( sizeof( int ) * cItemSum);
	for (i = 0; i < cItemSum; i++)
	{
		pCmdIndex[i] = g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].iDSCmdID;
	}
	
	for( i = 0; i < cItemSum; i++ ) //
	{
		SendCmd[1] = g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].iDSCmdID;
		for (j = 0; j < i && j != cItemSum && i != 0; j++) //判断此次发送的命令是否已经发送
		{
			if (SendCmd[1] == pCmdIndex[j])
			{
				bCommonFlag = true;
				break;
			}							
		}
		
		if (!bCommonFlag)
		{
			iReceiveResult = send_and_receive_cmd(SendCmd);
			if (iReceiveResult != SUCCESS && iReceiveResult != NEGATIVE)
			{
				if (pstInforOut != NULL)
				{
					free(pstInforOut);
					pstInforOut = NULL;
				}

				if(pCmdIndex != NULL)
				{
					free(pCmdIndex);
					pCmdIndex = NULL;
				}

				general_return_status( iReceiveResult, NULL, 0, pOut );
				return;
			}
		}
		else
		{
			bCommonFlag = false;
		}	
		cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[SendCmd[1]].cBufferOffset;
		iDisplayID			= g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].iDSID;
		cStartOffset		= g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].cValidByteOffset;
		cValidByteLen		= g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].cValidByteNumber;
		if (cValidByteLen == 0xff)//支持ECU信息按帧长显示
		{
			cValidByteLen = g_stBufferGroup[cBufferOffset].iValidLen - cStartOffset;
		}

		ptemp2 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) );
		//一个字节对应的字符长度为2
		ptemp2->cLenHighByte = 0;
		ptemp2->cLenLowByte = 2;
		ptemp2->pcData = ( byte * )malloc( ( 2 + 1 ) * sizeof( byte ) );

		/*sprintf_s( ptemp2->pcData, ( 2 + 1 ), "%02d", cDisplayID ); //整型打印数据流ID*/
		sprintf( ptemp2->pcData, "%02d", iDisplayID ); //整型打印数据流ID
		//这里存放把数据转换成ASCII,HEX,DEC后的数据，如有消极相应则放消极响应内容
		ptemp3 = ( STRUCT_CHAIN_DATA_NODE * )malloc( sizeof( STRUCT_CHAIN_DATA_NODE ) );

		memset( cContentCache, 0, sizeof( cContentCache ) );

		//判断是否为消极响应
		if( g_stBufferGroup[cBufferOffset].iValidLen == 3 && g_stBufferGroup[cBufferOffset].cBuffer[0] == 0x7f )
		{
			for( j = 0; j < 3; j++ )
			{
				sprintf( &( cNegtiveDisplay[j * 2 + 5] ), "%02X", g_stBufferGroup[cBufferOffset].cBuffer[j] );
			}

			cNegtiveDisplay[11] = '\0';

			ptemp3->cLenHighByte = 0;
			ptemp3->cLenLowByte = ( byte )strlen( cNegtiveDisplay );
			ptemp3->pcData = ( byte * )malloc( strlen( cNegtiveDisplay ) * sizeof( byte ) );
			memcpy( ptemp3->pcData, cNegtiveDisplay, strlen( cNegtiveDisplay ) );
		}
		else
		{
			if( PROTOCOL_PARSER == g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].cFormulaType)
			{
				//调用protocol的计算公式
				process_normal_infor_calculate(
					g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].iDSID,
					g_stBufferGroup[cBufferOffset].cBuffer,
					cContentCache
					);
			}
			else
			{
				//调用公式解析器
				calculate(	g_stBufferGroup[cBufferOffset].cBuffer + cStartOffset,
					cValidByteLen,
					g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].pcFormula,
					g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].iFormulaLen,
					g_p_stInformationGroupConfigGroup[iGroupID]->pstDSFormulaConfig[i].pStrFormat,
					cContentCache
					);			 
			}
			
			ptemp3->cLenHighByte = 0;
			ptemp3->cLenLowByte = ( byte )strlen( cContentCache );
			ptemp3->pcData = ( byte * )malloc( strlen( cContentCache ) * sizeof( byte ) );
			memcpy( ptemp3->pcData, cContentCache, strlen( cContentCache ) );
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
pstInforOut->pstData = phead;

copy_data_to_out( pOut, pstInforOut );
free_param_out_data_space( pstInforOut );

free( pCmdIndex );
free( pstInforOut );
}