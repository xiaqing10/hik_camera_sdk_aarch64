#include "CXMCamera.h"
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace cv;
int main(int argc, char **argv)
{
    XMIPCamera *xmcp = new XMIPCamera("10.41.0.199", 34567, "admin", "");
    cout << *xmcp << endl;

    xmcp->start();

    for (auto i = 0; i < 1500; i++)
    {
        // imshow("display", xmcp->current());
        // imshow("display", xmcp->current(540,960));
        imshow("display", xmcp->current(1080, 1920));
        waitKey(1);
    }

    xmcp->stop();
    delete (xmcp);

    // cout << "C INTERFACE" << endl;

    // XMIPCamera *c_xmcp = XMIPCamera_init("10.41.0.208", 34567, "admin", "");
    // XMIPCamera_start(c_xmcp);
    // sleep(2);

    // for (auto i = 0; i < 50; i++)
    // {
    //     imshow("c_display", c_xmcp->current());
    //     waitKey(1);
    // }

    // XMIPCamera_stop(c_xmcp);
    // delete (c_xmcp);
    return 0;
}
