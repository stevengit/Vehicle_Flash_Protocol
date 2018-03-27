/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	声明特殊功能函数、定义相关宏和数据类型
History:
	<author>	<time>		<desc>

************************************************************************/
#ifndef _REPROGRAM_FUNCTION
#define _REPROGRAM_FUNCTION

#include "../interface/protocol_define.h"
#include "../command/command.h"
#include "../public/public.h"
#include "../public/protocol_config.h"
#include "../protocol/iso_14230.h"
#include "../protocol/iso_15765.h"
#include "../formula/formula.h"
#include "../formula/formula_comply.h"

#define VDI_UPDATE_TIP       0x00 //VDI升级进行到哪一步的提示
#define VDI_UPDATE_PROGRESS  0x01 //VDI升级的进度

//刷写文件的总块数
#define REPROGRAM_FILE_BLOCKS_NUMBER 0x05  

//刷写文件的起始于结束地址
#define REPROGRAM_BEGIN_ADDRESS_00	0x80000000
#define REPROGRAM_END_ADDRESS_00	0x8000BFFF

#define REPROGRAM_BEGIN_ADDRESS_01	0x8001C000
#define REPROGRAM_END_ADDRESS_01	0x8001FFFF

#define REPROGRAM_BEGIN_ADDRESS_02	0x8000C000
#define REPROGRAM_END_ADDRESS_02	0x8001BFFF

#define REPROGRAM_BEGIN_ADDRESS_03	0x80040000
#define REPROGRAM_END_ADDRESS_03	0x800FFFFF

#define REPROGRAM_BEGIN_ADDRESS_04	0x80020000
#define REPROGRAM_END_ADDRESS_04	0x8003FFFF

#define REPROGRAM_EACH_BLOCK_SIZE	0xFFF - 2
#define REPROGRAM_READ_DATA_SIZE	0xFFF

//#define SECRET_KEY	'F'
#define SECRET_KEY	0

#define REPROGRAM_SYSTEM_NAME	"_EDC17V720-"	//当前系统，生成的rpm刷写文件的名称

typedef struct
{
	bool cDataFlag;	//标志位
	UNN_2WORD_4BYTE  FrameTotalNumber;  //每块数据的总帧数
	UNN_2WORD_4BYTE  AllDataChecksum;   //每块数据的校验和，F0帧，用于VDI的校验
	UNN_2WORD_4BYTE  ValueDataChecksum; //每块数据的校验和，纯数据，用于ECU的校验
	UNN_2WORD_4BYTE cValueDataLen;	//每块数据的纯数据总长度
	UNN_2WORD_4BYTE cBeginAddress;
	UNN_2WORD_4BYTE cEndAddress;
	byte* pcData;
} STRUCT_REPROGRAM_DATA;	//存放刷写文件数据的结构体

enum security_mode {
	sa_diactivated			=10,
	sa_conti_standard_cus	=20,		/* Conti standard customer SA algorithm */
	sa_customer_reprog		=30,		/* Customer reprog mode */
	sa_customer_dev			=40,		/* Customer development mode */
	sa_other_modes			=50			/* Other possible modes */
};


extern uint32 ReceiveFileByteNumber;
extern byte g_VDILinkStatus;
extern byte g_NegativeCode[3];	//存放消极响应值
extern uint16 g_seedCMDBF[2];
extern byte g_LogisticID[18];

extern STRUCT_REPROGRAM_DATA g_ProgramData[REPROGRAM_FILE_BLOCKS_NUMBER];
extern byte * g_ReprogramDllFilePath_Decryption;//文件调用Dll路径
extern byte * g_ReprogramDllFilePath_Key;//文件调用Dll路径
extern byte * g_ReprogramDllFilePath[5];//文件调用Dll路径
extern byte * g_ReprogramFilePath;	//文件路径
extern byte * g_ReprogramFilePath1;	//文件路径
extern byte * g_ReprogramFilePath2;	//文件路径  解密Fp文件

extern byte g_ChooseReprogramBlock; //指定操作哪一块数据
extern bool g_DataSendOver;	//数据是否已发送
extern bool g_VDIUploadSchedule;//VDI上传刷写进度，蓝牙通讯时用
extern byte *pcPackageCmdCache;	//打包好后的命令存储区
extern byte g_CurrentBlockNumber;	//当前操作块数
extern byte g_ReprogramFileType;	//所选刷写文件格式 1:RPM 2:HEX(原厂) 3:EOL 
extern 	byte ECUVersionReceive[10];	//ECU版本号 
extern char *g_Secretkey ; //原始文件加密秘钥
extern 	UNN_2WORD_4BYTE CheckSum_Address;



/********************************具体功能函数声明***********************************/
void process_reprogram_function( void* pIn, void* pOut );
int reprogram_write_enter( byte * cSpecialCmdData, const STRUCT_CHAIN_DATA_INPUT* pstParam, void * pOut );
int reprogram_write_send( byte * cSpecialCmdData, const STRUCT_CHAIN_DATA_INPUT* pstParam, void * pOut );

int reprogram_send_usb( byte CurrentBlock,uint32* u32CmdIndex,void * pOut );
int reprogram_send_bluetooth( byte CurrentBlock,uint32* u32CmdIndex,void * pOut );
int reprogram_write_exit( byte * cSpecialCmdData, const STRUCT_CHAIN_DATA_INPUT* pstParam, void * pOut );

bool enter_system(void);
byte asc_to_hex(byte FileData);
byte merge_two_byte(byte FileData1,byte FileData2);
byte send_cmd_checksum(byte *buffer,int len);
int repro_send_link_mode(void* pOut);
// int repro_start_cmd(void);
int repro_checksum_and_cmd_sum(byte CurrentBlock);
int repro_start_transfer_data(void);
int repro_stop_transfer_data(void);
int repro_config_negative_response(byte NegativeControlStatus);
int repro_config_insert_Framing_response(byte NegativeControlStatus);
int read_ecu_file_way( byte * cSpecialCmdData, const STRUCT_CHAIN_DATA_INPUT* pstParam, void * pOut );
int repro_config_current_file_block(byte CurrentFileBlock);
int repro_make_file_data_HEX(void* pOut, byte *Filepath);
int repro_change_original_file_to_rpm(void);
int repro_initialize_rpm_file(void* pOut);
void _Encryption( char* pData, int nLen );
int repro_make_file_data(void* pOut);
void reprogram_exit_operate( void );
void get_programe_date( char *DateTemp );
byte merge_two_byte_and_secret(byte *FileData1,byte *FileData2);
int initialize_variable(void);
int repro_make_file_data_eol(void* pOut);
int repro_make_file_data_S19(void* pOut);
int judge_file_secret_hex(void* pOut, byte *Filepath);
void Encryption(char* vDataBuffer,const int nSize,const char* pSeed,const int nSeedLen,int nCount);

typedef int ( * lpEnableSADll)( const char*, int, void* );
typedef int ( * lpLoadSeed2Dll)( const unsigned char *, const unsigned char );
typedef int ( * lpGetSaKey)( unsigned char *, unsigned char * );
typedef unsigned long ( * MYPROC)( unsigned long Seed );
typedef unsigned long ( * MYPROC_CheckSum)( byte *pBuffer,unsigned long u32Size );
typedef unsigned long ( * MYPROC_DecryptHex)( byte *srcFileName, byte *tgtFileName );
unsigned long  Dll_Key(unsigned long Seed);
#endif