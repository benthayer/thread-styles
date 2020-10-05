#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>

using namespace std;

mutex coutMutex;

mutex workerReadyMutex;
condition_variable workerReadyCondition;
int numReady = 0;

void readyUp(int i) {
    unique_lock<mutex> lck(workerReadyMutex);
    numReady++;
    int r = numReady;
    coutMutex.lock();
    cout << "We're ready: " << to_string(r) << "(" << to_string(i) << ")" << endl;
    coutMutex.unlock();
    workerReadyCondition.notify_all();
}

mutex workMutex;
bool workAvailable = true;

mutex workerCountMutex;
int numWorking = 0;
condition_variable workerCountCondition;

bool workIsAvailable() {
    lock_guard<mutex> lg(workMutex);
    return workAvailable;
}

void randomPause()
{
    std::mt19937_64 eng{std::random_device{}()};  // or seed however you want
    std::uniform_int_distribution<> dist{10, 100};
    std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
}

void startWork() {
    unique_lock<mutex> lck(workerCountMutex);
    numWorking++;
}
void stopWork() {
    unique_lock<mutex> lck(workerCountMutex);
    numWorking--;
    workerCountCondition.notify_one();
}

void work1(int i) {
    readyUp(i);
    unique_lock<mutex> startLock(workerReadyMutex);
    coutMutex.lock();
    cout << "I guess I'm waiting" << endl;
    coutMutex.unlock();
    startLock.unlock();
    this_thread::sleep_for(chrono::milliseconds(100));
    startLock.lock();
    workerReadyCondition.wait(startLock, [] { coutMutex.lock(); cout << "Checking from thread 1" << endl; coutMutex.unlock(); return numReady == 2;});
    startLock.unlock();
    coutMutex.lock();
    cout << "Time to work" << endl;
    coutMutex.unlock();
    while (workIsAvailable()) {
        startWork();
        randomPause();
        coutMutex.lock();
        cout << "Locking with thread " << to_string(i) << endl;
        coutMutex.unlock();
        stopWork();
    }
}

void work2(int i) {
    readyUp(i);
    unique_lock<mutex> startLock(workerReadyMutex);
    coutMutex.lock();
    cout << "I guess I'm waiting" << endl;
    coutMutex.unlock();
    workerReadyCondition.wait(startLock, [] { coutMutex.lock(); cout << "Checking from thread 2" << endl; coutMutex.unlock(); return numReady == 2;});
    startLock.unlock();
    coutMutex.lock();
    cout << "Time to work" << to_string(i) << endl;
    coutMutex.unlock();
    while (workIsAvailable()) {
        startWork();
        randomPause();
        coutMutex.lock();
        cout << "Locking with thread " << to_string(i) << endl;
        coutMutex.unlock();
        stopWork();
    }
}


int main() {
    thread t1(work1, 1);
    thread t2(work2, 2);

    // Give workers a chance to start up
    unique_lock<mutex> startLock(workerReadyMutex, defer_lock);
    workerReadyCondition.wait(startLock, [] {return numReady == 2;}); // Workers are notified too

    coutMutex.lock();
    cout << "Started" << endl;
    coutMutex.unlock();
    // Alternatively, we could be the only thread waiting and
    // then do some prep before we notify the other threads

    // Work for a bit. Each will have a random loop
    this_thread::sleep_for(chrono::seconds(1));

    // Stop them, but don't join
    workMutex.lock();
    workAvailable = false;
    workMutex.unlock();

    // How do we know when all of them have stopped?
    unique_lock<mutex> finishLock(workerCountMutex);
    workerCountCondition.wait(finishLock, [] {return numWorking == 0;});
    finishLock.unlock();
    coutMutex.lock();
    cout << "Done" << endl;
    coutMutex.unlock();

    t1.join();
    t2.join();
    return 0;
}
