#include "CHKCamera.h"
#include <time.h>
#include <string>

using namespace cv;
using namespace std;

// cv::Mat wall;
// = imread("/home/scarlet/Pictures/wallpaper.png");
INIT_DEBUG
INIT_TIMER

HKIPCamera::HKIPCamera(char *IP, int Port, char *UserName, char *Password)
{
    m_ip = IP;
    m_port = Port;
    m_username = UserName;
    m_password = Password;
    cvImg = Mat(Size(MAX_FRAME_HEIGHT, MAX_FRAME_WIDTH), CV_8UC3);
}


time_t StringToTimeStamp(int year, int month, int day,int hour, int minute, int second){
    struct tm tm_;
    tm_.tm_year  = year-1900;
    tm_.tm_mon   = month-1;
    tm_.tm_mday  = day;
    tm_.tm_hour  = hour;
    tm_.tm_min   = minute;
    tm_.tm_sec   = second;
    tm_.tm_isdst = 0;

    time_t timeStamp = mktime(&tm_);
    return timeStamp;
}

long HKIPCamera::get_id(){
    std::lock_guard<std::mutex> my_lock_guard(mylock);
    return id;
}


long HKIPCamera::get_time(){
    std::lock_guard<std::mutex> my_lock_guard(mylock);
    return time_stamp;
}

std::string HKIPCamera::get_time_show(){
    std::lock_guard<std::mutex> my_lock_guard(mylock);
    return time_stamp_show;
}



//call back now
void g_fPlayESCallBack(LONG lPreviewHandle, NET_DVR_PACKET_INFO_EX *pstruPackInfo, void *pUser)
{
    HKIPCamera *hkcp = (HKIPCamera *)pUser;
    if (pstruPackInfo->dwPacketType == 11)
        return;

    // std::cout  << "type : "  << pstruPackInfo->dwPacketType << std::endl;
    hkcp->pkt->size = pstruPackInfo->dwPacketSize;

    // std::cout  << "size : "  << hkcp->pkt->size  << std::endl;
    hkcp->pkt->data = (uint8_t *)pstruPackInfo->pPacketBuffer;
    int ret = avcodec_send_packet(hkcp->c, hkcp->pkt);


    // START_TIMER
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(hkcp->c, hkcp->pYUVFrame);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            // STOP_TIMER("HKIPCamera_frame")
            return;
        }

        auto frame_height = hkcp->pYUVFrame->height;
        auto frame_width = hkcp->pYUVFrame->width;
        if (av_image_fill_arrays(hkcp->pRGBFrame->data, hkcp->pRGBFrame->linesize, hkcp->buffer, AV_PIX_FMT_BGR24, frame_width, frame_height, 1))
        {
            auto img_convert_ctx = sws_getContext(frame_width, frame_height, AV_PIX_FMT_YUV420P, frame_width, frame_height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
            sws_scale(img_convert_ctx, (uint8_t const *const *)hkcp->pYUVFrame->data, hkcp->pYUVFrame->linesize, 0, frame_height, hkcp->pRGBFrame->data, hkcp->pRGBFrame->linesize);
            sws_freeContext(img_convert_ctx);

	// add lock

	{
	std::lock_guard<std::mutex> my_lock_guard(hkcp->mylock);

        hkcp->cvImg = Mat(frame_height, frame_width, CV_8UC3, *(hkcp->pRGBFrame->data));
        auto time_stamp = StringToTimeStamp(pstruPackInfo->dwYear, pstruPackInfo->dwMonth, pstruPackInfo->dwDay, \
            pstruPackInfo->dwHour ,pstruPackInfo->dwMinute , pstruPackInfo->dwSecond);
        char ss[100];
        sprintf(ss ,"%4d/%02d/%02d-%02d:%02d:%02d",pstruPackInfo->dwYear, pstruPackInfo->dwMonth, pstruPackInfo->dwDay, \
            pstruPackInfo->dwHour ,pstruPackInfo->dwMinute , pstruPackInfo->dwSecond);
        hkcp->time_stamp = time_stamp * 1000 + pstruPackInfo->dwMillisecond ;
        hkcp->time_stamp_show = ss;

        hkcp->id = hkcp->id + 1;
	std::cout << hkcp->id << " " << hkcp->time_stamp << std::endl;
        }

	}
    }
}

