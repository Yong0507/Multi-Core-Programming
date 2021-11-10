#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

const int NUM_TEST = 1000'0000;

class NODE {
public:
	int value;
	NODE* volatile next;

	NODE() : next(nullptr) {}
	NODE(int x) : value(x), next(nullptr) {}

	~NODE() {}
};

struct STAMP_POINTER
{
	NODE* volatile ptr;
	int volatile stamp;
};

class SPLFQUEUE {
	STAMP_POINTER head, tail;

	bool CAS(NODE* volatile* addr, NODE* old_node, NODE* new_node)
	{
		int a_addr = reinterpret_cast<int> (addr);
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_int*>(a_addr), 
			reinterpret_cast<int*> (&old_node), 
			reinterpret_cast<int>(new_node));
	}
	bool STAMP_CAS(STAMP_POINTER* addr, NODE* old_node, NODE* new_node, int old_stamp, int new_stamp)
	{
		STAMP_POINTER old_p{ old_node, old_stamp };
		STAMP_POINTER new_p{ new_node, new_stamp };
		long long new_value = *(reinterpret_cast<long long*>(&new_p));

		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_llong*>(addr),
			reinterpret_cast<long long*> (&old_p), new_value);
	}
public:
	SPLFQUEUE()
	{
		head.ptr = tail.ptr = new NODE(0);
		head.stamp = tail.stamp = 0;
	}

	~SPLFQUEUE() {
		Init();
		delete head.ptr;
	}

	void Init() {
		while (head.ptr != tail.ptr) {
			NODE* p = head.ptr;
			head.ptr = head.ptr->next;
			delete p;
		}
	}

	void Enq(int x)
	{
		NODE* newnode = new NODE(x);
		while (true)
		{
			STAMP_POINTER last = tail;
			NODE* next = last.ptr->next;
			if (last.ptr != tail.ptr || last.stamp != tail.stamp) continue;
			if (nullptr == next)
			{
				if (CAS(&(last.ptr->next), nullptr, newnode))
				{
					STAMP_CAS(&tail, last.ptr, newnode, last.stamp, last.stamp + 1);
					return;
				}
			}
			else
			{
				STAMP_CAS(&tail, last.ptr, next, last.stamp, last.stamp + 1);
			}
		}
	}
	int Deq()
	{
		while (true)
		{
			STAMP_POINTER first = head;
			STAMP_POINTER last = tail;

			NODE* next = first.ptr->next;
			if (first.ptr != head.ptr || first.stamp != head.stamp) continue;
			if (nullptr == next) return -1;
			if (first.ptr == last.ptr)
			{
				STAMP_CAS(&tail, last.ptr, next, last.stamp, last.stamp + 1);
				continue;
			}
			int value = next->value;
			if (false == STAMP_CAS(&head, first.ptr, next, first.stamp, first.stamp + 1)) continue;
			delete first.ptr;
			return value;
		}
	}

	void Print20() {
		NODE* p = head.ptr->next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr)
				break;
			cout << p->value << ",";
			p = p->next;

		}
		cout << "  ";
	}
};

SPLFQUEUE myqueue;

void Benchmark(int num_threads) 
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		if ((0 == rand() % 2) || (i < 32))
			myqueue.Enq(i);
		else
			myqueue.Deq();

	}
}

int main()
{
	for (int i = 1; i <= 8; i = i * 2)
	{
		vector <thread> workers;

		myqueue.Init();
		auto start_t = system_clock::now();
		for (int j = 0; j < i; ++j)
		{
			workers.emplace_back(Benchmark, i);
		}
		for (auto& th : workers)
		{
			th.join();
		}

		//myqueue.Print20();

		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;
		cout << i << "Threads ";
		cout << "exec_time = " << duration_cast<milliseconds>(exec_t).count();
		cout << "ms\n";
	}
}