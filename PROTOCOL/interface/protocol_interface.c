/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	定义各接口函数
History:
	<author>	<time>		<desc>

************************************************************************/

#define PROTOCOL_DLL_EXPORTS

#include <string.h>
#include <stdlib.h>

/*#define NDEBUG*/
#include <assert.h>

#include "protocol_interface.h"

#include "../function/dtc_lib.h"
#include "../function/clear_dtc_lib.h"
#include "../function/ds_lib.h"
#include "../function/infor_lib.h"
#include "../function/active_ecu_lib.h"
#include "../function/idle_link_lib.h"
#include "../function/freeze_lib.h"
#include "../function/actuator_test_lib.h"
#include "../SpecialFunction/special_function.h"
#include "../function/quit_system_lib.h"
#include "../InitConfigFromXml/init_config_from_xml_lib.h"
#include "../public/public.h"
#include "../ECUReprogram/reprogram_function.h"


//全局变量，用于退出死循环
bool g_bCancelWaitDlg = true;
/*************************************************
Description:	注册发送命令回调函数
Input:
	receive_cmd_callback	发送命令函数指针，由应用程序定义
Return:	void
Others:
*************************************************/
void regist_send_cmd_callback( SEND_CMD_CALLBACK send_cmd_callback )
{
	send_cmd = send_cmd_callback;
}

/*************************************************
Description:	注册接收命令回调函数
Input:
	receive_cmd_callback	接收命令函数，由应用程序定义
Return:	void
Others:
*************************************************/
void regist_receive_cmd_callback( RECEIVE_CMD_CALLBACK receive_cmd_callback )
{
	receive_cmd = receive_cmd_callback;
}
/*************************************************
Description:	注册接收所有带帧头命令回调函数
Input:
receive_cmd_callback	接收命令函数，由应用程序定义
Return:	void
Others:
*************************************************/
void regist_receive_all_cmd_callback( RECEIVE_ALL_CMD_CALLBACK receive_all_cmd_callback )
{
	receive_all_cmd = receive_all_cmd_callback;
}
/*************************************************
Description:	注册延时回调函数
Input:
	time_delay_callback		接收命令函数，由应用程序定义
Return:	void
Others:
*************************************************/
void regist_time_delay( TIME_DELAY time_delay_callback )
{
	time_delay_ms = time_delay_callback;
}
/*************************************************
Description:	注册更新上位机界面回调函数
Input:
update_ui_callback		更新上位机界面回调函数，由应用程序定义
Return:	void
Others:
*************************************************/
void regist_update_ui_callback( UPDATE_UI update_ui_callback )
{
	update_ui = update_ui_callback;
}
/*************************************************
Description:	设置VCI
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:	尝试发送三次若失败输出FAIL
*************************************************/
int setting_vci( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	byte *pcOutTemp = ( byte * )pOut;

	int iVciConfigOffset = 0;
	int iProtocolConfigOffset = 0;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iVciConfigOffset = atoi( pstParam->pcData );//获取VCI配置偏移
	g_p_stVCI_params_config = g_p_stVciParamsGroup[iVciConfigOffset];

	pstParam = pstParam->pNextNode;//获取VCI配置模板号

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	pstParam = pstParam->pNextNode;//获取协议配置偏移

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	iProtocolConfigOffset = atoi( pstParam->pcData );

	pstParam = pstParam->pNextNode;//获取协议配置偏移

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );

	g_stInitXmlGobalVariable.m_cProtocolConfigType = ( byte )atoi( pstParam->pcData ); //获取协议配置模板号(类型)

	//选择协议配置，根据相应协议选择相应配置
	select_protocol_config( iProtocolConfigOffset, g_stInitXmlGobalVariable.m_cProtocolConfigType );

	if( package_and_send_vci_config() ) //如果接收成功且设置成功则返回SUCCESS
	{
		general_return_status( SUCCESS, NULL, 0, pcOutTemp );
		pcOutTemp[2] = 0;//不提示
	}
	else
	{
		general_return_status( FAILE, NULL, 0, pcOutTemp );
		pcOutTemp[2] = 0;//不提示
	}

	return 0;
}
/*************************************************
Description:	激活ECU
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int active_ECU( void* pIn, void* pOut )
{
	process_active_ECU( pIn, pOut );

	return 0;
}

/*************************************************
Description:	读取当前故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int read_current_Dtc( void* pIn, void* pOut )
{
	process_read_current_Dtc( pIn, pOut );

	return 0;
}

/*************************************************
Description:	读取历史故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int read_history_Dtc( void* pIn, void* pOut )
{
	process_read_history_Dtc( pIn, pOut );

	return 0;
}


/*************************************************
Description:	清除故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int clear_Dtc( void* pIn, void* pOut )
{

	process_clear_Dtc( pIn, pOut );

	return 0;
}


/*************************************************
Description:	读冻结帧故障码
Input:	pIn		输入参数（保留）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int read_freeze_frame_DTC( void* pIn, void* pOut )
{
	process_read_freeze_frame_DTC( pIn, pOut );

	return 0;
}


/*************************************************
Description:	读冻结帧数据流
Input:	pIn		列表中DTC的ID
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int read_freeze_frame_DS( void* pIn, void* pOut )
{
	process_read_freeze_frame_DS( pIn, pOut );

	return 0;
}

/*************************************************
Description:	读数据流
Input:	pIn		DS的ID
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int read_data_stream( void* pIn, void* pOut )
{

	process_read_data_stream( pIn, pOut );

	return 0;
}


/*************************************************
Description:	读版本信息
Input:	pIn		版本信息组的ID
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int read_Ecu_information( void* pIn, void* pOut )
{
	process_read_ECU_information( pIn, pOut );

	return 0;
}

/*************************************************
Description:	执行器测试
Input:	pIn		测试项的ID（带输入型的附加输入值）
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int actuator_test( void* pIn, void* pOut )
{
	process_actuator_test( pIn, pOut );

	return 0;
}


/*************************************************
Description:	特殊功能
Input:	pIn		与执行功能有关的命令数据
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int special_function( void* pIn, void* pOut )
{
	process_special_function( pIn, pOut );
	return 0;
}
/*************************************************
Description:	ECU刷写
Input:	pIn		与执行功能有关的命令数据
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int reprogram_function( void* pIn, void* pOut )
{
	repro_config_insert_Framing_response(1);//在发送命令之前设置插帧命令；时间间隔2S  0：关  1：开
	process_reprogram_function( pIn, pOut );
	repro_config_insert_Framing_response(0);//在发送命令之前设置插帧命令；时间间隔2S  0：关  1：开
	return 0;
}
/*************************************************
Description:	退出系统
Input:	pIn		保留
Output:	pOut	输出数据地址
Return:	保留
Others:
*************************************************/
int quit_system( void* pIn, void* pOut )
{

	process_quit_system( pIn, pOut );

	return 0;
}
/*************************************************
Description:	从xml获取系统配置
Input:
		iConfigType		配置类型
		PIn				具体配置内容
Output:	保留
Return:	保留
Others:
*************************************************/

