#include "CHKCamera.h" 
#include <unistd.h> 
using namespace std; 
int main()
{
    HKIPCamera *hkcp = new HKIPCamera("10.10.31.119", 8000, "admin", "sy123456");
    cout << *hkcp << endl;
    hkcp->start();
    sleep(1);

    long old_id = 0;
    for (auto i = 0; ; i++)
    {
        //cv::imshow("display", show_img);
        //cv::waitKey(1);
	//usleep(50000);

        long now_id = hkcp->get_id();
	if (now_id == old_id) {continue;}

	cv::Mat show_img;
        cv::resize( hkcp->current(1080, 1920), show_img, cv::Size(800,600));
	string out ="imgs/" + to_string(hkcp->get_time()) + ".jpg" ;
	cv::imwrite(out, show_img);
	old_id = now_id;
	//cv::imshow("display", show_img);
        //cv::waitKey(1);

    }

    hkcp->stop();
    delete hkcp;
}