bool HKIPCamera::initFFMPEG()
{
    pkt = av_packet_alloc();
    if (!pkt)
        return HPR_ERROR;

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        fprintf(stderr, "Codec not found\n");
        return HPR_ERROR;
    }

    c = avcodec_alloc_context3(codec);
    if (!c)
    {
        fprintf(stderr, "Could not allocate video codec context\n");
        return HPR_ERROR;
    }
    c->thread_count = 2;

    if (avcodec_open2(c, codec, NULL) < 0)
    {
        fprintf(stderr, "Could not open codec\n");
        return HPR_ERROR;
    }

    pYUVFrame = av_frame_alloc();
    if (!pYUVFrame)
    {
        fprintf(stderr, "Could not allocate video pYUVFrame\n");
        return HPR_ERROR;
    }

    pRGBFrame = av_frame_alloc();
    if (!pRGBFrame)
    {
        fprintf(stderr, "Could not allocate video pRGBFrame\n");
        return HPR_ERROR;
    }

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, 1);
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    std::cout << "ffmpeg init done! " << std::endl;
    return HPR_OK;
}

bool HKIPCamera::login()
{
    NET_DVR_Init();
    initFFMPEG();

    NET_DVR_DEVICEINFO struDeviceInfo;

    printf("start to login camera \n");
    m_lUserID = NET_DVR_Login(m_ip, m_port, m_username, m_password, &struDeviceInfo);
    printf("login return %d \n", m_lUserID);

    if (m_lUserID >= 0){

        OUTPUT_DEBUG("login success!");
        return HPR_OK;
	}
    else
    {
        OUTPUT_DEBUG("login wrong");
        cout << "Login ID = " << m_lUserID << "; Error Code = " << NET_DVR_GetLastError() << endl;
        return HPR_ERROR;
    }
}

bool HKIPCamera::open()
{
    NET_DVR_PREVIEWINFO struPlayInfo = {0};
    struPlayInfo.hPlayWnd = 0;     //?????? SDK ??????????????????????????????,?????????????????????????????????
    struPlayInfo.lChannel = 1;     //???????????????
    struPlayInfo.dwStreamType = 0; //0-?????????,1-?????????,2-?????? 3,3-?????? 4,????????????
    struPlayInfo.dwLinkMode = 1;   //0- TCP ??????,1- UDP ??????,2- ????????????,3- RTP ??????,4-RTP/RTSP,5-RSTP/HTTP
    struPlayInfo.bBlocked = 0;     //0- ???????????????,1- ????????????

    m_lRealPlayHandle = NET_DVR_RealPlay_V40(m_lUserID, &struPlayInfo, NULL, this);

    printf("start to realplay \n");
    if (NET_DVR_SetESRealPlayCallBack(m_lRealPlayHandle, g_fPlayESCallBack, this))
    {
        OUTPUT_DEBUG("RealPlay started!");
        return HPR_OK;
    }


    cout << "????????????????????????????????????" << NET_DVR_GetLastError() << endl;
    NET_DVR_Logout(m_lUserID);
    NET_DVR_Cleanup();
    return HPR_ERROR;
}

bool HKIPCamera::close()
{
    NET_DVR_StopRealPlay(m_lRealPlayHandle);

    return HPR_OK;
}

bool HKIPCamera::logout()
{
    NET_DVR_Logout(m_lUserID);
    NET_DVR_Cleanup();

    avcodec_free_context(&c);

    av_frame_free(&pYUVFrame);
    av_frame_free(&pRGBFrame);
    av_packet_free(&pkt);
    av_free(buffer);

    OUTPUT_DEBUG("Finished");
    return HPR_OK;
}

// ======================C++ Interface defined========================

bool HKIPCamera::start()
{
    return login() && open();
}

bool HKIPCamera::stop()
{
    return close() && logout();
}

Mat HKIPCamera::current(int rows, int cols)
{

    std::lock_guard<std::mutex> my_lock_guard(mylock);
    Mat frame;
    resize(cvImg, frame, cv::Size(cols, rows));
    return frame;
}

ostream &operator<<(ostream &output, HKIPCamera &hkcp)
{
    output << hkcp.m_ip << "-"
           << hkcp.m_port << "-"
           << hkcp.m_username << "-"
           << hkcp.m_password;

    return output;
}

// ======================C/Python Interface defined========================
HKIPCamera *HKIPCamera_init(char *ip, int port, char *name, char *pass) { return new HKIPCamera(ip, port, name, pass); }
int HKIPCamera_start(HKIPCamera *hkcp) { return hkcp->start(); }
int HKIPCamera_stop(HKIPCamera *hkcp) { return hkcp->stop(); }
void HKIPCamera_frame(HKIPCamera *hkcp, int rows, int cols, unsigned char *frompy)
{
    // START_TIMER
    memcpy(frompy, (hkcp->current(rows, cols)).data, rows * cols * 3);
    // STOP_TIMER("HKIPCamera_frame")
}
