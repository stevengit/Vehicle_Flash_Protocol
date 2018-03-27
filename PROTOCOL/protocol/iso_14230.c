/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义根据ISO14230协议处理收发数据的相关函数
History:
	<author>	<time>		<desc>

************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "iso_14230.h"
//#include "sae_1939.h"
#include "iso_15765.h"
#include "../interface/protocol_interface.h"
#include "../public/protocol_config.h"
#include "../command/command.h"
#include "../public/public.h"
#include "../InitConfigFromXml/init_config_from_xml_lib.h"
#include "../function/idle_link_lib.h"

/*************************************************
Description:	根据ISO14230协议收发函数
Input:	piCmdIndex	命令索引地址
Output:	none
Return:	int	收发处理时的状态
Others:	根据命令索引可以发送和接收多条命令，
		把ECU积极相应的数据放到命令相应的缓存中
注意：这里存放ECU数据是从SID的回复开始的，如：
CMD:80 59 F1 01 81 4C RET:80 F1 59 03 C1 EA 8F 07
是从C1开始保存，若为消极相应则从7F开始保存。
*************************************************/
int send_and_receive_cmd_by_iso_14230( const int* piCmdIndex )
{
	int iStatus = TIME_OUT;
	int i = 0;
	int iCmdSum = piCmdIndex[0];
	byte cReceiveBuffer[256] = {0};
	STRUCT_CMD stSendCmd = {0};

	for( i = 0; i < iCmdSum; i++ )
	{
		iStatus = send_and_receive_single_cmd_by_iso_14230( ( STRUCT_CMD * )&stSendCmd, piCmdIndex[1 + i], cReceiveBuffer );

		if( NULL != stSendCmd.pcCmd )
			free( stSendCmd.pcCmd );

		if( ( iStatus != SUCCESS ) && ( iStatus != NEGATIVE ) ) //如果状态既不是SUCCESS又不是NEGATIVE则认为出错
		{
			break;
		}

		time_delay_ms( g_p_stISO14230Config->u16TimeBetweenFrames );
	}

	return iStatus;
}
/*************************************************
Description:	收发单条命令函数
Input:
	cCmdIndex		命令索引
	pcReceiveBuffer	存放ECU回复数据的缓存

Output:	none
Return:	int	收发处理时的状态
Others:	按照ISO14230协议处理
*************************************************/
int send_and_receive_single_cmd_by_iso_14230( STRUCT_CMD *pstSendCmd, const int cCmdIndex, byte* pcReceiveBuffer )
{
	int		iReceiveStaus = TIME_OUT;
	byte	cTimeBetweenBytes		= g_p_stISO14230Config->cTimeBetweenBytes;
	byte	cRetransTime			= g_p_stISO14230Config->cRetransTime;
	bool	bSendStatus = false;

	byte	cBufferOffset = 0;


	pstSendCmd->cBufferOffset	= g_stInitXmlGobalVariable.m_p_stCmdList[cCmdIndex].cBufferOffset;
	pstSendCmd->cReserved		= g_stInitXmlGobalVariable.m_p_stCmdList[cCmdIndex].cReserved;
	pstSendCmd->iCmdLen			= g_stInitXmlGobalVariable.m_p_stCmdList[cCmdIndex].iCmdLen;
	pstSendCmd->pcCmd			= ( byte* )malloc( sizeof( byte ) * pstSendCmd->iCmdLen );
	memcpy( pstSendCmd->pcCmd, g_stInitXmlGobalVariable.m_p_stCmdList[cCmdIndex].pcCmd, pstSendCmd->iCmdLen );

	cBufferOffset = pstSendCmd->cBufferOffset;
	pre_process_cmd( pstSendCmd ); //计算校验和

	while( 1 )
	{
		switch( g_p_stVCI_params_config->cCommunicationType )
		{
		case 0x00:
			bSendStatus = package_and_send_frame( FRAME_HEAD_NORMAL_FRAME, pstSendCmd, cTimeBetweenBytes );
			break;

		case 0x01:
		case 0x50:
		case 0x51:/* 按时间上传 */
			bSendStatus = package_and_send_frame_time( FRAME_HEAD_NORMAL_FRAME, pstSendCmd, cTimeBetweenBytes );
			break;

		default:
			return PROTOCOL_ERROR;
			break;
		}


		if( !bSendStatus )
		{
			return FAILE;
		}

		iReceiveStaus = process_KWP_receive_Cmd( cBufferOffset, pcReceiveBuffer );

		switch( iReceiveStaus )
		{

		case TIME_OUT:
		{
			if( ( --cRetransTime ) == 0 )
				return iReceiveStaus;
		}
		break;

		case SUCCESS:
		case FRAME_TIME_OUT:
		case NEGATIVE:

		default:
			return iReceiveStaus;
		}
	}
}


