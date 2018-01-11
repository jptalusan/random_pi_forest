#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <omp.h>

/***
Concurrency: splits the data file into N number of files (depending on the number of nodes)
Another function should send the data once written to files.
***
*/
int generateUniqueNoInRange(std::vector<int> iv, int start, int end);
std::vector<std::string> readFileToBuffer(std::string fileName);

void concurrentReads(int numberOfThreads, std::vector<std::string> vec, std::vector<int> randomData);
//get node list from main

void saveStringVecToFile(const std::vector<std::string>& svec, int start, int end, int count, const std::vector<int>& randomData);

#endif
