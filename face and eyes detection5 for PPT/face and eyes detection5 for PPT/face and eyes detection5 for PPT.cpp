/*
功能：实现眨眼计数。
版本：2.0 
时间：2014-11-25
*/
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>

using namespace std;
using namespace cv;

#define width_needed           250      //定义所需图像长度
#define rate                   10      //定义检测频率
#define threshold_limit        15       //定义眼睛二值化上限
#define integral_threshold     70      //定义眨眼积分上限
#define close_eyes_time_limit  2        //定义闭眼报警时间长度
#define blink_frequency_time   10       //定义眨眼频率监视时间
#define blink_frequency_number_limit  10       //定义眨眼频率报警上限


void detectEyeAndFace( Mat frame );//声明检测函数

//加载级联器
//将下面两个文件复制到当前工程下，当前文件路径应该是OpenCV安装路径下的sources\data\haarcascades目录下
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_mcs_eyepair_big.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;

int main(void)
{
	//判断face_cascade_name、eye_cascade_name能够顺利加载
	if( !face_cascade.load( face_cascade_name ) ){ cout<<"face_cascade_name加载失败"<<endl; system("puase");return -1; }
	if( !eyes_cascade.load( eyes_cascade_name ) ){ cout<<"eye_cascade_name加载失败"<<endl;system("puase"); return -1; }
	
    VideoCapture cap("3.avi");	//打开摄像头：0打开电脑自带摄像头，1打开usb摄像头 
	if(!cap.isOpened()){cout<<"摄像头连接错误"<<endl;system("puase");return -1;}

	//显示原视频图像和处理后图像宽度，高度
	//int width=cap.get(CV_CAP_PROP_FRAME_WIDTH);//640
	//int hight=cap.get(CV_CAP_PROP_FRAME_HEIGHT);//480
	
	//cout<<"initial"<<endl;
	//cout<<"width:"<<width<<endl;
	//cout<<"hight:"<<hight<<endl<<endl;

	Mat frame;//定义一帧图像
	cap>>frame;//取下一帧
	namedWindow("眼睛和脸部跟踪检测",WINDOW_NORMAL);//创建视频窗口
	namedWindow("眼睛检测区域",WINDOW_NORMAL);//创建眼睛检测区域窗口（单人）
	namedWindow("眼睛",WINDOW_NORMAL);//创建眼睛检测区域窗口（单人）
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

		//识别函数
		detectEyeAndFace( frame ); 

		if(waitKey(1000/rate)>=0)stop = true;//设置视频帧间隔，通过按键停止视频
	}
	return 0;
}

