#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>

using namespace std; 
using namespace cv;

//将视频文件复制到当前工程下，并改名为Sampe
int main()
{
	VideoCapture cap("3.avi");	//打开视频  
	if(!cap.isOpened())
	{
		cout<<"视频读取错误"<<endl;
		system("puase");
		return -1;
	}

	double rate=cap.get(CV_CAP_PROP_FPS);//获取帧率
	int delay=1000/rate;//每帧之间的延迟与视频的帧率相对应

	Mat frame;//定义帧
	Mat edges;

	//开始视频处理
	bool stop = false;
	while(!stop)
	{
		if(!cap.read(frame))//获取视频下一帧
		{
			cout<<"视频结束"<<endl;
			system("pause");
			break;
		}
		cvtColor(frame, edges, CV_BGR2GRAY);//将原图像转换为灰度图像
		GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);//高斯滤波
		Canny(edges, edges, 0, 30, 3);//边缘检测
		imshow("当前视频",edges);//显示视频
		if(waitKey(delay)>=0)//引入延迟，通过按键停止视频
			stop = true;
	}
	return 0;
}