 
#include <stdio.h>
#include <iostream>
#include <fstream> 
#include <string>
#include <streambuf> 
#include "HCNetSDK.h"
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <iconv.h>
#include <malloc.h>
#include<queue>
#include<list>
#include<deque>

using namespace std;
 
 
//参数声明
int iNum = 0; 	//图片名称序号
LONG IUserID;	//摄像机设备
LONG IHandle = -1;//报警布防/撤防;
NET_DVR_DEVICEINFO_V40 struDeviceInfo;	//设备信息
 
typedef struct plateInfo{
    string carNum;
    int byEntireBelieve;

    float xmin;
    float ymin;
    float xmax;
    float ymax;
    int byVehicleType ;
    int byColorDepth;

    int byColor;
    // 事件
    int byIllegalType ;
    // 车辆品牌
    int byVehicleLogoRecog;
    // 车辆子品牌
    int byVehicleSubLogoRecog ;

}plateInfo;

// 队列消息
std::queue<plateInfo> queuePlate;  

char sDVRIP[20];	//抓拍摄像机设备IP地址
short wDVRPort = 8000;	//设备端口号
char sUserName[20];	//登录的用户名
char sPassword[20];	//用户密码
string LineByLine;//逐行读取文件 
 
 
//---------------------------------------------------------------------------------
				  //函数声明
void Init();//初始化
void Demo_SDK_Version(); //获取sdk版本
void Connect();//设置连接事件与重连时间
void Htime();//获取海康威视设备时间
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword);//注册摄像机设备
void CALLBACK MSesGCallback(LONG ICommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void *pUser);//报警回调函数
void SetMessageCallBack();//设置报警回调函数
void SetupAlarm();//报警布防
void CloseAlarm();//报警撤防
void OnExit(void);//退出
				  //---------------------------------------------------------------------------------------------------
				  //函数定义
				  //初始化
void Init()
{

	NET_DVR_Init();//初始化
	Demo_SDK_Version();//获取 SDK  的版本号和 build  信息	
}
 
//设置连接事件与重连时间
void Connect()
{
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);
}
//获取海康威视设备时间
void Htime() {
	bool iRet;
	DWORD dwReturnLen;
	NET_DVR_TIME struParams = { 0 };
 
	iRet = NET_DVR_GetDVRConfig(IUserID, NET_DVR_GET_TIMECFG, 1, \
		&struParams, sizeof(NET_DVR_TIME), &dwReturnLen);
	if (!iRet)
	{
		printf("NET_DVR_GetDVRConfig NET_DVR_GET_TIMECFG  error.\n");
		NET_DVR_Logout(IUserID);
		NET_DVR_Cleanup();
	}
 
	printf("%d年%d月%d日%d:%d:%d\n", struParams.dwYear, struParams.dwMonth, struParams.dwDay, struParams.dwHour, struParams.dwMinute, struParams.dwSecond);
}
 
//注册摄像机设备
bool Login(char *sDVRIP, short wDVRPort, char *sUserName, char *sPassword)
{

    NET_DVR_USER_LOGIN_INFO  pLoginInfo = {0};
    strcpy(pLoginInfo.sDeviceAddress, "10.10.31.119");
    strcpy(pLoginInfo.sUserName, "admin");
    strcpy(pLoginInfo.sPassword, "sy123456");
    pLoginInfo.wPort=8000;

	IUserID = NET_DVR_Login_V40(&pLoginInfo, &struDeviceInfo);

	// IUserID = NET_DVR_Login_V30("10.10.31.119", 8000, "admin", "sy123456", &struDeviceInfo);

    printf("encoder type:  %d\n" , struDeviceInfo.byCharEncodeType);
 
	if (IUserID < 0)
	{
		std::cout << "Login Failed! Error number：" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Cleanup();
		return false;
	}
	else
	{
		std::cout << "Login Successfully!" << std::endl;
		return true;
	}
 
}
 
