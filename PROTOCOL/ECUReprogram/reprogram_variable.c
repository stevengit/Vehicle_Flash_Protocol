/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义特殊功能用到的变量
History:
	<author>	<time>		<desc>

************************************************************************/
#include "reprogram_function.h"
#include <stdio.h>

uint32 ReceiveFileByteNumber = 0;
byte g_VDILinkStatus = 0x00;//0x01:USB连接方式	0x02:蓝牙连接方式
byte g_NegativeCode[3] = {0};	//存放消极响应值
byte * g_ReprogramDllFilePath_Decryption = NULL;//文件调用Dll路径
byte * g_ReprogramDllFilePath_Key = NULL; //文件调用Dll路径
byte * g_ReprogramDllFilePath[5] = {0}; //文件调用Dll路径
byte * g_ReprogramFilePath = NULL;	//文件路径
byte * g_ReprogramFilePath1 = NULL;	//文件路径1
byte * g_ReprogramFilePath2 = NULL;	//文件路径2 解密Fp文件

STRUCT_REPROGRAM_DATA g_ProgramData[REPROGRAM_FILE_BLOCKS_NUMBER] = {false,
											  0,
											  0,
											  0,
											  0,
											  0,
											  0,
											  NULL};
byte g_ChooseReprogramBlock = 0; //指定操作哪一块数据
bool g_DataSendOver = false;	//数据是否已发送
uint16 g_seedCMDBF[2];
byte g_LogisticID[18] = {0};
bool g_VDIUploadSchedule = false;//VDI上传刷写进度，蓝牙通讯时用
byte *pcPackageCmdCache = NULL;	//打包好后的命令存储区
byte g_CurrentBlockNumber = 0;	//当前操作块数
byte g_ReprogramFileType = 0;	//所选刷写文件格式 1:RPM 2:HEX(原厂) 3:EOL 
byte ECUVersionReceive[10] = {0};	//ECU版本号 
char *g_Secretkey = "20080808" ;
UNN_2WORD_4BYTE CheckSum_Address = {0};