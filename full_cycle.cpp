#include <iostream>
#include <vector>
#include <thread>
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

bool noMoreJobs = false;
mutex jobReadyMutex;
condition_variable jobReadyCondition;

vector<thread> workers;

Job* jobArray;
bool* jobReadyArray;

int* resultArray;
bool* resultReadyArray;
mutex* resultReadyMutexArray;
condition_variable* resultReadyConditionArray;

int add(int a, int b) {
    return a + b;
}

void setJob(int i, Job j) {
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    jobArray[i] = j;
    jobReadyArray[i] = true;
    jobReadyLock.unlock();
    jobReadyCondition.notify_all();
}

void waitForJob(int i) {
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    jobReadyCondition.wait(jobReadyLock, [i] {return jobReadyArray[i];});

    if (noMoreJobs) {
        throw NoMoreJobsException();
    }
    jobReadyArray[i] = false;

    jobReadyLock.unlock();
}

void submitResult(int i, int result) {
    unique_lock<mutex> resultReadyLock(resultReadyMutexArray[i]);
    resultArray[i] = result;
    resultReadyArray[i] = true;
    resultReadyLock.unlock();
    resultReadyConditionArray[i].notify_all();
}


int getResult(int i) {
    unique_lock<mutex> resultReadyLock(resultReadyMutexArray[i]);
    resultReadyConditionArray[i].wait(resultReadyLock, [i] {return resultReadyArray[i];});
    resultReadyArray[i] = false;
    int temp = resultArray[i];
    resultReadyLock.unlock();
    return temp;
}

void worker(int i) {
    while (true) {
        try {
            waitForJob(i);
        } catch (NoMoreJobsException& x) {
            break;
        }
        cout << "Got a job on thread " << to_string(i) << "!" << endl;
        int x = add(jobArray[i].a, jobArray[i].b);
        submitResult(i, x);
    }
}

void setNoMoreJobs() {
    unique_lock<mutex> jobReadyLock(jobReadyMutex);
    noMoreJobs = true;
    for (int i = 0; i < workers.size(); i++) {
        jobReadyArray[i] = true;
    }
    jobReadyCondition.notify_all();
    cout << "No more jobs" << endl;
}

void startWorkers(int numWorkers) {
    jobArray = new Job[numWorkers];
    jobReadyArray = new bool[numWorkers];
    resultArray = new int[numWorkers];
    resultReadyArray = new bool[numWorkers];
    resultReadyMutexArray = new mutex[numWorkers];
    resultReadyConditionArray = new condition_variable[numWorkers];;

    for (int i = 0; i < numWorkers; i++) {
        workers.push_back(thread(worker, i));
    }
}

void stopWorkers() {
    setNoMoreJobs();
    for (int i = 0; i < workers.size(); i++) {
        workers[i].join();
    }
}

int currentJobNum = 0;
vector<Job> jobQueue{Job{3,4}, Job{10,15}, Job{30, 20}, Job{1,2}};

bool jobsLeft () {
    return currentJobNum < jobQueue.size();
}

Job getNextJob () {
    return jobQueue[currentJobNum++];
}

int main () {
    int maxThreads = 2;
    startWorkers(maxThreads);
    cout << "Hello" << endl;

    int threadNum = 0;
    while (jobsLeft()) {
        Job j = getNextJob();
        setJob(threadNum, j);
        int result = getResult(threadNum);
        cout << "Result is " << to_string(result) << endl;
        threadNum = (threadNum + 1) % maxThreads;
    }

    stopWorkers();

    return 0;
}