int init_config_from_xml( int iConfigType, void* pIn )
{
	process_init_config_from_xml( iConfigType, pIn );

	return 0;
}



STRUCT_SELECT_FUN stSetConfigFunGroup[] =
{
	{SET_CONFIG_FC_CAN, process_SET_CONFIG_FC_CMD_CAN},
	{SET_ECU_REPLAY, process_SET_ECU_REPLY},
	{SET_SEND_AND_RECEIVE, get_accord_ecu_cmdnum_send_cmdconfig_data },

};


/*************************************************
Description:	获取处理激活函数
Input:
	cType		配置类型
Output:	保留
Return:	pf_general_function 函数指针
Others:
*************************************************/
pf_general_function get_set_config_fun( byte cType )
{
	int i = 0;

	for( i = 0; i < sizeof( stSetConfigFunGroup ) / sizeof( stSetConfigFunGroup[0] ); i++ )
		if( cType == stSetConfigFunGroup[i].cType )
			return stSetConfigFunGroup[i].pFun;

	return 0;
}

/*************************************************
Description:	 设置配置命令
Input:	PIn		保留
Output:	保留
Return:	保留
Others:
*************************************************/
int special_config_function( void* pIn, void* pOut )
{
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	pf_general_function pFun = NULL;
	byte cConfigType = 0;
	int iConfigOffset = 0;

	do
	{
		assert( pstParam->pcData );
		assert( pstParam->iLen != 0 );
		iConfigOffset = atoi( pstParam->pcData ); //获得处理函数配置ID

		pstParam = pstParam->pNextNode;
		assert( pstParam->pcData );
		assert( pstParam->iLen != 0 );
		cConfigType = ( byte )atoi( pstParam->pcData ); //获得处理函数配置模板号

		u32Config_fuc = u32Config_fuc_Group[iConfigOffset];

		pstParam = pstParam->pNextNode;

		pFun = get_set_config_fun( cConfigType );

		assert( pFun );

		pFun( pstParam, pOut );

		if( 1 != *( byte * )pOut )
		{
			return 0;
		}

		pstParam = pstParam->pNextNode;

	}
	while( pstParam != NULL );

	return 0;
}



/*************************************************
Description:	 等待框中加按钮，此函数用于退出死循环
Input:	PIn		保留
Output:	保留
Return:	保留
Others:
*************************************************/
void cancelWaitDlg(void* pIn, void* pOut)
{
	g_bCancelWaitDlg = false;
}
