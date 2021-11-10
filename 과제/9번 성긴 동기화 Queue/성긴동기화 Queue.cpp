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

class CQUEUE {
    NODE *head, *tail;
    mutex enq_lock;
    mutex deq_lock;

public:
    CQUEUE()
    {
        head = tail = new NODE(0);
    }

    ~CQUEUE() {
        Init();
        delete head;
    }

    void Init() {
        // head와 tail이 만나면 보초노드 빼고 다 지운 것
        while (head != tail) {
            NODE* p = head;
            head = head->next;
            delete p;
        }
    }

    void Enq(int x)
    {
        enq_lock.lock();
        NODE* e = new NODE(x);
        tail->next = e;
        tail = e;
        enq_lock.unlock();
    }

    int Deq()
    {
        int result;
        deq_lock.lock();

        if (head->next == nullptr)
        {
            deq_lock.unlock();
            return - 1;
        }
        //if (head == tail) {
        //    deq_lock.unlock();
        //    return -1;
        //}

        
        NODE* node = head;
        result = head->next->value;
        head = head->next;
        delete node;
        deq_lock.unlock();
        return result;
        
    }

    void Print20() 
    {
        NODE* p = head->next;
        for (int i = 0; i < 20; ++i) {
            if (p == nullptr)
                break;
            cout << p->value << ",";
            p = p->next;
        }
        cout << "  ";
    }
};

CQUEUE myqueue;

void Benchmark_CQueue(int num_threads)
{
    for (int i = 0; i < NUM_TEST / num_threads; ++i) {
        if ((0 == rand() % 2) || (i < 32 / num_threads))
            myqueue.Enq(i);
        else
            myqueue.Deq();
    }
}

int main()
{
    for (int i = 1; i <= 8; i *= 2) {
        vector<thread> workers;
        myqueue.Init();

        auto start_t = system_clock::now();
        for (int j = 0; j < i; ++j)
            workers.emplace_back(Benchmark_CQueue, i);
        for (auto& th : workers)
            th.join();
        auto end_t = system_clock::now();
        auto exec_t = end_t - start_t;

        //myqueue.Print20();

        cout << i << " threads";
        cout << " exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
    }
}