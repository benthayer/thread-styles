#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>

using namespace std;

struct NoMoreJobsException : public exception {
   const char * what () const throw () {
      return "No more jobs!";
   }
};
struct Job {
    int a;
    int b;
};

Job job;
bool noMoreJobs = false;
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

void setJob(Job x) {
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    job = x;
    jobReady = true;
    jobReadyLock.unlock();
    jobReadyCondition.notify_all();
}

void waitForJob() {
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    jobReadyCondition.wait(jobReadyLock, [] {return jobReady;});

    if (noMoreJobs) {
        throw NoMoreJobsException();
    }
    jobReady = false;

    jobReadyLock.unlock();
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
    while (true) {
        try {
            waitForJob();
        } catch (NoMoreJobsException& x) {
            break;
        }
        cout << "Got a job!" << endl;
        int x = add(job.a, job.b);
        submitResult(x);
    }
}

void setNoMoreJobs() {
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    noMoreJobs = true;
    jobReady = true;
    jobReadyCondition.notify_all();
    cout << "No more jobs" << endl;
}

int main () {
    thread t(worker);
    cout << "Hello" << endl;

    setJob(Job{3,4});
    int i = getResult();
    cout << "Result is " << to_string(i) << endl;

    setJob(Job{10,15});
    i = getResult();
    cout << "Result is " << to_string(i) << endl;

    setNoMoreJobs();

    t.join();
    return 0;
}
