#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
using namespace std;
using namespace chrono;

mutex myl;

const int MAX_THREAD = 8;
volatile bool flag[MAX_THREAD];
volatile int label[MAX_THREAD]{};
volatile int sum = 0;

bool CmpLabel(int other_id, int my_id)
{
	if (label[other_id] < label[my_id]) 
		return true;
	else if (label[other_id] == label[my_id])
	{
		if (other_id < my_id)
			return true;
		else
			return false;
	}
	else
		return false;
}

void bakery_lock(int my_id, int thread_num)
{
	flag[my_id] = true;
	int max_label = 0;
	for (int i = 0; i < thread_num; ++i) {
		if (max_label < label[i]) 
			max_label = label[i];		
	}
	atomic_thread_fence(memory_order_seq_cst);
	label[my_id] = (max_label + 1);
	atomic_thread_fence(memory_order_seq_cst);
	for (int i = 0; i < thread_num; ++i) {
		if (i == my_id) continue;
		while (flag[i] && CmpLabel(i, my_id));
	}
}

void bakery_unlock(int my_id)
{
	flag[my_id] = false;
}

void worker(int num_threads, int my_id)
{
	const int loop_count = 5000'0000 / num_threads;
	for (auto i = 0; i < loop_count ; ++i) {
		bakery_lock(my_id, num_threads);
		//myl.lock();
		sum += 2;
		//myl.unlock();
		bakery_unlock(my_id);
	}
}

int main()
{
	for (int i = 1; i <= MAX_THREAD; i *= 2) {

		sum = 0;
		for (int i = 0; i < MAX_THREAD; ++i) {
			label[i] = 0;
			flag[i] = false;
		}

		vector<thread> workers;
		auto start_t = high_resolution_clock::now();

		for (int j = 0; j < i; ++j)
			workers.emplace_back(thread{ worker, i,j });
		for (auto & th : workers) 
			th.join();
		
		auto end_t = high_resolution_clock::now();
		auto du_t = end_t - start_t;

		cout << "number of threads = " << i;
		cout << "    exec_time = " << duration_cast<milliseconds>(du_t).count();
		cout << "    Sum = " << sum << endl;
	}
}

