//#include <iostream> 
//#include <thread>
//#include <atomic>
//#include <mutex>
//using namespace std;
//
//int error_count = 0;
//volatile int* bound;
//volatile bool done = false;
//
//mutex myl;
//
//void worker0()
//{
//	for (int i = 0; i < 2500'0000; ++i) {
//		//myl.lock();
//		*bound = -(1 + *bound);
//		//myl.unlock();
//	}
//	done = true;
//}
//
//void worker1()
//{
//	while (false == done) {
//		//myl.lock();
//		int v = *bound;
//		//myl.unlock();
//		if ((v != 0) && (v != -1)) {
//			cout << hex << v << ", ";
//			error_count++;
//		}
//	}
//}
//
//int main()
//{
//	int arr[32];
//	long long temp = reinterpret_cast<long long>(arr + 16);
//	temp = temp & 0xFFFFFFFFFFFFFFC0; //64∫Ò∆Æ
//	temp = temp - 2;
//	bound = reinterpret_cast<int *>(temp);
//	*bound = 0;
//
//	thread t1{ worker0 };
//	thread t2{ worker1 };
//	t1.join();
//	t2.join();
//	cout << "Memory Error : " << error_count << endl;
//}