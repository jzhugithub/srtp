/*
功能：实现眨眼计数等等（详见自定义参数部分）
版本：3.0 
时间：2015-4-5
*/
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;
using namespace cv;

//-------------------------自定义参数----------------------------
#define initial_time					3		//定义参数初始化时间
#define width_needed 					400 	//定义所需图像长度默认长宽比为4：3
#define rate 							10 		//定义检测频率
#define threshold_limit 				45 		//定义眼睛二值化上限
#define integral_threshold 				36  	//定义眨眼积分上限（手动方式）
#define blink_intergral_coefficient		0.8		//眨眼积分上限判断系数（即占睁眼积分的多大比例算闭眼）
#define close_eyes_time_limit 			2 		//定义闭眼报警时间长度
#define blink_frequency_time 			10 		//定义眨眼频率监视时间
#define blink_frequency_number_limit 	8 		//定义眨眼频率报警上限
#define faces_empty_time                5		//定义脸部丢失报警时间长度
#define eyes_empty_time					10		//定义眼部丢失报警时间长度
#define mode 							0 		//选择视频图像来源：0-打开电脑自带摄像头，1-打开usb摄像头 
#define set_size						0		//选择视频调整方法：0-驱动调整，1-程序调整
#define blink_judge_method				1		//选择眨眼积分确定方式：0-手动，1-自动
#define integral_show					1		//是否显示眼部积分：1-是，0-否
#define blink_number_all_show			1 		//是否显示总眨眼次数：1-是，0-否
#define close_eyes_number_show			0		//是否显示连续闭眼帧数：1-是，0-否
#define faces_width_show				0		//是否显示脸部区域宽度：1-是，0-否
#define playmusic						1		//是否播放音乐：1-是，0-否

//-------------------------定义全局变量---------------------------
int total_integral=0;					//初始化时眼部的积分累加值
int total_faces_width=0;				//初始化时脸部宽度累加值
int integral_coefficient=0;				//“脸部宽度平方/眼睛积分”系数估计

//--------------------------程序准备------------------------------
void adjust( Mat frame );				//声明用户调节摄像头函数
void initial( Mat frame );				//声明初始化函数
void detectEyeAndFace( Mat frame );		//声明检测函数

//加载级联器
//将下面两个文件复制到当前工程下，当前文件路径应该是OpenCV安装路径下的sources\data\haarcascades目录下
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_mcs_eyepair_big.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;

