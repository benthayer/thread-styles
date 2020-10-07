#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>

using namespace std;

struct Job {
    int a;
    int b;
};

Job job;
bool jobReady = false;
mutex jobReadyMutex;
condition_variable jobReadyCondition;

int result;
bool resultReady = false;
mutex resultReadyMutex;
condition_variable resultReadyCondition;

int add(int a, int b) {
    return a + b;
}

void submitJob(Job x) {
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    job = x;
    jobReady = true;
    jobReadyLock.unlock();
    jobReadyCondition.notify_all();
}

Job getJob() {
    jobReady = false;
    return job;
}

void submitResult(int x) {
    result = x;
    resultReady = true;
    resultReadyCondition.notify_all();
}


int getResult() {
    unique_lock<mutex> resultReadyLock(resultReadyMutex);
    resultReadyCondition.wait(resultReadyLock, [] {return resultReady;});
    resultReady = false;
    int temp = result;
    resultReadyLock.unlock();
    return temp;
}

void worker() {
    cout << "Working!" << endl;
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    jobReadyCondition.wait(jobReadyLock, [] {return jobReady;});
    Job job = getJob();
    jobReadyLock.unlock();

    int x = add(job.a, job.b);

    unique_lock<mutex> resultReadyLock(resultReadyMutex);
    submitResult(x);
    resultReadyLock.unlock();
}

int main () {
    thread t(worker);
    cout << "Hello" << endl;

    submitJob(Job{3,4});
    int i = getResult();

    cout << "Result is " << to_string(i) << endl;

    t.join();
    return 0;
}
