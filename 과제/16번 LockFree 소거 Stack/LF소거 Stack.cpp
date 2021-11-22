#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

const int NUM_TEST = 10000000;
const int MAX_THREADS = 16;

class NODE {
public:
	int value;
	NODE* next;
	NODE() : next(nullptr) {}
	NODE(int x) : value(x), next(nullptr) {}
	~NODE() {}
};

class nullmutex
{
public:
	void lock() {}
	void unlock() {}
};

class CSTACK
{
	NODE* top;
	mutex top_lock;
public:
	CSTACK() : top(nullptr)
	{
	}
	~CSTACK() {
		Init();
	}
	void Init()
	{
		while (top != nullptr) {
			NODE* p = top;
			top = top->next;
			delete p;
		}
	}
	void Push(int x)
	{
		NODE* new_node = new NODE{ x };
		top_lock.lock();
		new_node->next = top;
		top = new_node;
		top_lock.unlock();
	}
	int Pop()
	{
		top_lock.lock();
		if (nullptr == top) {
			top_lock.unlock();
			return -2;
		}
		int value = top->value;
		NODE* t = top;
		top = top->next;
		top_lock.unlock();
		delete t;
		return value;
	}
	void Print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}
};

class BackOff_old {
	int minDelay, maxDelay;
	int limit;
public:
	BackOff_old(int min, int max)
		: minDelay(min), maxDelay(max), limit(min) {}

	void InterruptedException() {
		int delay = 0;
		if (limit != 0) delay = rand() % limit;
		limit *= 2;
		if (limit > maxDelay) limit = maxDelay;
		this_thread::sleep_for(chrono::microseconds(delay));;
	}
};

class BackOff_TSC {
	int minDelay, maxDelay;
	int limit;
public:
	BackOff_TSC(int min, int max)
		: minDelay(min), maxDelay(max), limit(min) {}

	void InterruptedException() {
		int delay = 0;
		if (limit != 0)
			delay = rand() % limit;
		limit *= 2;
		if (limit > maxDelay)
			limit = maxDelay;
		int start, current;
		_asm RDTSC;
		_asm mov start, eax;
		do {
			_asm RDTSC;
			_asm mov current, eax;
		} while (current - start < delay);
	}
};

class BackOff {
	int minDelay, maxDelay;
	int limit;
public:
	BackOff(int min, int max)
		: minDelay(min), maxDelay(max), limit(min) {}

	void InterruptedException() {
		int delay = 0;
		if (0 != limit) delay = rand() % limit;
		if (0 == delay) return;
		limit += limit;
		if (limit > maxDelay) limit = maxDelay;

		_asm mov eax, delay;
	myloop:
		_asm dec eax
		_asm jnz myloop;
	}
};


class LFSTACK
{
	NODE* volatile top;
	bool CAS_TOP(NODE* old_node, NODE* new_node) {
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(&top),
			reinterpret_cast<int*>(&old_node),
			reinterpret_cast<int>(new_node)
		);
	}
public:
	LFSTACK() : top(nullptr)
	{
	}
	~LFSTACK() {
		Init();
	}
	void Init()
	{
		while (top != nullptr) {
			NODE* p = top;
			top = top->next;
			delete p;
		}
	}
	void Push(int x)
	{
		NODE* new_node = new NODE{ x };
		while (true) {
			NODE* mytop = top;
			new_node->next = mytop;
			if (true == CAS_TOP(mytop, new_node)) return;
		}
	}
	int Pop()
	{
		while (true) {
			NODE* mytop = top;
			if (nullptr == mytop) return -2;
			NODE* next = mytop->next;
			if (top != mytop) continue;
			int value = mytop->value;
			if (true == CAS_TOP(mytop, next))
				return value;
		}
	}
	void Print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}
};

class LFBOSTACK
{
	NODE* volatile top;
	bool CAS_TOP(NODE* old_node, NODE* new_node) {
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(&top),
			reinterpret_cast<int*>(&old_node),
			reinterpret_cast<int>(new_node)
		);
	}
public:
	LFBOSTACK() : top(nullptr)
	{
	}
	~LFBOSTACK() {
		Init();
	}
	void Init()
	{
		while (top != nullptr) {
			NODE* p = top;
			top = top->next;
			delete p;
		}
	}
	void Push(int x)
	{
		BackOff bo{ 1, 100 };
		NODE* new_node = new NODE{ x };
		while (true) {
			NODE* mytop = top;
			new_node->next = mytop;
			if (true == CAS_TOP(mytop, new_node))
				return;
			else bo.InterruptedException();
		}
	}
	int Pop()
	{
		BackOff bo{ 1, 100 };
		while (true) {
			NODE* mytop = top;
			if (nullptr == mytop) return -2;
			NODE* next = mytop->next;
			if (top != mytop) continue;
			int value = mytop->value;
			if (true == CAS_TOP(mytop, next))
				return value;
			else bo.InterruptedException();
		}
	}
	void Print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}
};

