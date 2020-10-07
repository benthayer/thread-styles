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
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    jobReadyCondition.wait(jobReadyLock, [] {return jobReady;});
    jobReady = false;
    Job temp = job;
    jobReadyLock.unlock();
    return job;
}

void submitResult(int x) {
    unique_lock<mutex> resultReadyLock(resultReadyMutex);
    result = x;
    resultReady = true;
    resultReadyLock.unlock();
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
    Job job = getJob();
    int x = add(job.a, job.b);
    submitResult(x);
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