//定义检测函数
void detectEyeAndFace( Mat frame )
{
	vector<Rect> empty;
	vector<Rect> faces; 
	vector<Rect> eyes;
	static vector<Rect> faces_front; 
	static vector<Rect> eyes_front; 
	Mat grayFrame;//定义灰度图
	Mat faceROI;//当前标注的脸部区域
	Mat eyeROI;//当前标注的眼部区域
	int k,l;
	int integral=0;//二值图积分
	static int close_eyes_number=0;                //定义连续闭眼帧数
	static int blink_number_all=0;                 //定义眨眼总次数
    static bool blink_record[blink_frequency_time*rate];//定义眼睛眨眼记录数组
    static int blink_record_i=0;                   //定义当前帧在眼睛眨眼记录数组中的位置
    static bool eyestate=0;                        //定义眼睛状态
    static int blink_number=0;                     //监视时间内眨眼次数


	cvtColor( frame, grayFrame, CV_BGR2GRAY );//转换为灰度图
	equalizeHist( grayFrame, grayFrame );//灰度图均衡化

	face_cascade.detectMultiScale( grayFrame, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE);//脸部检测
	if(faces.empty())
	{
		faces=faces_front;//如果这一帧没有捕捉到脸部，则沿用上一帧
	}

	for( size_t i = 0; i < faces.size(); i++ )
	{
		//用矩形标注脸部
		Point p1(faces[i].x,faces[i].y),p2(p1.x + faces[i].width,p1.y + faces[i].height);
		rectangle(frame,p1,p2,Scalar( 255, 0, 255 ),2,8,0);
		//用圆形标注脸部
		//Point center( int(faces[i].x + faces[i].width*0.5), int(faces[i].y + faces[i].height*0.5) );
		//ellipse( frame, center, Size( int(faces[i].width*0.5), int(faces[i].height*0.5)), 0, 0, 360, Scalar( 255, 0, 255 ), 2, 8, 0 );

		faceROI = grayFrame( faces[i] );  //得到当前标注的脸部区域
		//显示眼睛检测区域
		imshow( "眼睛检测区域", faceROI );

		eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE);//眼部检测
		if(eyes.empty())
		{
			eyes=eyes_front;//如果这一帧没有捕捉到脸部，则沿用上一帧
		}


		for( size_t j = 0; j < eyes.size(); j++ )
		{
			//用矩形标注眼睛
			Point p3(p1.x + eyes[j].x,p1.y + eyes[j].y),p4(p3.x + eyes[j].width,p3.y + eyes[j].height);
			rectangle(frame,p3,p4,Scalar( 255, 0, 0 ),2,8,0);
			//用圆形标注眼睛
			//Point center( int(faces[i].x + eyes[j].x + eyes[j].width*0.5), int(faces[i].y + eyes[j].y + eyes[j].height*0.5) ); 
			//int radius = cvRound( (eyes[j].width + eyes[i].height)*0.25 );
			//circle( frame, center, radius, Scalar( 255, 0, 0 ), 3, 8, 0 );
			
			eyeROI = faceROI( eyes[j] );  //得到当前标注的脸部区域
			//imshow( "眼睛", eyeROI );/////////////
			threshold(eyeROI,eyeROI, threshold_limit, 255, THRESH_BINARY);//将眼睛灰度图二值化///////////////////////////////
		    //显示眼睛检测区域
		    imshow( "眼睛", eyeROI );/////////////
			//计算眼睛积分
			for(k=1;k<eyeROI.rows;k++)
			{
				for(l=1;l<eyeROI.cols;l++)
				{
					if(eyeROI.at<uchar>(k,l)==0)integral++;
				}
			}
			//cout<<integral<<endl;//////////////////////////////////////////////////////////////
			//眨眼判别与处理
			if(integral<integral_threshold)//*******************************这一帧闭眼
			{
				close_eyes_number++;
				//cout<<"闭眼帧数"<<close_eyes_number<<endl;////////////
				//闭眼时间监视
				if(close_eyes_number>(close_eyes_time_limit*rate))
				{
					cout<<"闭眼时间超过限制！"<<endl;
				}
			    if(eyestate==0)//**************前一帧睁眼
			    {
			        blink_number_all++;
			        blink_record[blink_record_i]=1;
			        cout<<blink_number_all<<endl;//总眨眼次数//////////////////////////////////////
					eyestate=1;
			    }else//************************前一帧闭眼
				{
					blink_record[blink_record_i]=0;
				}
		    }else//*********************************************************这一帧睁眼
			{
				close_eyes_number=0;
				blink_record[blink_record_i]=0;
				if(eyestate==1)//***************前一帧闭眼
				{
					eyestate=0;
				}else//*************************前一帧睁眼
				{
				}
			}
		}
	}
	blink_record_i++;
	//监视时间内眨眼处理
	if(blink_record_i==blink_frequency_time*rate)
	{
		for(int i=0;i<blink_frequency_time*rate;i++)
		{
			if(blink_record[i]==1)blink_number++;
		}
		if(blink_number>blink_frequency_number_limit)
		{
			cout<<"眨眼频率过高！"<<endl;
		}
		blink_record_i=0;
	}
	faces_front=empty;
	eyes_front=empty;
	faces_front=faces;//保存这一帧脸部
	eyes_front=eyes; //保存这一帧眼部
	faces=empty;
	eyes=empty;

	imshow( "眼睛和脸部跟踪检测", frame );
}