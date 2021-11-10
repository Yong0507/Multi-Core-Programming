#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

const int NUM_TEST = 4000'000;
const int RANGE = 1000;

class nullmutex
{
public:
    void lock() {}
    void unlock() {}
};

class LFNODE;

// 마킹과 포인터 합성 자료구조
class MPTR {
    atomic_int V;
public:
    MPTR() : V(0) {}
    ~MPTR() {}

    void set_ptr(LFNODE* p)
    {
        V = reinterpret_cast<int>(p);
    }

    LFNODE* get_ptr()
    {
        return reinterpret_cast<LFNODE*>(V & 0xFFFFFFFE);
        // 포인터 값만 깔끔하게 return
    }

    bool get_removed(LFNODE*& p)
    {
        int curr_v = V;
        p = reinterpret_cast<LFNODE*>(curr_v & 0xFFFFFFFE);
        return (curr_v & 0x1) == 1;   // 1이면 remove 된 것
    }

    bool CAS(LFNODE* old_p, LFNODE* new_p, bool old_removed, bool new_removed)
    {
        int old_v = reinterpret_cast<int>(old_p);
        if (true == old_removed) old_v++;
        int new_v = reinterpret_cast<int>(new_p);
        if (true == new_removed) new_v++;
        return atomic_compare_exchange_strong(&V, &old_v, new_v);

    }
};

class LFNODE {
public:
    int value;
    MPTR next;

    LFNODE() {}
    LFNODE(int x) : value(x) {}
    ~LFNODE() {}
};


class LFLIST {
    LFNODE head, tail;

public:
    LFLIST()
    {
        head.value = 0x80000000;
        tail.value = 0x7FFFFFFF;
        head.next.set_ptr(&tail);
    }

    ~LFLIST() {
        Init();
    }

    void Init() {
        while (head.next.get_ptr() != &tail) {
            LFNODE* p = head.next.get_ptr();
            head.next.set_ptr(p->next.get_ptr());
            delete p;
        }
    }

    // 찌꺼기가 있으면 치우면서 전진해라
    void Find(int x, LFNODE*& pred, LFNODE*& curr)
    {
        while (true) {
        retry:
            pred = &head;
            curr = pred->next.get_ptr();
            while (true) {
                // curr 쓰레기인지 확인 후 
                LFNODE* succ;
                bool removed = curr->next.get_removed(succ);

                // curr 쓰레기이면 제거
                while (true == removed) {
                    if (false == pred->next.CAS(curr, succ, false, false))
                        goto retry;
                    removed = curr->next.get_removed(succ);
                }

                if (curr->value >= x)
                    return;
                pred = curr;
                curr = curr->next.get_ptr();
            }
        }
    }

    bool Add(int x)
    {
        LFNODE* pred, * curr;
        LFNODE* new_node = new LFNODE(x);
        while (true) {
            Find(x, pred, curr);
            if (curr->value == x) {
                delete new_node;
                return false;
            }
            else {
                new_node->next.set_ptr(curr);   
                if (true == pred->next.CAS(curr, new_node, false, false))
                    return true;
            }
        }
    }

    bool Remove(int x)
    {
        LFNODE* pred, * curr;
        while (true) {
            Find(x, pred, curr);

            if (curr->value != x)
                return false;
            else {
                LFNODE* succ = curr->next.get_ptr();
                if (false == curr->next.CAS(succ, succ, false, true))
                    continue;

                pred->next.CAS(curr, succ, false, false);
                return true;
            }
        }
    }

    bool Contains(int x)
    {
        LFNODE* curr = &head;
        bool removed = false;
        while (curr->value < x) {
            removed = curr->next.get_removed(curr);
        }
        return (curr->value == x) && (false == removed);
    }

    void Print20() {
        LFNODE* p = head.next.get_ptr();
        for (int i = 0; i < 20; ++i)
        {
            if (p == &tail)
                break;
            cout << p->value << ",";
            p = p->next.get_ptr();

        }
        cout << "  ";
    }
};


LFLIST lflist;

void Benchmark_LFList(int num_threads)
{
    for (int i = 0; i < NUM_TEST / num_threads; ++i) {
        int x = rand() % RANGE;
        switch (rand() % 3) {
        case 0:
            lflist.Add(x);
            break;
        case 1:
            lflist.Remove(x);
            break;
        case 2:
            lflist.Contains(x);
            break;
        default:
            break;
        }
    }
}

int main()
{
    for (int i = 1; i <= 8; i *= 2) {
        vector<thread> workers;
        lflist.Init();

        auto start_t = system_clock::now();
        for (int j = 0; j < i; ++j)
            workers.emplace_back(Benchmark_LFList, i);
        for (auto& th : workers)
            th.join();
        auto end_t = system_clock::now();
        auto exec_t = end_t - start_t;

        //lflist.Print20();

        cout << i << " threads";
        cout << " exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
    }
}