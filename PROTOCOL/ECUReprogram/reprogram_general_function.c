/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义特殊功能处理函数和通用的相关函数
History:
	<author>	<time>		<desc>

************************************************************************/
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "reprogram_function.h"
#include "..\InitConfigFromXml\init_config_from_xml_lib.h"
#include "..\interface\protocol_interface.h"
#include "..\function\idle_link_lib.h"
#include "..\SpecialFunction\special_function.h"
#include <time.h>

/*************************************************
Description:	重新激活系统
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
bool enter_system(void)
{
	byte cReceiveTemp[20] = {0}; //接收缓存
	bool bSendStatus = false;
	byte SystemCongig[] = {0x80,0x00,0x1a,0x80,0x01,0x00,0x00,0x06,0x0e,0x00,0x07,0xa1,0x20,0x01,0xc2,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x07,0xe8,0xaa}; //系统设置
	byte FcCmdCongig[]  = {0x91,0x00,0x11,0x04,0x00,0x10,0x07,0xe0,0x30,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0xd5}; //30帧设置
	byte EnterCmd[]     = {0xe0,0x00,0x0f,0xaa,0x07,0xe0,0x02,0x10,0x03,0x00,0x00,0x00,0x00,0x00,0x95};
	int ipCmdIndex[2] = {1, 0};
	byte cBufferOffset = 0;//缓存偏移
	int iReceiveResult = TIME_OUT;

	//发送BCM系统设置
	send_cmd( SystemCongig, SystemCongig[2] );

	if( ( bool )receive_all_cmd( cReceiveTemp, 5, 3000 ) )
	{
		if( cReceiveTemp[3] == 0x00 )
		{
			bSendStatus = true;
		}
	}

	time_delay_ms(30);

	//发送BCM30帧设置
	send_cmd( FcCmdCongig, FcCmdCongig[2] );

	if( ( bool )receive_all_cmd( cReceiveTemp, 5, 3000 ) )
	{
		if( cReceiveTemp[3] == 0x00 )
		{
			bSendStatus = true;
		}
	}

	time_delay_ms(30);

	set_idle_link(1);	//开始发送空闲

	time_delay_ms(30);

	//发送BCM进入命令
	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[2].cBufferOffset;
	ipCmdIndex[1] = 2;

	iReceiveResult = send_and_receive_cmd( ipCmdIndex );

	if( iReceiveResult != SUCCESS )
	{
		return false;
	}

	return true;
}
/*************************************************
Description:	ASCII转换为HEX
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
byte asc_to_hex(byte FileData)
{
	if ((FileData >= '0')&&(FileData <='9'))
	{
		FileData -= 0x30;
	}
	else if((FileData >= 'A')&&(FileData <='F'))
	{
		FileData -= 0x37;
	}
	else if((FileData >= 'a')&&(FileData <='f'))
	{
		FileData -= 0x57;
	}

	return FileData;
}
/*************************************************
Description:	ASCII转换为HEX
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
byte hex_to_asc(byte FileData)
{
	if ((FileData >= 0x00)&&(FileData <=0x09))
	{
		FileData += 0x30;
	}
	else if((FileData >= 0x0A)&&(FileData <=0x0F))
	{
		FileData += 0x37;
	}
	else if((FileData >= 0x0a)&&(FileData <=0x0f))
	{
		FileData += 0x57;
	}

	return FileData;
}
/*************************************************
Description:	两个字节合并为一个
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
byte merge_two_byte(byte FileData1,byte FileData2)
{
	byte FileData;

	FileData = (asc_to_hex(FileData1) << 4)| asc_to_hex(FileData2);

	return FileData;
}
/*************************************************
Description:	两个字节合并为一个，并进行加密处理
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
byte merge_two_byte_and_secret(byte *FileData1,byte *FileData2)
{
	byte FileData;

	_Encryption(FileData1,1);	//数据加密处理
	_Encryption(FileData2,1);	//数据加密处理

	FileData = (asc_to_hex(*FileData1) << 4)| asc_to_hex(*FileData2);

	return FileData;
}

/*************************************************
Description:	停止给VDI传输数据
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
byte send_cmd_checksum(byte *buffer,int len)
{
	int i;

	buffer[len -1] = 0;

	for( i = 0; i < len - 1; i++ )
	{
		buffer[len -1] += buffer[i];
	}

	return true;
}
/*************************************************
Description:	给VDI发送连接方式，USB或蓝牙
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_send_link_mode(void* pOut)
{
	byte cReceiveTemp[20] = {0}; //接收缓存
	int	iReceiveStatus	= FAILE;
	byte cCheckNum = 0;
	int CmdLen = 0;
	byte VDILinkMode[] = {0xC7,0x00,0x07,0x01,0x01,0x01,0x00};
	
	VDILinkMode[5] = g_VDILinkStatus;
	
	CmdLen = VDILinkMode[1]<<8|VDILinkMode[2];
	send_cmd_checksum(VDILinkMode ,CmdLen);

	send_cmd( VDILinkMode, CmdLen );

	//接收命令的确认帧
	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveTemp, g_p_stISO15765Config->u16ECUResTimeout );
	
	if (iReceiveStatus != SUCCESS)
	{
		special_return_status( PROCESS_OK | NO_JUMP | HAVE_TIP, NULL, "ID_STR_ECU_REPROGRAM_FAIL", 0, pOut );
		return iReceiveStatus;
	}

	special_return_status( PROCESS_OK | HAVE_JUMP | NO_TIP, "test_success", NULL, 0, pOut );

	return iReceiveStatus;
}
/*************************************************
Description:	开始刷写指令
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
// int repro_start_cmd(void)
// {
// 	byte cReceiveTemp[20] = {0}; //接收缓存
// 	int	iReceiveStatus	= FAILE;
// 	byte cCheckNum = 0;
// 	byte VDIStartCmd[] = {0xC7,0x00,0x07,0x01,0x02,0x01,0x00};
// 	int CmdLen = 0;
// 
// 	CmdLen = VDIStartCmd[1]<<8|VDIStartCmd[2];
// 	send_cmd_checksum(VDIStartCmd ,CmdLen);
// 
// 	send_cmd( VDIStartCmd, VDIStartCmd[2] );
// 
// 	//接收命令的确认帧
// 	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveTemp, g_p_stISO15765Config->u16ECUResTimeout );
// 
// 	if (iReceiveStatus != SUCCESS)
// 	{
// 		return iReceiveStatus;
// 	}
// 	
// 	g_VDIUploadSchedule = true;//VDI上传刷写进度，蓝牙通讯时用
// 	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveTemp, g_p_stISO15765Config->u16ECUResTimeout );
// 	g_VDIUploadSchedule = false;//VDI上传刷写进度，蓝牙通讯时用
// 	return iReceiveStatus;
// }

/*************************************************
Description:	发送校验和、命令总数
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_checksum_and_cmd_sum(byte CurrentBlock)
{
	byte cReceiveTemp[20] = {0}; //接收缓存
	bool bSendStatus = false;
	byte cCheckNum = 0;
	byte VDICmdChecksum[] = {0xC7,0x00,0x0E,0x01,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	int CmdLen = 0;
	int	iReceiveStatus	= FAILE;

	VDICmdChecksum[5] = g_ProgramData[CurrentBlock].AllDataChecksum.u8Bit[3];
	VDICmdChecksum[6] = g_ProgramData[CurrentBlock].AllDataChecksum.u8Bit[2];
	VDICmdChecksum[7] = g_ProgramData[CurrentBlock].AllDataChecksum.u8Bit[1];
	VDICmdChecksum[8] = g_ProgramData[CurrentBlock].AllDataChecksum.u8Bit[0];

	VDICmdChecksum[9]  = g_ProgramData[CurrentBlock].FrameTotalNumber.u8Bit[3];
	VDICmdChecksum[10] = g_ProgramData[CurrentBlock].FrameTotalNumber.u8Bit[2];
	VDICmdChecksum[11] = g_ProgramData[CurrentBlock].FrameTotalNumber.u8Bit[1];
	VDICmdChecksum[12] = g_ProgramData[CurrentBlock].FrameTotalNumber.u8Bit[0];

	CmdLen = VDICmdChecksum[1]<<8|VDICmdChecksum[2];
	send_cmd_checksum(VDICmdChecksum ,CmdLen);

	send_cmd( VDICmdChecksum, CmdLen );

	time_delay_ms(500);
	//接收命令的确认帧
	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveTemp, g_p_stISO15765Config->u16ECUResTimeout );
	if (iReceiveStatus != SUCCESS)
	{
		return iReceiveStatus;
	}

	return iReceiveStatus;
}
/*************************************************
Description:	开始给VDI传输数据
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_start_transfer_data(void)
{
	byte cReceiveTemp[20] = {0}; //接收缓存
	int	iReceiveStatus	= FAILE;
	byte cCheckNum = 0;
	byte VDIStartCmd[] = {0xC7,0x00,0x07,0x01,0x03,0x01,0x00};
	int CmdLen = 0;

	CmdLen = VDIStartCmd[1]<<8|VDIStartCmd[2];
	send_cmd_checksum(VDIStartCmd ,CmdLen);
	send_cmd( VDIStartCmd, CmdLen );

	//接收命令的确认帧
	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveTemp, g_p_stISO15765Config->u16ECUResTimeout );

	return iReceiveStatus;
}

/*************************************************
Description:	停止给VDI传输数据
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_stop_transfer_data(void)
{
	byte cReceiveTemp[20] = {0}; //接收缓存
	int	iReceiveStatus	= FAILE;
	byte cCheckNum = 0;
	byte VDIStopCmd[] = {0xC7,0x00,0x07,0x01,0x03,0x02,0x00};
	int CmdLen = 0;

	CmdLen = VDIStopCmd[1]<<8|VDIStopCmd[2];
	send_cmd_checksum(VDIStopCmd ,CmdLen);

	send_cmd( VDIStopCmd, VDIStopCmd[2] );

	//接收命令的确认帧
	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveTemp, g_p_stISO15765Config->u16ECUResTimeout );

	return iReceiveStatus;
}

/*************************************************
Description:	多帧发送时异常处理设置
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_config_negative_response(byte NegativeControlStatus)
{
	byte cReceiveTemp[20] = {0}; //接收缓存
	int	iReceiveStatus	= FAILE;
	byte cCheckNum = 0;
	int CmdLen = 0;
	byte ConfigNegative[] = {0xC6,0x00,0x23,0x01,0x01,0x02,0x02,0x7F,0x02,0x78,0x04,0x00,0x00,0x00,0x00,0x01,0x00,0x10,0x27,0x10,0x01,0x7F,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x38};
	
	ConfigNegative[4] = NegativeControlStatus;
	
	CmdLen = ConfigNegative[1]<<8|ConfigNegative[2];
	send_cmd_checksum(ConfigNegative ,CmdLen);

	send_cmd( ConfigNegative, CmdLen );

	//接收命令的确认帧
	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveTemp, g_p_stISO15765Config->u16ECUResTimeout );

	return iReceiveStatus;
}
/*************************************************
Description:刷写插帧命令设置
Input:
pIn		输入与ECU刷写有关的命令数据
和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_config_insert_Framing_response(byte NegativeControlStatus)
{
	byte cReceiveTemp[20] = {0}; //接收缓存
	int	iReceiveStatus	= FAILE;
	byte cCheckNum = 0;
	int CmdLen = 0;
	byte ConfigNegative[] = {0x90,0x00,0x1b,0x02,0x01,0xff,0x01,0x00,0x07,0xD0,0x00,0x00,0x00,0x00,0x01,0x0a,0x07,0xdf,0x02,0x3e,0x80,0x00,0x00,0x00,0x00,0x00,0x3c};

	ConfigNegative[6] = NegativeControlStatus;
	CmdLen = ConfigNegative[1]<<8|ConfigNegative[2];
	send_cmd_checksum(ConfigNegative ,CmdLen);

	send_cmd( ConfigNegative, CmdLen );
	////接收命令的确认帧
	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveTemp, g_p_stISO15765Config->u16ECUResTimeout );
	return iReceiveStatus;
}
/*************************************************
Description:	当前操作的是刷写文件的哪一块
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_config_current_file_block(byte CurrentFileBlock)
{
	byte	cReceiveCache[20] = {0}; //接收缓存
	int		iReceiveStatus	= FAILE;
	byte	cCheckNum = 0;
	int		CmdLen = 0;
	byte	ConfigFileBlock[] = {0xC7,0x00,0x07,0x01,0x06,0x00,0x00};

	ConfigFileBlock[5] = CurrentFileBlock;
	
	CmdLen = ConfigFileBlock[1]<<8|ConfigFileBlock[2];
	send_cmd_checksum(ConfigFileBlock ,CmdLen);

	send_cmd( ConfigFileBlock, CmdLen );

	//接收命令的确认帧
	iReceiveStatus = process_CAN_receive_single_Cmd( cReceiveCache, g_p_stISO15765Config->u16ECUResTimeout );

	return iReceiveStatus;
}
/*************************************************
Description:从文件中取出刷写文件的版本号
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/

int accept_file_ECU_version(void* pOut)
{
	byte ECUVersionOriginal[10];
	byte CompareEcuVersionResult = 0;

	if(g_ProgramData[0].cDataFlag)
	{
		memcpy( ECUVersionOriginal, g_ProgramData[0].pcData+36, 8 );

		//比较平台号	P903V762  P1074752
		CompareEcuVersionResult = memcmp(ECUVersionOriginal,ECUVersionReceive,5);

		if (CompareEcuVersionResult)	//平台号不一致，返回FAILE
		{
			special_return_status( PROCESS_FAIL | NO_JUMP | HAVE_TIP, NULL, "ID_STR_REPROGRAM_FILE_ECU_VERSION", 0, pOut );
			return FAILE;
		}
		else
		{
			//比较版本号
			CompareEcuVersionResult = memcmp(ECUVersionOriginal+5,ECUVersionReceive+5,3);

			if (CompareEcuVersionResult)
			{
				g_ChooseReprogramBlock = 0;//不相同，三部分全刷
			}
			else
			{
				g_ChooseReprogramBlock = 2;//相同，只刷写第三部分
			}

			//客户选择了强制刷写，则刷写三部分
			if (u8CalibrationFunctionJudge[0] == 1)
			{
				g_ChooseReprogramBlock = 0;
			}
		}
	}
	return SUCCESS;
}

/*************************************************
Description:解密HEX文件
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int switch_Secrethex_Hex(byte *FilePath)  //将加密文件转换成原始文件
{
	int j = 0;
	int bStatus = FAILE;
	FILE *FilePointer = NULL;
	FILE *FilePointer1 = NULL;
	int AddressLen = 0;
	char HeadFrameData[255] = {0};//一行数据
	uint32 temp = 0;
	int Num =0;
	int nCount = 0;
	/*打开文件*/
	FilePointer = fopen( FilePath, "r" );
	AddressLen = (int)strlen(FilePath);
	if (NULL == FilePointer)
	{
		free( FilePath );
		return FAILE;
	}
	g_ReprogramFilePath1 =( byte* )malloc(AddressLen + 1) ;

	for (j=0;;j++)
	{
		if (FilePath[j] == '.')
		{
			FilePath[j-1] += 1 ;
			break;
		}
	}
	memcpy(g_ReprogramFilePath1,FilePath,AddressLen);
	g_ReprogramFilePath1[AddressLen] = '\0';
	FilePointer1 = fopen(g_ReprogramFilePath1,"w");
	while(!( feof(FilePointer) ))
	{
		fgets(HeadFrameData,255,FilePointer);
		Num = (int)strlen(HeadFrameData);
		if (temp != 0)
		{
			Encryption(HeadFrameData,Num,g_Secretkey,(int)strlen(g_Secretkey),nCount); //解密函数
			fputs(HeadFrameData,FilePointer1);
			fflush(FilePointer1);   //清除缓冲区
		}
		temp ++;
	}
	if (feof(FilePointer))
	{
		fclose(FilePointer1);
		FilePointer1 = NULL;
		fclose(FilePointer);
		FilePointer = NULL;
	}
	bStatus = repro_make_file_data_HEX(NULL ,g_ReprogramFilePath1);
	remove(g_ReprogramFilePath1);
	if(NULL != g_ReprogramFilePath1)
		free(g_ReprogramFilePath1 );
	return bStatus;
}
//解密函数
void Encryption(char* vDataBuffer,const int nSize,const char* pSeed,const int nSeedLen,int nCount)
{
	char cTmp;
	int i =0;
	for (i = 0 ; i < nSize ; i++)
	{
		if (vDataBuffer[i] == '\r' || vDataBuffer[i] == '\n')
		{
			nCount = 0;
			continue;
		}
		cTmp = vDataBuffer[i] ^  *(pSeed + nCount % nSeedLen);
		if ( cTmp == '\r' || cTmp == '\n' || cTmp == '\0')
			vDataBuffer[i] = vDataBuffer[i];
		else
			vDataBuffer[i] = vDataBuffer[i] ^  *(pSeed + nCount % nSeedLen);

		nCount++;
	}
}