//---------------------------主程序--------------------------------------------------
//-----------------------------------------------------------------------------------
int main(void)
{
	//判断face_cascade_name、eye_cascade_name能够顺利加载
	if( !face_cascade.load( face_cascade_name ) ){ cout<<"face_cascade_name加载失败"<<endl;getchar();return -1; }
	if( !eyes_cascade.load( eyes_cascade_name ) ){ cout<<"eye_cascade_name加载失败"<<endl;getchar(); return -1; }
	
	VideoCapture cap(mode);		//打开摄像头
	if(!cap.isOpened()){cout<<"摄像头连接错误"<<endl;getchar();return -1;}

	//--------------------调整图像大小-----------------------------
	//显示原视频图像和处理后图像宽度，高度
	int width0=cap.get(CV_CAP_PROP_FRAME_WIDTH);	//640
	int hight0=cap.get(CV_CAP_PROP_FRAME_HEIGHT);	//480
	cout<<"initial"<<endl;
	cout<<"width:"<<width0<<endl;
	cout<<"hight:"<<hight0<<endl<<endl;

	Mat frame;		//定义一帧图像
	cap>>frame;		//获取第一帧，删除后运行，会发生错误
	cap>>frame;
	int hight1;
	int width1;

	//调整视频大小
	bool set_size1=set_size;
	if(set_size1==1)//1.程序调整方法
	{
		resize(frame, frame, Size(), width_needed*1.0/width0, width_needed*1.0/width0);
		//改变后的视频图像宽度，高度
		hight1=frame.rows;
		width1=frame.cols;
	}
	else//2.驱动调整方法
	{
		//改变视频图像宽度，高度(由于视频驱动的关系，可能不能用，可进入驱动设置或卸载驱动)
		cap.set(CV_CAP_PROP_FRAME_WIDTH,width_needed);
		cap.set(CV_CAP_PROP_FRAME_HEIGHT,width_needed*3/4);	
		//改变后的视频图像宽度，高度，频率
		width1=cap.get(CV_CAP_PROP_FRAME_WIDTH);
		hight1=cap.get(CV_CAP_PROP_FRAME_HEIGHT);
		cap>>frame;
	}

	//显示改变后的视频图像宽度，高度
	cout<<"result"<<endl;
	cout<<"width:"<<width1<<endl;
	cout<<"hight:"<<hight1<<endl<<endl;

	namedWindow("眼睛和脸部跟踪检测",WINDOW_NORMAL);	//创建视频窗口
	namedWindow("眼睛检测区域",WINDOW_NORMAL);			//创建眼睛检测区域窗口
	namedWindow("眼睛",WINDOW_NORMAL);					//创建眼睛区域窗口
	namedWindow("眼睛（二值化）",WINDOW_NORMAL);		//创建眼睛二值化区域窗口
	
	//-----------------等待用户调节摄像头-------------------------
	cout<<"请将摄像头调节至适当位置，输入任何字符确认"<<endl;
	bool stop = false;
	while(!stop)
	{
		cap>>frame;			//取下一帧
		if(!frame.empty())
		{
			if(set_size1!=0)//调整视频大小1.程序调整方法
			{
				resize(frame, frame, Size(), width_needed*1.0/width0, width_needed*1.0/width0);
			}

			//用户调节摄像头函数
			adjust( frame );

			if(waitKey(1000/rate)>=0)stop = true;//设置视频帧间隔，通过按键进行确认
		}
	}
	cout<<"退出调节阶段"<<endl;

	//---------------------参数初始化-----------------------------
	cout<<"请保持睁眼"<<initial_time<<"秒，"<<"或提前按任意键退出初始化阶段"<<endl;
	int i=0;
	stop = false;
	while( i < initial_time*rate && !stop )
	{
		cap>>frame;			//取下一帧
		if(!frame.empty())
		{
			if(set_size1!=0)//调整视频大小1.程序调整方法
			{
				resize(frame, frame, Size(), width_needed*1.0/width0, width_needed*1.0/width0);
			}

			//初始化函数
			initial( frame );

			if(waitKey(1000/rate)>=0)stop = true;//设置视频帧间隔，通过按键停止视频
			i++;
		}
	}

	int average_integral=total_integral/(initial_time*rate);					//初始化时眼部的积分累加值平均
	int average_faces_width=total_faces_width/(initial_time*rate);				//初始化时脸部宽度累加值平均
	if(average_integral!=0)integral_coefficient=average_faces_width*average_faces_width/average_integral;//“脸部宽度平方/眼睛积分”系数估计
	else integral_coefficient=200;//默认值200，防程序出错

	cout<<"退出初始化阶段"<<endl;

	//---------------------开始视频处理----------------------------
	cout<<"开始检测，输入任何字符停止"<<endl;
	stop = false;
	while(!stop)
	{
		cap>>frame;			//取下一帧
		if(!frame.empty())
		{
			if(set_size1!=0)//调整视频大小1.程序调整方法
			{
				resize(frame, frame, Size(), width_needed*1.0/width0, width_needed*1.0/width0);
			}

			//识别函数
			detectEyeAndFace( frame ); 

			if(waitKey(1000/rate)>=0)stop = true;//设置视频帧间隔，通过按键停止视频
		}
	}
	return 0;
}