//Demo_SDK_Version()海康威视sdk版本获取函数
void Demo_SDK_Version()
{
	unsigned int uiVersion = NET_DVR_GetSDKBuildVersion();
 
	char strTemp[1024] = { 0 };
	sprintf(strTemp, "HCNetSDK V%d.%d.%d.%d\n", \
		(0xff000000 & uiVersion) >> 24, \
		(0x00ff0000 & uiVersion) >> 16, \
		(0x0000ff00 & uiVersion) >> 8, \
		(0x000000ff & uiVersion));
	printf(strTemp);
}


int code_convert(const char *from_charset, const char *to_charset, char *inbuf, size_t inlen,
                 char *outbuf, size_t outlen) {
    iconv_t cd;
    char **pin = &inbuf;
    char **pout = &outbuf;

    cd = iconv_open(to_charset, from_charset);
    if (cd == 0)
        return -1;

    memset(outbuf, 0, outlen);

    if ((int)iconv(cd, pin, &inlen, pout, &outlen) == -1)
    {
        iconv_close(cd);
        return -1;
    }
    iconv_close(cd);
    *pout = (char *)'\0';

    return 0;
}

int g2u(char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
    return code_convert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
}

std::string GBKToUTF8(const std::string& strGBK)
{
    int length = strGBK.size()*2+1;

    char *temp = (char*)malloc(sizeof(char)*length);

    if(g2u((char*)strGBK.c_str(),strGBK.size(),temp,length) >= 0)
    {
        std::string str_result;
        str_result.append(temp);
        free(temp);
        return str_result;
    }else
    {
        free(temp);
        return "";
    }
}