/*************************************************
Description:处理刷写文件数据---HEX    判断是否是加密文件
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int judge_file_secret_hex(void* pOut, byte *Filepath)
{
	byte i = 0;
	int bStatus = FAILE;
	char HeadFrameData[250] = {0};//一行数据
	char cData[5] = {0};
	/*打开文件*/
	FILE *FilePointer = NULL;
	FilePointer = fopen( Filepath, "r" );
	if (NULL == FilePointer)
	{
		free( Filepath );
		return FAILE;
	}
	for (i = 0; i < 5; i++)
	{
		fgets(HeadFrameData,250,FilePointer);
		if(HeadFrameData[0] == ':')
		{
			fclose(FilePointer);
			FilePointer = NULL;
			bStatus = repro_make_file_data_HEX(pOut, Filepath);//取前五行，如果行首以':'开头证明是原始文件
			return bStatus;
		}
	}

	fclose(FilePointer);
	FilePointer = NULL;
	bStatus = switch_Secrethex_Hex(pOut);//如果前5行没有':'证明是加密文件
	return bStatus;
}
/*************************************************
Description:处理刷写文件数据---HEX
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_make_file_data_HEX(void* pOut, byte *Filepath)
{
	int j = 0;
	int i = 0;
	FILE *FilePointer = NULL;
	char HeadFrameData[10] = {0};//一行数据
	byte DataType = 0; 
	byte DataLen = 0;
	byte CurrentBlock = 0;
	UNN_2WORD_4BYTE unnBaseAddress = {0};	//刷写文件的基地址
	byte *ValueData = NULL;
	int  CurrentDataCount;

	/*打开文件*/
	FilePointer = fopen( Filepath, "r" );
	if (NULL == FilePointer)
	{
		free( Filepath );
		special_return_status( PROCESS_FAIL | NO_JUMP | HAVE_TIP, NULL, "ID_STR_OPEN_FILE_FAIL", 0, pOut );
		return FAILE;
	}

	while (1)
	{
		fgets(HeadFrameData,10,FilePointer);

		DataType = merge_two_byte(HeadFrameData[7],HeadFrameData[8]);
		DataLen = merge_two_byte(HeadFrameData[1],HeadFrameData[2]);

		ValueData = ( byte * )malloc( sizeof( byte ) *(DataLen*2+5)  );

		fgets(ValueData,DataLen*2 + 5, FilePointer);

		switch (DataType)  //数据类型
		{
		case 0x02:
		case 0x04:	//基地址
			unnBaseAddress.u8Bit[3] = merge_two_byte(ValueData[0],ValueData[1]);
			unnBaseAddress.u8Bit[2] = merge_two_byte(ValueData[2],ValueData[3]);			
			break;
		case 0x00:	//数据
				unnBaseAddress.u8Bit[1] = merge_two_byte(HeadFrameData[3],HeadFrameData[4]); 
				unnBaseAddress.u8Bit[0] = merge_two_byte(HeadFrameData[5],HeadFrameData[6]);

				if ( unnBaseAddress.u32Bit == 0x8000BF80  )
				{
					for ( i=0,j=0; i<19,j <38; j+=2,i++ )
					{
						g_LogisticID[i] = merge_two_byte( ValueData[2+j],ValueData[3+j] );

					}
				}

	 			for (CurrentBlock=g_ChooseReprogramBlock; CurrentBlock<REPROGRAM_FILE_BLOCKS_NUMBER; CurrentBlock++)
	 			{
	 				if (unnBaseAddress.u32Bit == g_ProgramData[CurrentBlock].cBeginAddress.u32Bit)
	 				{
	 					g_ProgramData[CurrentBlock].cDataFlag = true;
	 				}
	 			}
				for (CurrentBlock=0; CurrentBlock<REPROGRAM_FILE_BLOCKS_NUMBER; CurrentBlock++)
				{
					if (g_ProgramData[CurrentBlock].cDataFlag)
					{
						CurrentDataCount = ReceiveFileByteNumber;	//记录当前操作的数据位置，存储文件中版本号时使用

						for (j=0; j<DataLen*2;)
						{
							g_ProgramData[CurrentBlock].pcData[ReceiveFileByteNumber++] = merge_two_byte(ValueData[0+j],ValueData[1+j]);
							j+=2;

							if (g_ProgramData[CurrentBlock].cValueDataLen.u32Bit == ReceiveFileByteNumber)	//是否把这块数据存完
							{
								if ( CurrentBlock == REPROGRAM_FILE_BLOCKS_NUMBER )
								{
									if (accept_file_ECU_version(pOut) != SUCCESS)	//获取刷写文件中的版本号，并于读取到的ECU版本号进行比较
									{
										return FAILE;
									}
								}

								g_ProgramData[CurrentBlock].cDataFlag = false;
								ReceiveFileByteNumber = 0;
								break;
							}
						}
					}
				}
			break;

		case 0x01:	//文件结束
			fclose(FilePointer);
			FilePointer = NULL;

			if (ValueData != NULL)
			{
				free(ValueData);
				ValueData = NULL;
			}
			return SUCCESS;

		default:
			break;
		}
	}
	return SUCCESS;
}

