/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:
	1.定义获取故障码状态函数；
	2.定义处理版本信息显示方式函数；
	3.定义处理普通数据流计算函数；
	4.定义处理冻结帧数据流计算函数；
	5.定义处理安全进入算法函数；

History:
	<author>	<time>		<desc>
************************************************************************/

#include <string.h>
#include "formula_comply.h"
#include "formula.h"
#include "../command/command.h"
#include "../public/public.h"
#include "../InitConfigFromXml/init_config_from_xml_lib.h"
#include "../public/protocol_config.h"
#include "../ECUReprogram/reprogram_function.h"

int	g_iDefaultSessionCmdIndex[]		= {1, 0};
int	g_iExtendSessionCmdIndex[]		= {1, 0};
int	g_iRequestSeedCmdIndex[]		= {1, 0};
int	g_iSendKeyCmdIndex[]			= {1, 0};
int g_iActiveECURequestSeedOffset;
int g_iActiveECUSendKeyOffset;

//故障码各状态位为1的显示
byte *DTCStatusBitTrueArry[] =
{
	"ID_STR_DTC_STATUS_BIT0_TRUE",
	"ID_STR_DTC_STATUS_BIT1_TRUE",
	"ID_STR_DTC_STATUS_BIT2_TRUE",
	"ID_STR_DTC_STATUS_BIT3_TRUE",
	"ID_STR_DTC_STATUS_BIT4_TRUE",
	"ID_STR_DTC_STATUS_BIT5_TRUE",
	"ID_STR_DTC_STATUS_BIT6_TRUE",
	"ID_STR_DTC_STATUS_BIT7_TRUE"

};
//故障码各状态位为0的显示
byte *DTCStatusBitFalseArry[] =
{
	"ID_STR_DTC_STATUS_BIT0_FALSE",
	"ID_STR_DTC_STATUS_BIT1_FALSE",
	"ID_STR_DTC_STATUS_BIT2_FALSE",
	"ID_STR_DTC_STATUS_BIT3_FALSE",
	"ID_STR_DTC_STATUS_BIT4_FALSE",
	"ID_STR_DTC_STATUS_BIT5_FALSE",
	"ID_STR_DTC_STATUS_BIT6_FALSE",
	"ID_STR_DTC_STATUS_BIT7_FALSE"
};
/*************************************************
Description:	获得故障码状态
Input:
	cDctStatusData	故障码状态字节
	cDtcMask		故障码mask值

Output:	pcOut	结果输出地址
Return:	int		该故障码支持的状态个数
Others:
*************************************************/

int get_Dtc_status( byte cDctStatusData, byte *pcOut, byte cDtcMask )
{
	int i = 0;
	int iSupportStatusCounter = 0;//支持的状态计数
	byte temp_Status = 0;
	byte temp_SupportStatus = 0;
	bool bFirst = true;
  
	while( i < 8 )
	{
		temp_SupportStatus = ( ( cDtcMask >> i ) & 0x01 );
		temp_Status = ( ( cDctStatusData >> i ) & 0x01 );

		if( 0x01 == temp_SupportStatus )//位为1
		{

			if( i > 0 && !bFirst )
			{
				*pcOut = ',';
				pcOut++;
			}

			bFirst = true;//第一次进来置为真

			if( 0x01 == temp_Status )
			{

				memcpy( pcOut, DTCStatusBitTrueArry[i], strlen( DTCStatusBitTrueArry[i] ) );
				pcOut += strlen( DTCStatusBitTrueArry[i] );

			}
			else//位为0
			{
				memcpy( pcOut, DTCStatusBitFalseArry[i], strlen( DTCStatusBitFalseArry[i] ) );
				pcOut += strlen( DTCStatusBitFalseArry[i] );

			}


			iSupportStatusCounter++;
		}

		i++;

	}

	*pcOut = '\0';

	return iSupportStatusCounter;
}





/*************************************************
Description:	处理版本信息显示格式
Input:
	pcSource	取值地址
	cIvalidLen	有效长度
	cStyle		显示方式

Output:	pcOut	结果输出地址
Return:	void
Others:
*************************************************/

void process_inform_format( const byte* pcSource, byte cIvalidLen, byte cStyle, byte* pcOut )
{
	switch( cStyle )
	{
	case 'A'://ASCII码方式处理
		DisASCII( pcSource, cIvalidLen, pcOut );
		break;

	case 'H':
	case 'B':
		DisHex( pcSource, cIvalidLen, pcOut );
		break;

	case 'D':
		break;

	default:
		break;
	}
}
/************************************************************************/
/* 下面是数据流字符串公式用到的                                         */
/************************************************************************/
/*************************************************
typedef struct _STRUCT_STRING_UNIT
{
	byte cCompareData;		//要比较的数据
	byte* pcOutputString;	//要输出的字符串
} STRUCT_STRING_UNIT;
*************************************************/

STRUCT_STRING_UNIT stStringUintArray000[] =
{
	0x00, "ID_STR_DS_CSTRING_000",//关	Off
	0x01, "ID_STR_DS_CSTRING_001",//开	On
};

/*************************************************
typedef struct _STRUCT_DIS_STRING
{
byte cCompareDataSum;					//需要比较数据个数
byte* pcDefaultOutputString;			//不满足要求时输出的字符串
STRUCT_STRING_UNIT *pstStringUnitGroup;	//定义在上面
} STRUCT_DIS_STRING;
*************************************************/
STRUCT_DIS_STRING stDisStringArray[] =
{
	{0x02, "----", stStringUintArray000}, //00  bit.0=1:开 bit.0=0:关
};
/*************************************************
Description:	根据数据流ID处理数据流计算公式
Input:
	iDsId		数据流项ID
	pcDsSource	取值地址

Output:	pcDsValueOut	结果输出地址
Return:	void
Others:
DisplayString(pcDsSource,stDisStringArraypcDsSource,stDisStringArray,0,0xff,0,pcDsValueOut);
*************************************************/