//定义异常消息回调函数
void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
	char tempbuf[256] = { 0 };
	switch (dwType)
	{
	case EXCEPTION_RECONNECT:    //预览时重连  
		printf("----------reconnect--------%d\n", time(NULL));
		break;
	default:
		break;
	}
}
 
 
//报警回调函数
void CALLBACK MSesGCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
 
	int i = 0;
	char filename[100];
	FILE *fSnapPic = NULL;
	FILE *fSnapPicPlate = NULL;
	switch (lCommand) {

	//交通抓拍结果(新报警消息)
	case COMM_ITS_PLATE_RESULT: {
		NET_ITS_PLATE_RESULT struITSPlateResult = { 0 };
		memcpy(&struITSPlateResult, pAlarmInfo, sizeof(struITSPlateResult));
		for (i = 0; i<struITSPlateResult.dwPicNum; i++)
		{
            auto carNum = GBKToUTF8(struITSPlateResult.struPlateInfo.sLicense).c_str();

			// if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 1) || (struITSPlateResult.struPicInfo[i].byType == 2))
			// {
			// 	sprintf(filename, "./pic/%s_%d.jpg", carNum.c_str(), i);
			// 	fSnapPic = fopen(filename, "wb");
			// 	fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, fSnapPic);
			// 	iNum++;
			// 	fclose(fSnapPic);
			// }
			// //车牌小图片
			// if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 0))
			// {
			// 	sprintf(filename, "./pic/snap/%s_%d.jpg", carNum.c_str(), i);
			// 	fSnapPicPlate = fopen(filename, "wb");
			// 	fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, \
			// 		fSnapPicPlate);
			// 	iNum++;
			// 	fclose(fSnapPicPlate);
			// }

			//获取车辆相关信息......
            // 车牌置信度
            int byEntireBelieve  = struITSPlateResult.struPlateInfo.byEntireBelieve;

            // 车牌位置 
            auto struPlateRect = struITSPlateResult.struPlateInfo.struPlateRect;

            // 车辆类型 byVehicleType 车辆类型，0-其他车辆，1-小型车，2-大型车，3- 行人触发，4- 二轮车触发，5- 三轮车触发，6- 机动车触发 

            int byVehicleType =struITSPlateResult.struVehicleInfo.byVehicleType;

            // 车身颜色深浅
            auto byColorDepth  = struITSPlateResult.struVehicleInfo.byColorDepth ;

            // 车身颜色
            auto byColor = struITSPlateResult.struVehicleInfo.byColor ;

            // 事件
            auto byIllegalType  = struITSPlateResult.struVehicleInfo.byIllegalType ;

            // 车辆品牌
            auto byVehicleLogoRecog  = struITSPlateResult.struVehicleInfo.byVehicleLogoRecog ;

            // 车辆子品牌
            auto byVehicleSubLogoRecog  = struITSPlateResult.struVehicleInfo.byVehicleSubLogoRecog ;

            // 子品牌年款
            // auto byVehicleModel  = struITSPlateResult.struVehicleInfo.byVehicleModel ; 

            //车辆置信度
            // auto byBelieve  = struITSPlateResult.struVehicleInfo.byBelieve ;


            // std::cout << carNum << " 车牌置信度: " << byEntireBelieve << " 车辆类型: "<< byVehicleType << " 车身颜色深浅: " <<byColorDepth \
            //  << " 车身颜色: " << byColor << " 车辆品牌: " << byIllegalType <<  " 车辆子品牌:  " << byVehicleLogoRecog << std::endl;

            plateInfo plate;
            // if (byEntireBelieve == 0)  break;

            plate.carNum = carNum;
            plate.byEntireBelieve  = byEntireBelieve ;
            plate.xmin = struPlateRect.fX;
            plate.ymin = struPlateRect.fY;
            plate.xmax = struPlateRect.fX + struPlateRect.fWidth;
            plate.ymax = struPlateRect.fY + struPlateRect.fHeight;
            plate.byVehicleType    = byVehicleType;
            plate.byColor   = byColor;
            plate.byColorDepth =byColorDepth;
            plate.byIllegalType = byIllegalType;
            plate.byVehicleLogoRecog = byVehicleLogoRecog;
            plate.byVehicleSubLogoRecog = byVehicleSubLogoRecog;

            queuePlate.push(plate);

		}
		break;
	}
	default: {
		break;
	}
	}
 	return;
}
//设置报警回调函数
void SetMessageCallBack()
{
	NET_DVR_SetDVRMessageCallBack_V30(MSesGCallback, NULL);
}
//报警布防
void SetupAlarm()
{
	//启动布防
	NET_DVR_SETUPALARM_PARAM struSetupParam = { 0 };
	struSetupParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
 
 
	struSetupParam.byAlarmInfoType = 1;//上传报警信息类型：0-老报警信息(NET_DVR_PLATE_RESULT), 1-新报警信息(NET_ITS_PLATE_RESULT)
	struSetupParam.byLevel = 1;//布防优先级：0- 一等级（高），1- 二等级（中），2- 三等级（低）
							   //bySupport 按位表示，值：0 - 上传，1 - 不上传;  bit0 - 表示二级布防是否上传图片;
 
 
	IHandle = NET_DVR_SetupAlarmChan_V41(IUserID, &struSetupParam);//建立报警上传通道，获取报警等信息。
	if (IHandle < 0)
	{
		std::cout << "NET_DVR_SetupAlarmChan_V41 Failed! Error number：" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Logout(IUserID);
		NET_DVR_Cleanup();
		return;
	}
	std::cout << "启动布防成功\n" << endl;
 
}
//报警撤防
void CloseAlarm()
{
	//撤销布防上传通道
	if (!NET_DVR_CloseAlarmChan_V30(IHandle))//布防句柄IHandle
	{
		std::cout << "NET_DVR_CloseAlarmChan_V30 Failed! Error number：" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Logout(IUserID);
		NET_DVR_Cleanup();
		return;
	}
	IHandle = -1;//布防句柄;
}
//退出
void OnExit(void)
{
 
	//报警撤防
	CloseAlarm();
 
	//释放相机
	NET_DVR_Logout(IUserID);//注销用户
	NET_DVR_Cleanup();//释放SDK资源	
}



void run()
{
	Init();//初始化sdk
	Connect();//设置连接事件与重连时间			  	
	Login(sDVRIP, wDVRPort, sUserName, sPassword);	//注册设备
	Htime(); //获取海康威视设备时间
	SetupAlarm();//布防
	SetMessageCallBack();	//注册报警回调函数 

}


int main(){
    run();

    // 不断的从队列中取数据，然后发送kafka

    while(1){
        if(queuePlate.size())
        {	 
            printf("qsize len : %d \n", queuePlate.size());
            auto info =  queuePlate.front();
            printf("get from queue and carNum: %s", info.carNum.c_str());
            queuePlate.pop();

        }

        sleep(0.1);
    }

	atexit(OnExit);//退出
    return 0;
    }