/*************************************************
Description:处理刷写文件数据---启明仪器EOL刷写文件
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_make_file_data_eol(void* pOut)
{
	FILE *FilePointer = NULL;
	char HeadFrameData[10] = {0};//一行数据
	byte DataType = 0; 
	byte DataLen = 0;
	byte CurrentBlock = 0;
	UNN_2WORD_4BYTE unnBaseAddress = {0};	//刷写文件的基地址
	byte *ValueData = NULL;
	int i = 0;
	uint32 m = 0;

	/*打开文件*/
	FilePointer = fopen( g_ReprogramFilePath, "rb" );
	if (NULL == FilePointer)
	{
		free( g_ReprogramFilePath );
		special_return_status( PROCESS_FAIL | NO_JUMP | HAVE_TIP, NULL, "ID_STR_OPEN_FILE_FAIL", 0, pOut );
		return FAILE;
	}

	fgets(HeadFrameData,7,FilePointer);	//获取8个字节地址

	for (i=0; i<REPROGRAM_FILE_BLOCKS_NUMBER; i++)
	{		
		fgets(HeadFrameData,9,FilePointer);	//获取8个字节地址

		if (0 == i)
		{
			CurrentBlock = 0;
		}
		else if (1 == i)
		{
			CurrentBlock = 2;
		}
		else if (2 == i)
		{
			CurrentBlock = 1;
		}

		for (m=0; m<g_ProgramData[CurrentBlock].cValueDataLen.u32Bit; m++)
		{
			g_ProgramData[CurrentBlock].pcData[m] = fgetc(FilePointer);
		}

		if (CurrentBlock == 0)
		{
			g_ProgramData[0].cDataFlag = true;
		}
		if (accept_file_ECU_version(pOut) != SUCCESS)	//获取刷写文件中的版本号，并于读取到的ECU版本号进行比较
		{
			return FAILE;
		}
		g_ProgramData[0].cDataFlag = false;
	}

	fclose(FilePointer);
	FilePointer = NULL;

	return SUCCESS;
}
/*************************************************
Description:处理刷写文件数据---rpm
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_initialize_rpm_file(void* pOut)
{
	int i = 0;
	int j = 0;
	FILE *FilePointer = NULL;
	char FrameHeadData[10] = {0};//帧头数据
	byte CurrentBlock = 0;
	UNN_2WORD_4BYTE unnBaseAddress = {0};	//刷写文件的基地址
	byte FileData1 = 0;//文件内容
	byte FileData2 = 0;//文件内容
	byte FrameHead = 0;//帧头
	byte FrameLenHigh = 0;//帧长，高位
	byte FrameLenLow = 0;//帧长，低位
	int FrameLen = 0;
	byte ECUVersionFile[10];
	byte Len = 0;
	byte temp[10];
	byte CompareEcuVersionResult = 0;

	/*打开文件*/
	FilePointer = fopen( g_ReprogramFilePath, "r" );
	if (NULL == FilePointer)
	{
		free( g_ReprogramFilePath );
		special_return_status( PROCESS_FAIL | NO_JUMP | HAVE_TIP, NULL, "ID_STR_OPEN_FILE_FAIL", 0, pOut );
		return FAILE;
	}

	while (1)
	{
		if (fgetc(FilePointer) == 'S')
		{
			switch (fgetc(FilePointer))
			{
			case '0':	//文件信息
				fgets(temp,3,FilePointer);	//取个数
				Len = merge_two_byte(temp[0],temp[1]);
				fgets(temp,Len+1,FilePointer);	//取个数
			
				fgets(temp,3,FilePointer);	//取个数
				Len = merge_two_byte(temp[0],temp[1]);
				fgets(ECUVersionFile,Len+1,FilePointer);	//取文件中的ECU版本号
				
				CompareEcuVersionResult = memcmp(ECUVersionFile,ECUVersionReceive,8);

				if (CompareEcuVersionResult)	//不相同，提示文件与ECU版本不一致，请重新选择正确的文件！
				{
					special_return_status( PROCESS_FAIL | NO_JUMP | HAVE_TIP, NULL, "ID_STR_REPROGRAM_FILE_ECU_VERSION", 0, pOut );
					return FAILE;
				}

				break;
			case '1':	//数据信息
				fgets(FrameHeadData,9,FilePointer);	//取出一帧命令的前几个字节，获取块数

				CurrentBlock = merge_two_byte_and_secret(FrameHeadData+0,FrameHeadData+1);

				g_ProgramData[CurrentBlock].FrameTotalNumber.u32Bit++;	//统计每一块数据的总帧数
				break;
			default:
				break;
			}
		}

		if( feof( FilePointer ) )	//文件取完
		{
			fclose(FilePointer);
			FilePointer = NULL;	

			return SUCCESS;
		}
	}
}
/*************************************************
Description:	打包命令帧
Input:
	cFrameHead		命令帧头
	pstFrameContent	具体命令结构体指针
	cReservedByte	命令中保留字节

Output:	none
Return:	bool	返回发送状态（成功、失败）
Others:	该函数会尝试发送三次，根据收发装置
		回复的内容判断发送状态；
*************************************************/
int package_frame( const byte cFrameHead, STRUCT_CMD* pstFrameContent, const byte cReservedByte )
{
	UNN_2WORD_4BYTE uFrameLen;
	byte cCheckNum = 0;
	int i = 0;

	uFrameLen.u32Bit = 1 + 2 + 1 + pstFrameContent->iCmdLen + 1;

	pcPackageCmdCache = ( byte * )malloc( ( uFrameLen.u32Bit ) * sizeof( byte ) ); //发送帧的缓存

	pcPackageCmdCache[0] = cFrameHead;
	pcPackageCmdCache[1] = uFrameLen.u8Bit[1];
	pcPackageCmdCache[2] = uFrameLen.u8Bit[0];
	pcPackageCmdCache[3] = cReservedByte;

	memcpy( &pcPackageCmdCache[4], pstFrameContent->pcCmd, pstFrameContent->iCmdLen );

	for( i = 0; ( i < ( int )uFrameLen.u32Bit - 1 ); i++ )
	{
		cCheckNum += pcPackageCmdCache[i];
	}

	pcPackageCmdCache[ uFrameLen.u32Bit - 1] = cCheckNum;

	return SUCCESS;
}
/*************************************************
Description:	将数据按照上下位机格式进行打包
Input:
	cCmdIndex		命令索引
	pcReceiveBuffer	存放ECU回复数据的缓存

Output:	none
Return:	int	收发处理时的状态
Others:	按照ISO15765协议处理
*************************************************/
int reprogram_make_cmd_by_iso_15765( STRUCT_CMD *pstSendCmd, byte* pcReceiveBuffer )
{
	int		i = 0;
	int		j = 0;
	int		iReceiveStatus = FAILE;
	byte	cIso15765ReservedByte	= g_p_stISO15765Config->cReserved;
	uint16	u16ECUResTimeout		= g_p_stISO15765Config->u16ECUResTimeout;
	bool	bSendStatus = false;
	int		iCmdLen = 0;
	bool	bMultiFrame = false;//多帧标志
	int 	iReceiveFCStatus = FAILE;//等待FC帧标志
	int	cFrameSum = 0;
	STRUCT_CMD stCopySendCmd = {0};
	byte	FrameTypeOffset = 0;
	byte	cBufferOffset = 0;
	const byte ExtendCAN = 2;
	const byte StandardCAN = 0;
	int LastBytesLen = 0;
	byte m = 0;

	cBufferOffset = pstSendCmd->cBufferOffset;

	if( pstSendCmd->iCmdLen > 2 && ( pstSendCmd->pcCmd[2 + g_CANoffset] & 0x10 ) == 0x10 )
	{
		bMultiFrame = true;
		iCmdLen = pstSendCmd->iCmdLen;

		cFrameSum = ( iCmdLen - 10 - g_CANoffset ) / 7;  // 满帧的个数；
		cFrameSum += ( ( iCmdLen - 10 - g_CANoffset ) % 7 == 0 ) ? 0 : 1;
	}
	
	//多帧，组装成一帧
	if( bMultiFrame )
	{
		stCopySendCmd.iCmdLen = pstSendCmd->iCmdLen + cFrameSum * (4 + g_CANoffset) + 5;   //命令长度
		stCopySendCmd.pcCmd = ( byte* )malloc( sizeof( byte ) * stCopySendCmd.iCmdLen + 6 );

		stCopySendCmd.pcCmd[0] = ( byte )g_p_stISO15765Config->cMultiframestime;	//多帧帧时间间隔

		stCopySendCmd.pcCmd[1] = 0x07; //帧属性  0x01: CAN多帧格式，第一帧有30帧回复； 0x02:发一帧等待回复一帧  0x04:刷写，两个字节帧长
			
		if (stCopySendCmd.pcCmd[1] == 0x01)
		{
			stCopySendCmd.pcCmd[2] = cFrameSum + 1;
			
			FrameTypeOffset = 0;
		}
		else if(stCopySendCmd.pcCmd[1] == 0x07)
		{
			//帧个数
			if (cFrameSum + 1 > 0xff)
			{
				stCopySendCmd.pcCmd[2] = ((cFrameSum + 1) >> 8) & 0xFF;
			}
			else
			{
				stCopySendCmd.pcCmd[2] = 0x00;
			}

			stCopySendCmd.pcCmd[3] = (cFrameSum + 1) & 0xFF;
			
			FrameTypeOffset = 1;
		}

		stCopySendCmd.pcCmd[3 + FrameTypeOffset] = 10 + g_CANoffset; //第一帧命令的字节个数

		memcpy( stCopySendCmd.pcCmd + 4 + FrameTypeOffset, pstSendCmd->pcCmd, 10 + g_CANoffset );

		for( i = 0; i < cFrameSum; i++ )
		{
			if(g_CANoffset == StandardCAN)
			{
				stCopySendCmd.pcCmd[14+ FrameTypeOffset + i * 11 + 1] = stCopySendCmd.pcCmd[4 + FrameTypeOffset]; //ID
				stCopySendCmd.pcCmd[14+ FrameTypeOffset + i * 11 + 2] = stCopySendCmd.pcCmd[5 + FrameTypeOffset]; //ID
				stCopySendCmd.pcCmd[14+ FrameTypeOffset + i * 11 + 3] = 0x20 | ( ( i + 1 ) & 0x0F );
			}
			else if(g_CANoffset == ExtendCAN)
			{
				stCopySendCmd.pcCmd[16+ FrameTypeOffset + i * 13 + 1] = stCopySendCmd.pcCmd[4 + FrameTypeOffset]; //ID
				stCopySendCmd.pcCmd[16+ FrameTypeOffset + i * 13 + 2] = stCopySendCmd.pcCmd[5 + FrameTypeOffset]; //ID
				stCopySendCmd.pcCmd[16+ FrameTypeOffset + i * 13 + 3] = stCopySendCmd.pcCmd[6 + FrameTypeOffset]; //ID
				stCopySendCmd.pcCmd[16+ FrameTypeOffset + i * 13 + 4] = stCopySendCmd.pcCmd[7 + FrameTypeOffset]; //ID
				stCopySendCmd.pcCmd[16+ FrameTypeOffset + i * 13 + 5] = 0x20 | ( ( i + 1 ) & 0x0F );
			}

			if( i != cFrameSum - 1 ) //如果不是最后一帧
			{
				memcpy( stCopySendCmd.pcCmd + (14 + g_CANoffset) + FrameTypeOffset + i * (11 + g_CANoffset) + (4 + g_CANoffset), pstSendCmd->pcCmd + (10 + g_CANoffset) + i * 7, 7 );
				stCopySendCmd.pcCmd[14 + g_CANoffset + FrameTypeOffset + i * (11 + g_CANoffset)] = 2 + g_CANoffset + 1 + 7; //每帧命令的字节个数

			}
			else
			{
				LastBytesLen = iCmdLen - (10 + g_CANoffset) - i * 7;

				if (LastBytesLen < 7)                                                                                                                        
				{                                                                                                                                                                    
					stCopySendCmd.iCmdLen += 7-LastBytesLen;                                                                                                 
				}                                                                                                                                                                    

				memcpy( stCopySendCmd.pcCmd + (14 + g_CANoffset) + FrameTypeOffset + i * (11 + g_CANoffset) + (4 + g_CANoffset), pstSendCmd->pcCmd + (10 + g_CANoffset) + i * 7, 7 );
				
				if (LastBytesLen < 7)                                                                                                                        
				{
					for (m=0; m< 7-LastBytesLen; m++)
					{
						stCopySendCmd.pcCmd[stCopySendCmd.iCmdLen - (7-LastBytesLen) + m] = 0;
					}
				}

				stCopySendCmd.pcCmd[14 + g_CANoffset + FrameTypeOffset + i * (11 + g_CANoffset)] = 2 + g_CANoffset + 1 + 7; //每帧命令的字节个数                                     
			}
			////命令不满6个字节的补0x00
			//if ( stCopySendCmd.pcCmd[14 + g_CANoffset + i * (11 + g_CANoffset)] < 0x0A )
			//{

			//	stCopySendCmd.pcCmd = realloc( stCopySendCmd.pcCmd, stCopySendCmd.iCmdLen + 10 - stCopySendCmd.pcCmd[14 + g_CANoffset + i * (11 + g_CANoffset)] );
			//	stCopySendCmd.iCmdLen  += 10 - stCopySendCmd.pcCmd[14 + g_CANoffset + i * (11 + g_CANoffset)];

			//	for ( j = 10 - stCopySendCmd.pcCmd[14 + g_CANoffset + i * (11 + g_CANoffset)]; j > 0; j-- )
			//	{
			//		stCopySendCmd.pcCmd[ (14 + g_CANoffset) + i * (11 + g_CANoffset) + (5 + g_CANoffset) + 6 - j] = 0xAA;
			//	}
			//	stCopySendCmd.pcCmd[14 + g_CANoffset  + i * (11 + g_CANoffset)] = 0x0A;
			//}
		}

		bSendStatus = package_frame( FRAME_HEAD_MULTI_FRAME, &stCopySendCmd, cIso15765ReservedByte );
		
		//释放内存
		if( NULL != stCopySendCmd.pcCmd )
		{
			free( stCopySendCmd.pcCmd );
			stCopySendCmd.pcCmd = NULL;
		}
		return bSendStatus;
	}

	// 单帧
	bSendStatus = package_frame( FRAME_HEAD_NORMAL_FRAME, pstSendCmd, cIso15765ReservedByte );
	
	return bSendStatus;
}
/*************************************************
Description:	打包数据
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int make_reprogram_cmd(byte *StoreSendCmd, int ReceiveFileByteNumber, byte SendDataFrameConter)
{
	STRUCT_CMD stSendCmdCache = {0};
	int iStatus = FAILE;
	byte cReceiveBuffer[20] = {0};

	if (ReceiveFileByteNumber <= 5)	//单帧
	{
		stSendCmdCache.iCmdLen = ReceiveFileByteNumber + 7;	//获取整帧长度
		stSendCmdCache.pcCmd = ( byte* )malloc( sizeof( byte ) * stSendCmdCache.iCmdLen );
	
		stSendCmdCache.pcCmd[0] = 0x07;
		stSendCmdCache.pcCmd[1] = 0xE0;

		stSendCmdCache.pcCmd[2] = ReceiveFileByteNumber + 2;
		stSendCmdCache.pcCmd[3] = 0x36;
		stSendCmdCache.pcCmd[4] = SendDataFrameConter;
		
		memcpy(stSendCmdCache.pcCmd + 7,StoreSendCmd,ReceiveFileByteNumber);
	}
	else
	{
		stSendCmdCache.iCmdLen = ReceiveFileByteNumber + 6 + g_CANoffset; //获取整帧长度
		stSendCmdCache.pcCmd = ( byte* )malloc( sizeof( byte ) * stSendCmdCache.iCmdLen );
		
		stSendCmdCache.pcCmd[0] = 0x07;
		stSendCmdCache.pcCmd[1] = 0xE0;

		if (ReceiveFileByteNumber + 2 > 0xff)
		{
			stSendCmdCache.pcCmd[2 + g_CANoffset] = 0x10 | (((ReceiveFileByteNumber +2) >> 8) & 0x0F);
		}
		else
		{
			stSendCmdCache.pcCmd[2 + g_CANoffset] = 0x10;
		}

		stSendCmdCache.pcCmd[3 + g_CANoffset] = (ReceiveFileByteNumber + 2) & 0xFF;

		stSendCmdCache.pcCmd[4 + g_CANoffset] = 0x36;
		stSendCmdCache.pcCmd[5 + g_CANoffset] = SendDataFrameConter;

		memcpy(stSendCmdCache.pcCmd + 6 + g_CANoffset,StoreSendCmd,ReceiveFileByteNumber);		
	}

	stSendCmdCache.cBufferOffset = 3;

	iStatus = reprogram_make_cmd_by_iso_15765( ( STRUCT_CMD * )&stSendCmdCache, cReceiveBuffer );

	if(NULL != stSendCmdCache.pcCmd)
		free( stSendCmdCache.pcCmd );

	return iStatus;
}
/*************************************************
Description: 原厂文件，转换为rpm文件
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int separate_and_send_file_data(byte FileData,FILE *FilePointer)
{
	byte temp[2];
	byte i;

	temp[0] = hex_to_asc((FileData>>4) & 0x0f);
	temp[1] = hex_to_asc(FileData & 0x0f);

	for (i=0; i<2; i++)
	{
		_Encryption(temp+i,1);	//数据加密处理
		fputc(temp[i],FilePointer);
	}

	return true;
}

/*************************************************
Description: 原厂文件，转换为rpm文件
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
int repro_change_original_file_to_rpm(void)
{
	FILE *FilePointer = NULL;
	int i;
	byte CurrentBlock = 0;
	byte *StoreSendCmd = NULL;
	int iReceiveResult = TIME_OUT;
	int LastDataLen = 0;
	int FrameLen = 0;
	int SendCmdByteCounter;	//统计一共发送了多少字节
	byte SendDataFrameConter = 0;

	for (i=0;;i++)
	{
		if (g_ReprogramFilePath[i] == '.')
		{
			g_ReprogramFilePath = realloc(g_ReprogramFilePath,i+5);
			g_ReprogramFilePath[i+1] = 'r';
			g_ReprogramFilePath[i+2] = 'p';
			g_ReprogramFilePath[i+3] = 'm';
			g_ReprogramFilePath[i+4] = '\0';
			break;
		}
	}

	/*打开文件*/
	FilePointer = fopen( g_ReprogramFilePath, "w" );
	if (NULL == FilePointer)
	{
		free( g_ReprogramFilePath );
		return FAILE;
	}

	//fputs("S00901.1125000380",FilePointer);	//写入版本信息
	//fputs("\n",FilePointer);

	for (CurrentBlock=g_ChooseReprogramBlock; CurrentBlock<REPROGRAM_FILE_BLOCKS_NUMBER; CurrentBlock++)
	{
		StoreSendCmd = ( byte * )malloc( REPROGRAM_EACH_BLOCK_SIZE  );

		SendCmdByteCounter = 0;//已发送字节清零
		SendDataFrameConter = 0;//发送命令帧计数器清零

		while (g_ProgramData[CurrentBlock].cValueDataLen.u32Bit - SendCmdByteCounter > REPROGRAM_EACH_BLOCK_SIZE)
		{
			memcpy( StoreSendCmd, g_ProgramData[CurrentBlock].pcData + SendCmdByteCounter, REPROGRAM_EACH_BLOCK_SIZE );
			SendCmdByteCounter = SendCmdByteCounter + REPROGRAM_EACH_BLOCK_SIZE;
			iReceiveResult = make_reprogram_cmd(StoreSendCmd, REPROGRAM_EACH_BLOCK_SIZE, ++SendDataFrameConter);
			if (iReceiveResult != SUCCESS)
			{
				return iReceiveResult;
			}

			FrameLen = pcPackageCmdCache[1]<<8 | pcPackageCmdCache[2];

			fputs("S1",FilePointer);

			separate_and_send_file_data(CurrentBlock,FilePointer);

			for (i = 0; i<FrameLen; i++)
			{
				separate_and_send_file_data(pcPackageCmdCache[i],FilePointer);
			}

			fputs("\n",FilePointer);

			if(NULL != pcPackageCmdCache)
				free( pcPackageCmdCache );
		}

		LastDataLen = g_ProgramData[CurrentBlock].cValueDataLen.u32Bit - SendCmdByteCounter;
		memcpy( StoreSendCmd, g_ProgramData[CurrentBlock].pcData + SendCmdByteCounter, LastDataLen );
		iReceiveResult = make_reprogram_cmd(StoreSendCmd, LastDataLen, ++SendDataFrameConter);
		if (iReceiveResult != SUCCESS)
		{
			return iReceiveResult;
		}

		FrameLen = pcPackageCmdCache[1]<<8 | pcPackageCmdCache[2];

		fputs("S1",FilePointer);

		separate_and_send_file_data(CurrentBlock,FilePointer);

		for (i = 0; i<FrameLen; i++)
		{
			separate_and_send_file_data(pcPackageCmdCache[i],FilePointer);
		}

		fputs("\n",FilePointer);

		if(NULL != pcPackageCmdCache)
			free( pcPackageCmdCache );

		if( NULL != StoreSendCmd )
			free( StoreSendCmd );
	}

	fclose(FilePointer);
	FilePointer = NULL;

	return SUCCESS;
}
/*************************************************
Description: 文件加密
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
void _Encryption( char* pData, int nLen )
{
	char cTmp;
	int i = 0;

	for ( ; i < nLen; i++ )
	{
		cTmp = *( pData + i );
		cTmp = cTmp ^ SECRET_KEY ;
		*(pData + i) = cTmp;
	}
}
/*************************************************
Description: 获取编程日期
Input:
pIn		输入与ECU刷写有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
void get_programe_date( char *DateTemp )
{
	int year, month, day;
	time_t nowtime;
	struct tm *timeinfo;
	byte temp[4];
	byte i, j;

	time( &nowtime );
	timeinfo = localtime( &nowtime );

	year = timeinfo->tm_year + 1900;
	month = timeinfo->tm_mon + 1;
	day = timeinfo->tm_mday;

	temp[0] = year / 100;
	temp[1] = year % 100;
	temp[2] = month;
	temp[3] = day;

	j = 0;

	for( i = 0; i < 4; i++ )
	{
		DateTemp[j++] = ( ( temp[i] >> 4 & 0x0F ) * 16 + ( temp[i] & 0x0F ) ) / 10 + 0x30;
		DateTemp[j++] = ( ( temp[i] >> 4 & 0x0F ) * 16 + ( temp[i] & 0x0F ) ) % 10 + 0x30;
	}

	return;
}