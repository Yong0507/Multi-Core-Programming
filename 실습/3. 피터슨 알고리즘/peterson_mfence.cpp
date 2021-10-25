//#include <iostream>
//#include <mutex>
//#include <thread>
//#include <chrono>
//#include <vector>
//#include <atomic>
//using namespace std;
//using namespace chrono;
//
//volatile int sum;
//
//mutex myl;
//
//volatile bool flag[2] = { false,false };
//volatile int victim = 0;
//
////atomic <bool> flag[2] = { false,false };
////atomic <int> victim = 0;
//
//
//void p_lock(int my_id)
//{
//	int other = 1 - my_id;   // 내가 0이면 상대방 1이고, 내가 1이면 상대방 0이다.
//	flag[my_id] = true;
//	victim = my_id;
//	//_asm mfence; // 실행 순서 오동작을 막아줌
//	atomic_thread_fence(std::memory_order_seq_cst);
//	while ((flag[other] == true) && (my_id == victim)); // 상대방이 true이고 내가 양보자라면 그때만 실행하겠다.
//}
//
//void p_unlock(int my_id)
//{
//	flag[my_id] = false;
//}
//
//void worker(int num_threads, int my_id)
//{
//	const int loop_count = 5000'0000 / num_threads;
//	for (auto i = 0; i < loop_count ; ++i) {
//		p_lock(my_id);
//		//myl.lock();
//		sum += 2;
//		//myl.unlock();
//		p_unlock(my_id);
//	}
//}
//
//int main()
//{
//	for (int i = 2; i <= 2; i += 2) {
//
//		sum = 0;
//		vector<thread> workers;
//		auto start_t = high_resolution_clock::now();
//
//		for (int j = 0; j < i; ++j)
//			workers.emplace_back(thread{ worker, i,j });
//		for (auto & th : workers) 
//			th.join();
//		
//		auto end_t = high_resolution_clock::now();
//		auto du_t = end_t - start_t;
//
//		cout << "number of threads = " << i;
//		cout << "    exec_time = " << duration_cast<milliseconds>(du_t).count();
//		cout << "    Sum = " << sum << endl;
//	}
//} 