//------------------------定义用户调节摄像头函数-------------------------------------
//-----------------------------------------------------------------------------------
void adjust( Mat frame )
{
	//-----------------------参数定义------------------------------
	vector<Rect> faces; 
	vector<Rect> eyes;
	Mat grayFrame;										//定义灰度图
	Mat faceROI;										//当前标注的脸部区域
	Mat eyeROI;											//当前标注的眼部区域

	//-----------------------开始检测------------------------------
	cvtColor( frame, grayFrame, CV_BGR2GRAY );			//转换为灰度图
	equalizeHist( grayFrame, grayFrame );				//灰度图均衡化
	face_cascade.detectMultiScale( grayFrame, faces, 1.1, 2, 0, Size(10,10));//脸部检测

	//-----------------------脸部显示------------------------------
	if(!faces.empty())
	{
		//用矩形标注脸部
		Point p1(faces[0].x,faces[0].y),p2(p1.x + faces[0].width,p1.y + faces[0].height);
		rectangle(frame,p1,p2,Scalar( 255, 0, 255 ),2,8,0);

		faceROI = grayFrame( faces[0] );	//得到当前标注的脸部区域
		equalizeHist( faceROI, faceROI );	//脸部灰度图均衡化
		imshow( "眼睛检测区域", faceROI );	//显示眼睛检测区域
		eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0,Size(2,2));//眼部检测
		
		//--------------------眼部处理----------------------------
		if(!eyes.empty())
		{
			//用矩形标注眼睛
			Point p3(p1.x + eyes[0].x,p1.y + eyes[0].y),p4(p3.x + eyes[0].width,p3.y + eyes[0].height);
			rectangle(frame,p3,p4,Scalar( 255, 0, 0 ),2,8,0);

			eyeROI = faceROI( eyes[0] );		//得到当前标注的眼部区域
			imshow( "眼睛", eyeROI );			//显示眼睛区域
			threshold(eyeROI,eyeROI, threshold_limit, 255, THRESH_BINARY);//将眼睛灰度图二值化
			imshow( "眼睛（二值化）", eyeROI );//显示二值化眼睛区域

		}//眼部处理结束
	}//脸部处理结束

	imshow( "眼睛和脸部跟踪检测", frame );//显示摄像头区域
}

//------------------------定义初始化函数---------------------------------------------
//-----------------------------------------------------------------------------------
void initial( Mat frame )
{
	//-----------------------参数定义------------------------------
	vector<Rect> faces; 
	vector<Rect> eyes;
	static vector<Rect> faces_front; 
	static vector<Rect> eyes_front;
	Mat grayFrame;										//定义灰度图
	Mat faceROI;										//当前标注的脸部区域
	Mat eyeROI;											//当前标注的眼部区域
	int k,l;
	int integral=0;										//二值图积分

	//-----------------------开始检测------------------------------
	cvtColor( frame, grayFrame, CV_BGR2GRAY );			//转换为灰度图
	equalizeHist( grayFrame, grayFrame );				//灰度图均衡化
	face_cascade.detectMultiScale( grayFrame, faces, 1.1, 2, 0, Size(10,10));//脸部检测

	//-------------------处理脸部丢失情况--------------------------
	if(faces.empty())
	{
		faces=faces_front;				//如果这一帧没有捕捉到脸部或脸部图像有问题，则沿用上一帧
	}

	//-----------------------脸部处理------------------------------
	if(!faces.empty())
	{
		//用矩形标注脸部
		Point p1(faces[0].x,faces[0].y),p2(p1.x + faces[0].width,p1.y + faces[0].height);
		rectangle(frame,p1,p2,Scalar( 255, 0, 255 ),2,8,0);

		faceROI = grayFrame( faces[0] );	//得到当前标注的脸部区域
		equalizeHist( faceROI, faceROI );	//脸部灰度图均衡化
		imshow( "眼睛检测区域", faceROI );	//显示眼睛检测区域
		eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0,Size(2,2));//眼部检测

		//----------------处理眼部丢失情况------------------------
		if(eyes.empty())
		{
				eyes=eyes_front;			//如果这一帧没有捕捉到脸部或眼部图像有问题，则沿用上一帧
		}
		
		//--------------------眼部处理----------------------------
		if(!eyes.empty()&&eyes[0].x>0&&eyes[0].y>0&&eyes[0].x+eyes[0].width<faces[0].width&&eyes[0].y+eyes[0].height<faces[0].height)
		{
			//用矩形标注眼睛
			Point p3(p1.x + eyes[0].x,p1.y + eyes[0].y),p4(p3.x + eyes[0].width,p3.y + eyes[0].height);
			rectangle(frame,p3,p4,Scalar( 255, 0, 0 ),2,8,0);

			eyeROI = faceROI( eyes[0] );		//得到当前标注的眼部区域
			imshow( "眼睛", eyeROI );			//显示眼睛区域
			threshold(eyeROI,eyeROI, threshold_limit, 255, THRESH_BINARY);//将眼睛灰度图二值化
			imshow( "眼睛（二值化）", eyeROI );//显示二值化眼睛区域

			//--------------计算眼睛积分---------------------------
			for(k=1;k<eyeROI.rows;k++)
			{
				for(l=1;l<eyeROI.cols;l++)
				{
					if(eyeROI.at<uchar>(k,l)==0)integral++;
				}
			}
			bool integral_show1=integral_show;
			if(integral_show1==1){cout<<"眼部积分："<<integral<<endl;}	//显示眼部积分
			total_integral+=integral;	//每次检测的积分值累加
			total_faces_width+=faces[0].width;
		}//眼部处理结束
	}//脸部处理结束

	faces_front=faces;	//保存这一帧脸部
	eyes_front=eyes;	//保存这一帧眼部
	imshow( "眼睛和脸部跟踪检测", frame );//显示摄像头区域
}

