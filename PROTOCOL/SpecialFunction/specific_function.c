/**********************************************************************
Copyright:		YiXiong AUTO S&T Co., Ltd.
Description:	特殊功能的函数处理文件，将所有的处理函数写在 process_special_function 上边。这样不用写函数声明。
                所有特殊功能的操作都在一个文件内进行。
History:
	<author>    张学岭
	<time>		2015年5月18日14:08:25
	<desc>      文件最后有 函数模板，可以作参考
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "special_function.h"
#include "..\InitConfigFromXml\init_config_from_xml_lib.h"
#include <assert.h>

/*************************************************
Description:	标定处理函数
Input:
pIn		输入与特殊功能有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
bool process_judge_yes_no( byte * cSpecialCmdData, const STRUCT_CHAIN_DATA_INPUT* pstParam, void * pOut )
{
	int cBufferOffset = 0;//存储一条命令偏移量

	u8CalibrationFunctionJudge[cSpecialCmdData[1]] = cSpecialCmdData[2];  //存储每一组的是否选项

	switch( cSpecialCmdData[2] )
	{
	case 0:	//否
		special_return_status( PROCESS_OK | HAVE_JUMP | NO_TIP, "false", NULL, 0, pOut );
		break;

	case 1:	//是
		special_return_status( PROCESS_OK | HAVE_JUMP | NO_TIP, "true", NULL, 0, pOut );
		break;

	default:
		break;
	}

	return true;
}
/*************************************************
Description:	清空客户选择的强制刷写与否
Input:
pIn		输入与特殊功能有关的命令数据
		和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
bool process_calibration_exit( byte * cSpecialCmdData, const STRUCT_CHAIN_DATA_INPUT* pstParam, void * pOut )
{
	int i = 0;

	for (i=0; i<20; i++)
	{
		u8CalibrationFunctionJudge[i] = 0xff;
	}

	special_return_status( PROCESS_OK | HAVE_JUMP | NO_TIP, "return", NULL, 0, pOut );

	return true;
}

/*************************************************
Description:	特殊功能处理函数
Input:
pIn		输入与特殊功能有关的命令数据
和从UI输入的内容
Output:	pOut	输出数据地址
Return:	void
Others:	根据第一个命令数据执行不同的功能函数
*************************************************/
void process_special_function( void* pIn, void* pOut )
{

	byte cSpecialCmdData[40] = {0};//存放特殊功能命令数据
	STRUCT_CHAIN_DATA_INPUT* pstParam = ( STRUCT_CHAIN_DATA_INPUT* )pIn;
	byte FunctionSlect = 0;
	bool bStatus = false;

	assert( pstParam->pcData );
	assert( pstParam->iLen != 0 );
	FunctionSlect = atoi( pstParam->pcData );

	get_string_type_data_to_byte( cSpecialCmdData, pstParam->pcData, pstParam->iLen );
	//指向下一个结点
	pstParam = pstParam->pNextNode;

	/* 特殊功能的函数入口 */
	switch( cSpecialCmdData[0] )
	{
	case 0 ://判断是或否的标志位
		bStatus = process_judge_yes_no( cSpecialCmdData, pstParam, pOut );
		break;
	case 1:	//退出功能，清空缓存
		bStatus = process_calibration_exit( cSpecialCmdData, pstParam, pOut );
		break;

	default:
		break;
	}

}