constexpr int EMPTY = 0;
constexpr int WAITING = 1;
constexpr int BUSY = 2;

class LockFreeExchanger
{
	atomic<unsigned int> slot;
public:
	int exchange(int value, bool* time_out, bool* busy)
	{


		while (true) {
			unsigned int cur_slot = slot;
			int value = cur_slot & 0xCFFFFFFF;
			int stat = cur_slot >> 30;
			switch (stat) {
			case EMPTY: {
				unsigned int new_value = value | (WAITING << 30);
				if (true == atomic_compare_exchange_strong(&slot, &cur_slot, new_value)) {
					for (int i = 0; i < 10; ++i) {
						if (BUSY == slot >> 30) {
							int ret_value = slot & 0xCFFFFFFF;
							slot = 0;
							return ret_value;
						}
					}
					*time_out = true;
					*busy = false;
					return 0;
				}
				else
					continue;
			}
			case WAITING: {
				int new_value = value | (BUSY << 30);
				if (true == atomic_compare_exchange_strong(&slot, &cur_slot, new_value)) {
					return value;
				}
				else {
					*time_out = false;
					*busy = true;
					return 0;
				}
			}
			case BUSY:
				*time_out = false;
				*busy = true;
				return 0;
			}
		}
	}
};

class EliminationArray {
	int _num_threads;
	int range;
	LockFreeExchanger exchanger[MAX_THREADS];
public:
	EliminationArray() : range(1) {}
	~EliminationArray() {}
	int Visit(int value, bool* time_out) {
		int slot = rand() % range;
		bool busy;
		int ret = exchanger[slot].exchange(value, time_out, &busy);
		if ((true == *time_out) && (range > 1)) range--;
		if ((true == busy) && (range <= _num_threads / 2)) range++;
		// MAX RANGE is # of thread / 2
		return ret;
	}
};

EliminationArray el;

class LFELSTACK
{
	NODE* volatile top;

	bool CAS_TOP(NODE* old_node, NODE* new_node) {
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(&top),
			reinterpret_cast<int*>(&old_node),
			reinterpret_cast<int>(new_node)
		);
	}
public:
	LFELSTACK() : top(nullptr)
	{
	}
	~LFELSTACK() {
		Init();
	}
	void Init()
	{
		while (top != nullptr) {
			NODE* p = top;
			top = top->next;
			delete p;
		}
	}
	void Push(int x)
	{
		EliminationArray el;
		NODE* new_node = new NODE{ x };
		while (true) {
			NODE* mytop = top;
			new_node->next = mytop;
			if (true == CAS_TOP(mytop, new_node))
				return;
			else {
				bool time_out = false;
				int ret = el.Visit(x, &time_out);
				if ((false == time_out) && (0 != ret))  
					return;
				
			}
		}
	}
	int Pop()
	{
		EliminationArray el;
		while (true) {
			NODE* mytop = top;
			if (nullptr == mytop) return -2;
			NODE* next = mytop->next;
			if (top != mytop) continue;
			int value = mytop->value;
			if (true == CAS_TOP(mytop, next))
				return value;
			else {
				bool time_out = false;
				int ret = el.Visit(value, &time_out);
				if ((false == time_out) && (0 != ret)) {
					return value;
				}
			}
		}
	}
	void Print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr)
				break;
			cout << p->value << ", ";
			p = p->next;
		}
	}
};

LFELSTACK mystack;

void Benchmark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
		if ((0 == rand() % 2) || (i < 1000 / num_threads))
			mystack.Push(i);
		else
			mystack.Pop();
}

int main()
{
	for (int i = 1; i <= 8; i = i * 2) {
		vector <thread> workers;
		mystack.Init();
		auto start_t = system_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(Benchmark, i);
		for (auto& th : workers)
			th.join();
		auto end_t = system_clock::now();
		auto exec_t = end_t - start_t;

		mystack.Print20();
		cout << endl;
		cout << i << " threads";
		cout << "    exec_time = " << duration_cast<milliseconds>(exec_t).count();
		cout << "ms\n";
	}
}