#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>

using namespace std; 
using namespace cv;

int main()
{
	VideoCapture cap(0);	//打开摄像头 
	if(!cap.isOpened())
	{
		cout<<"摄像头连接错误"<<endl;
		system("puase");
		return -1;
	}

	//显示原视频图像宽度，高度
	double width=cap.get(CV_CAP_PROP_FRAME_WIDTH);//640
	double hight=cap.get(CV_CAP_PROP_FRAME_HEIGHT);//480
	
	cout<<"initial"<<endl;
	cout<<"width:"<<width<<endl;
	cout<<"hight:"<<hight<<endl<<endl;


	Mat frame;//定义帧
	Mat edges;
	cap>>frame;//获取第一帧，删除后运行，会发生错误
	cap>>frame;

	//改变图像宽度，高度
	resize(frame, frame, Size(), 0.5, 0.5);

	//显示改变后的视频图像宽度，高度
	hight=frame.rows;
	width=frame.cols;

	cout<<"result"<<endl;
	cout<<"width:"<<width<<endl;
	cout<<"hight:"<<hight<<endl<<endl;

	//开始视频处理
	bool stop = false;
	while(!stop)
	{
		cap>>frame;//获取视频下一帧

		resize(frame, frame, Size(), 0.5, 0.5); //改变图像大小
		cvtColor(frame, edges, CV_BGR2GRAY);//将原图像转换为灰度图像
		imshow("当前视频",edges);//显示视频
		if(waitKey(1)>=0)//改变摄像头频率，通过按键停止视频
			stop = true;
	}
	return 0;
}