void process_normal_ds_calculate( int iDsId, const byte* pcDsSource, byte* pcDsValueOut )
{
	switch( iDsId )
	{
	case   0:
		OneByteOperation( pcDsSource, 0, 0, 1, 10, "%4.1f", pcDsValueOut ); //x/10
		break;
	default:
		break;
	}
}
/*************************************************
Description:	版本信息程序处理
Input:
iDsId
pcDsSource	取值地址

Output:	pcDsValueOut	结果输出地址
Return:	void
Others:
DisplayString(pcDsSource,stDisStringArraypcDsSource,stDisStringArray,0,0xff,0,pcDsValueOut);
*************************************************/
void process_normal_infor_calculate( int iDsId, const byte* pcDsSource, byte* pcDsValueOut )
{
	switch( iDsId )
	{
	case   0:
		OneByteOperation( pcDsSource, 0, 0, 1, 10, "%4.1f", pcDsValueOut ); //x/10
		break;
	default:
		break;
	}
}
/*************************************************
Description:	根据冻结帧数据流ID处理数据流计算公式
Input:
	iDsId		数据流项ID
	pcDsSource	取值地址

Output:	pcDsValueOut	结果输出地址
Return:	void
Others:
*************************************************/

void process_freeze_ds_calculate( int iDsId, const byte* pcDsSource, byte* pcDsValueOut )
{
	switch( iDsId )
	{
	case   0:
		DisplayString( pcDsSource, stDisStringArray, 1, 0X01, 0, pcDsValueOut ); //ACC.0=1~Present   ACC.0=0~Not Present
		break;
	default:
		break;
	}
}

/************************************************************************/
/* 下面是安全进入计算公式                                               */
/************************************************************************/
uint32 seedToKey(uint32 seed, uint32 MASK)
{
	uint32 key = 0;
	uint8 i;


	if(seed != 0)
	{
		for (i = 0; i < 30; i++)
		{
			if (seed & 0x80000000)
			{
				seed = seed << 1;
				seed = seed ^ MASK;
			}
			else
			{
				seed = seed << 1;
			}
		}

		key = seed;
	}

	return key;
}

uint8 ems_calculate(uint8 *Group)
{
	uint32 seed;
	uint32 Key;
	const    uint32   Learmask = 0x2459D1A3;
	seed = (uint32)(*Group << 24) | (uint32)(*(Group + 1) << 16) | (uint32)(*(Group + 2) << 8)  | (uint32)(*(Group + 3));

	Key = seedToKey(seed, Learmask);
	*Group = (uint8)(Key >> 24);
	*(Group + 1) = (uint8)(Key >> 16);
	*(Group + 2) = (uint8)(Key >> 8);
	*(Group + 3) = (uint8)Key;
	return 4;

}

/*************************************************
Description:	根据安全等级处理安全算法
Input:	cAccessLevel	安全等级

Output:	pOut	结果输出地址
Return:	bool	算法执行状态（成功、失败）
Others:	函数具体实现会因系统而异
*************************************************/

bool process_security_access_algorithm( byte cAccessLevel, void* pOut )
{
	bool bProcessSingleCmdStatus = false;
	byte cBufferOffset = 0;//缓存偏移
	uint16 cRequestSeedCmdOffset = 0;
	uint16 cSendKeyCmdOffset = 0;

	byte cDataArray[10] = {0};
	byte cNeedBytes = 0;

	//根据安全等级确定命令偏移
	switch( cAccessLevel )
	{
	case 0:
	case 1:
		cRequestSeedCmdOffset	= g_iRequestSeedCmdIndex[1];
		cSendKeyCmdOffset		= g_iSendKeyCmdIndex[1];
		break;
	case 2:
		cRequestSeedCmdOffset	= g_seedCMDBF[0];
		cSendKeyCmdOffset		= g_seedCMDBF[1];
		break;

	default:
		break;
	}

	cBufferOffset = g_stInitXmlGobalVariable.m_p_stCmdList[ cRequestSeedCmdOffset ].cBufferOffset;
	bProcessSingleCmdStatus = process_single_cmd_without_subsequent_processing( cRequestSeedCmdOffset, pOut );
	if( !bProcessSingleCmdStatus )
	{
		return false;
	}

	memcpy( cDataArray, ( g_stBufferGroup[cBufferOffset].cBuffer + 2 ), 4 );
	if ((0x00 == cDataArray[0])&&(0x00 == cDataArray[1])&&(0x00 == cDataArray[2])&&(0x00 == cDataArray[3]))
	{
		return true;
	}

	//if ((cDataArray[0] == 0)&&(cDataArray[1] == 0)&&(cDataArray[2] == 0)&&(cDataArray[3] == 0))
	//{
	//	return true;
	//}

	//根据安全等级确定计算公式
	switch( cAccessLevel )
	{
	case 0:
	case 1:
		cNeedBytes = ems_calculate( cDataArray );
		break;
	case 2:
		cNeedBytes = ems_calculate( cDataArray );
		break;
	default:
		break;
	}

	memcpy( ( g_stInitXmlGobalVariable.m_p_stCmdList[cSendKeyCmdOffset].pcCmd + 5 + g_CANoffset), cDataArray, cNeedBytes );

	bProcessSingleCmdStatus = process_single_cmd_without_subsequent_processing( cSendKeyCmdOffset, pOut );

	if( !bProcessSingleCmdStatus )
	{
		return false;
	}

	return true;

}