//------------------------定义检测函数-----------------------------------------------
//-----------------------------------------------------------------------------------
void detectEyeAndFace( Mat frame )
{
	//-----------------------参数定义------------------------------
	vector<Rect> faces; 
	vector<Rect> eyes;
	static vector<Rect> faces_front; 
	static vector<Rect> eyes_front; 
	Mat grayFrame;										//定义灰度图
	Mat faceROI;										//当前标注的脸部区域
	Mat eyeROI;											//当前标注的眼部区域
	int k,l;
	int integral=0;										//二值图积分
	static int close_eyes_number=0;						//定义连续闭眼帧数
	static int blink_number_all=0;						//定义眨眼总次数
    static bool blink_record[blink_frequency_time*rate];//定义眼睛眨眼记录数组
    static int blink_record_i=0;						//定义当前帧在眼睛眨眼记录数组中的位置
    static bool eyestate=1;								//定义眼睛状态
	static int faces_empty_number;						//脸部丢失次数
	static int eyes_empty_number;						//眼部丢失次数
    int blink_number=0;									//监视时间内眨眼次数

	//-----------------------开始检测------------------------------
	cvtColor( frame, grayFrame, CV_BGR2GRAY );			//转换为灰度图
	equalizeHist( grayFrame, grayFrame );				//灰度图均衡化
	face_cascade.detectMultiScale( grayFrame, faces, 1.1, 2, 0, Size(10,10));//脸部检测

	//-------------------处理脸部丢失情况--------------------------
	if(faces.empty())
	{
		if(faces_empty_number<faces_empty_time*rate)
		{
			faces=faces_front;				//如果这一帧没有捕捉到脸部或脸部图像有问题，则沿用上一帧
			faces_empty_number++;
		}
		else
		{
			cout<<"脸部图像丢失！"<<endl;
			faces_empty_number=0;
		}
	}
	else{faces_empty_number=0;}

	//-----------------------脸部处理------------------------------
	if(!faces.empty())
	{
		//用矩形标注脸部
		Point p1(faces[0].x,faces[0].y),p2(p1.x + faces[0].width,p1.y + faces[0].height);
		rectangle(frame,p1,p2,Scalar( 255, 0, 255 ),2,8,0);

		bool faces_width_show1=faces_width_show;
		if(faces_width_show1==1){cout<<"脸部区域宽度"<<faces[0].width<<endl;}//显示脸部区域宽度
		faceROI = grayFrame( faces[0] );	//得到当前标注的脸部区域
		equalizeHist( faceROI, faceROI );	//脸部灰度图均衡化
		imshow( "眼睛检测区域", faceROI );	//显示眼睛检测区域
		eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0,Size(2,2));//眼部检测

		//----------------处理眼部丢失情况------------------------
		if(eyes.empty())
		{
			if(eyes_empty_number<eyes_empty_time*rate)
			{
				eyes=eyes_front;			//如果这一帧没有捕捉到脸部或眼部图像有问题，则沿用上一帧
				eyes_empty_number++;
			}
			else
			{
				cout<<"眼部图像丢失！"<<endl;
				eyes_empty_number=0;
			}
		}
		else{eyes_empty_number=0;}
		
		//--------------------眼部处理----------------------------
		if(!eyes.empty()&&eyes[0].x>0&&eyes[0].y>0&&eyes[0].x+eyes[0].width<faces[0].width&&eyes[0].y+eyes[0].height<faces[0].height)
		{
			//用矩形标注眼睛
			Point p3(p1.x + eyes[0].x,p1.y + eyes[0].y),p4(p3.x + eyes[0].width,p3.y + eyes[0].height);
			rectangle(frame,p3,p4,Scalar( 255, 0, 0 ),2,8,0);

			eyeROI = faceROI( eyes[0] );		//得到当前标注的眼部区域
			imshow( "眼睛", eyeROI );			//显示眼睛区域
			threshold(eyeROI,eyeROI, threshold_limit, 255, THRESH_BINARY);//将眼睛灰度图二值化
			imshow( "眼睛（二值化）", eyeROI );//显示二值化眼睛区域

			//--------------计算眼睛积分---------------------------
			for(k=1;k<eyeROI.rows;k++)
			{
				for(l=1;l<eyeROI.cols;l++)
				{
					if(eyeROI.at<uchar>(k,l)==0)integral++;
				}
			}

			bool integral_show1=integral_show;
			if(integral_show1==1){cout<<"眼部积分："<<integral<<endl;}//显示眼部积分

			//-------------眨眼判别与处理---------------------------
			bool blink_judge_method1=blink_judge_method;
			int integral_threshold1;
			if(blink_judge_method1==0)integral_threshold1=integral_threshold;//眨眼积分确定方式：0-手动
			else integral_threshold1=faces[0].width*faces[0].width/integral_coefficient*blink_intergral_coefficient;//眨眼积分确定方式：1-自动

			if(integral<integral_threshold1)//*（外层）这一帧闭眼
			{
				close_eyes_number++;
				bool close_eyes_number_show1=close_eyes_number_show;
				if(close_eyes_number_show1==1){cout<<"连续闭眼帧数："<<close_eyes_number<<endl;}//显示连续闭眼帧数
			    if(eyestate==0)//**（内层）前一帧睁眼
			    {
			        blink_number_all++;
			        bool blink_number_all_show1=blink_number_all_show;
			        if(blink_number_all_show1==1){cout<<"总眨眼次数："<<blink_number_all<<endl;}//显示总眨眼次数
			        blink_record[blink_record_i]=1;
			    }
			    else//**（内层）前一帧闭眼
				{
					if(close_eyes_number>close_eyes_time_limit*rate)
					{
						close_eyes_number=0;
						cout<<"闭眼时间超过限制！"<<endl;
						if (playmusic==1)//是否播放音乐
						{
							PlaySound (TEXT("提醒.wav"), NULL, SND_ASYNC | SND_NODEFAULT);
						}
					}
					//blink_record[blink_record_i]=0;//可省略
				}
				eyestate=1;
		    }
		    else//*（外层）这一帧睁眼
			{
				close_eyes_number=0;
				eyestate=0;
				//blink_record[blink_record_i]=0;//可省略
			}
			blink_record_i++;//帧在眼睛眨眼记录数组中的位置++

			//-----------监视时间内眨眼处理------------------------
			if(blink_record_i==blink_frequency_time*rate)
			{
				for(int i=0;i<blink_frequency_time*rate;i++)
				{
					if(blink_record[i]==1)blink_number++;
				}
				if(blink_number>blink_frequency_number_limit)
				{
					blink_number=0;
					cout<<"眨眼频率过高！"<<endl;
					if (playmusic==1)//是否播放音乐
					{
						PlaySound (TEXT("提醒.wav"), NULL, SND_ASYNC | SND_NODEFAULT);
					}		
				}
				blink_record_i=0;
				memset(blink_record, 0, sizeof(blink_record));
			}

		}//眼部处理结束
	}//脸部处理结束

	faces_front=faces;	//保存这一帧脸部
	eyes_front=eyes;	//保存这一帧眼部
	imshow( "眼睛和脸部跟踪检测", frame );//显示摄像头区域
}