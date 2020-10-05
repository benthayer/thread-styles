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
int numWorkers = 2;
int numReady = 0;

void readyUp(int i) {
    unique_lock<mutex> lck(workerReadyMutex);
    numReady++;
    int r = numReady;
    coutMutex.lock();
    cout << "Ready: " << to_string(r) << " (thread " << to_string(i) << ")" << endl;
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

void work(int i) {
    readyUp(i);

    unique_lock<mutex> startLock(workerReadyMutex);
    workerReadyCondition.wait(startLock, [i] { coutMutex.lock(); cout << "Checking CV from thread " << to_string(i) << endl; coutMutex.unlock(); return numReady == numWorkers;});
    startLock.unlock();

    coutMutex.lock();
    cout << "Time to work " << to_string(i) << endl;
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
    numWorkers = 3;
    cout << "Starting " << to_string(numWorkers) << " threads" << endl;
    thread *t = new thread[numWorkers];
    for (int i = 0; i < numWorkers; i++) {
        t[i] = thread(work, i);
    }

    // Give workers a chance to start up
    unique_lock<mutex> startLock(workerReadyMutex);
    workerReadyCondition.wait(startLock, [] {return numReady == numWorkers;}); // Workers are notified too

    coutMutex.lock();
    startLock.unlock();  // Other workers can start now, but we're the first to output
    cout << "Started" << endl;
    coutMutex.unlock();

    // Work for a bit. Every thread will complete at least one cycle
    this_thread::sleep_for(chrono::milliseconds(150));

    // Tell threads to stop
    workMutex.lock();
    workAvailable = false;
    workMutex.unlock();

    // How do we know when all of them have stopped?
    unique_lock<mutex> finishLock(workerCountMutex);
    workerCountCondition.wait(finishLock, [] {return numWorking == 0;});

    // Mutex no longer necessary
    cout << "Done" << endl;

    for (int i = 0; i < numWorkers; i++) {
        t[i].join();
    }
    return 0;
}
