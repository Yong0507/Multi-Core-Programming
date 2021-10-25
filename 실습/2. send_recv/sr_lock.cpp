//#include <iostream>
//#include <thread>
//#include <mutex>
//using namespace std;
//
//volatile int g_data = 0;
//volatile bool g_flag = false;
//
//void receive()
//{
//	while (false == g_flag);
//	cout << "Value : " << g_data << endl;
//}
//
//void sender()
//{
//	g_data = 999;
//	g_flag = true;
//	cout << "Sender finished." << endl;
//}
//
//int main()
//{
//	thread t1{ receive };
//	thread t2{ sender };
//	t1.join();
//	t2.join();
//}