/*************************************************
Description:	处理标准KWP的接收
Input:
	cBufferOffset	存储缓存偏移
	pcSource		接收ECU回复数据的缓存

Output:	none
Return:	int	收发处理时的状态
Others:	按照ISO14230协议处理接收，这里
		cBufferOffset对应的是g_stBufferGroup
		而pcSource是调用receive_cmd函数时暂存
		数据的地址；
*************************************************/
int process_KWP_receive_Cmd( const byte cBufferOffset, byte* pcSource )
{
	uint16	u16Code7F78Timeout	= g_p_stISO14230Config->u16Code7F78Timeout;
	uint16	u16ECUResTimeout	= g_p_stISO14230Config->u16ECUResTimeout;

	byte*	pcDestination		= g_stBufferGroup[cBufferOffset].cBuffer;

	int		iNegativeResponseCounter = 0;
	int		iValidLen = 0;
	int 	iReceiveStatus	= FAILE;

	iReceiveStatus = process_KWP_receive_single_Cmd( &pcSource, &iValidLen, u16ECUResTimeout );

	if( iReceiveStatus != SUCCESS )
	{
		return iReceiveStatus;
	}

	if( ( iValidLen == 0x03 ) && ( pcSource[0] == 0x7f ) ) //判断消极相应
	{
		if( pcSource[2] == 0x78 )
		{
			while( 1 ) //这个while是用来处理等待多个7F78，目前最多等200个
			{
				iReceiveStatus = process_KWP_receive_single_Cmd( &pcSource, &iValidLen, u16Code7F78Timeout );

				if( iReceiveStatus != SUCCESS )
				{
					return iReceiveStatus;
				}

				if( ( iValidLen == 0x03 ) && ( pcSource[0] == 0x7f ) && ( pcSource[2] == 0x78 ) )
					iNegativeResponseCounter++;
				else if( ( iValidLen == 0x03 ) && ( pcSource[0] == 0x7f ) )
				{
					memcpy( pcDestination, &pcSource[0], 3 ); //保存消极响应内容
					g_stBufferGroup[cBufferOffset].iValidLen = 3;
					return NEGATIVE;
				}
				else
					break;

				if( iNegativeResponseCounter == 200 )
					return TIME_OUT;

			}//end while

		}//end if
		else
		{
			memcpy( pcDestination, &pcSource[0], 3 ); //保存消极响应内容
			g_stBufferGroup[cBufferOffset].iValidLen = 3;
			return NEGATIVE;
		}

	}//end if

	//保存有效数据到指定的缓存中
	memcpy( pcDestination, pcSource, iValidLen );
	//修改缓存的有效字节
	g_stBufferGroup[cBufferOffset].iValidLen = iValidLen;

	return SUCCESS;
}
/*************************************************
Description:	处理以KWP协议方式接收单条回复函数
Input:	u16FrameTimeout			帧超时时间

Output:	ppcSource	存放接收数据地址的指针的地址
		piValidLen	接收数据有效字节

Return:	int	接收ECU回复的状态
Others: 按字节上传 ，上传的就是有效数据
        按时间上传， 上传的第一个字节是有效数据的长度，以后跟着的是有效数据
*************************************************/
int process_KWP_receive_single_Cmd( byte** ppcSource, int* piValidLen, const uint16 u16FrameTimeout )
{
	bool	bReceiveStatus	= false;
	uint16	u16FrameContentTimeout = g_p_stISO14230Config->u16ECUResTimeout;
	byte TimeTotalFrameLength = 0, SingleFrameLength = 0;

	bReceiveStatus = ( bool )receive_cmd( *ppcSource, 1, u16FrameTimeout ); //接收第一个字节
	*piValidLen = **ppcSource;
	TimeTotalFrameLength = *piValidLen;

	if( !bReceiveStatus )
	{
		return TIME_OUT;
	}

	if( g_p_stGeneralActiveEcuConfig->cActiveMode == 0x06 ) /* 三菱1个字节 */
	{
		return SUCCESS;
	}

TimeWaitReceive:
	switch( g_p_stVCI_params_config->cCommunicationType ) //根据通用配置的模式
	{
	case 0x01:
	case 0x50:
	case 0x51:/* 按时间上传,先接收一个字节 */
	{
		bReceiveStatus = ( bool )receive_cmd( *ppcSource, 1, u16FrameTimeout ); //接收第一个字节
		if( !bReceiveStatus )
		{
			return TIME_OUT;
		}
	}
	break;

	default: /* 默认情况下是 单字节上传 */
		break;
	}

	switch( ( *ppcSource )[0] & 0xC0 )
	{
	case 0X00://没有地址信息型
	{
		*piValidLen = ( *ppcSource )[0];
		bReceiveStatus = ( bool )receive_cmd( *ppcSource, *piValidLen + 1, u16FrameContentTimeout );	
		SingleFrameLength = *piValidLen + 2;
	}
	break;

	case 0XC0:
	case 0X80:
	{
		if( ( ( *ppcSource )[0] != 0XC0 ) && ( ( *ppcSource )[0] != 0X80 ) ) //既不是0X80也不是0XC0
		{
			*piValidLen = ( ( *ppcSource )[0] & 0X3F );
			bReceiveStatus = ( bool )receive_cmd( *ppcSource, *piValidLen + 3, u16FrameContentTimeout );
			if( !bReceiveStatus ) //判断接收剩余部分时的状态
				return FRAME_TIME_OUT;			
			
			*ppcSource += 2;//跳过两个字节的地址	
			SingleFrameLength = *piValidLen + 4;
		}
		else
		{
			bReceiveStatus = ( bool )receive_cmd( *ppcSource, 2 + 1, u16FrameContentTimeout ); //接收两个字节地址和一个字节长度

			if( !bReceiveStatus )
				return FRAME_TIME_OUT;

			*piValidLen = ( *ppcSource )[2];

			bReceiveStatus = ( bool )receive_cmd( *ppcSource, *piValidLen + 1, u16FrameContentTimeout ); //根据长度字节接收
			if( !bReceiveStatus ) //判断接收剩余部分时的状态
				return FRAME_TIME_OUT;

			SingleFrameLength = *piValidLen + 5;
		}
	}
	break;

	case 0X40://CARB模式
	{
		bReceiveStatus = ( bool )receive_cmd( *ppcSource, 2, u16FrameContentTimeout ); //6B + ECU地址

		if( !bReceiveStatus )
		{
			return FRAME_TIME_OUT;
		}

		bReceiveStatus = ( bool )receive_cmd( *ppcSource, *piValidLen - 3, u16FrameContentTimeout ); //接收剩余字节
		if( !bReceiveStatus )
		{
			return FRAME_TIME_OUT;
		}

		*piValidLen -= 4;
	}
	break;

	default:
		return FORMAT_ERORR;
	}
	//处理按时间上传 速度过快，两帧合并为一帧上传的情况
	if(g_p_stVCI_params_config->cCommunicationType == 0x01  ||  
		 g_p_stVCI_params_config->cCommunicationType == 0x50 || 
		g_p_stVCI_params_config->cCommunicationType == 0x51)
	{
		if (TimeTotalFrameLength > SingleFrameLength)
		{
			if (( *ppcSource )[0] == 0x7F && ( *ppcSource )[2] == 0x78)
			{
				goto TimeWaitReceive;
			}
		}
	}

	return SUCCESS;
}
/*************************************************
Description:	命令预处理函数
Input:	pstSnedCmd	待处理的命令结构体指针
Output:	pstSnedCmd	该函数会把校验和放到命令
					具体数组的最后一个字节
Return:	void
Others:
*************************************************/
void pre_process_cmd( STRUCT_CMD* pstSnedCmd )
{
	pstSnedCmd->pcCmd[pstSnedCmd->iCmdLen - 1] = calculate_Checksum( pstSnedCmd->pcCmd, pstSnedCmd->iCmdLen - 1 );
}