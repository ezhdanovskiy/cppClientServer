#pragma once

#include <iostream>

#define LOG(chain) std::cout << addCurrTime() << chain << "\t\t" << __FILE__ << ":" << __LINE__ <<std::endl;
#define LOG1(var)  std::cout << addCurrTime() << #var << "=" << var << "\t\t" << __FILE__ << ":" << __LINE__ <<std::endl;
#define CHECK_INT_ERR(var)  if (var < 0) { err(1, "%s = %d\t%s:%d", #var, var, __FILE__, __LINE__); }
#define CHECK_LONG_ERR(var) if (var < 0) { err(1, "%s = %ld\t%s:%d", #var, var, __FILE__, __LINE__); }

static std::string addCurrTime()
{
	time_t currTime = time(0);
	struct tm* timeinfo;
	timeinfo = localtime(&currTime);
	char buffer[50];
	strftime(buffer, 50, "%I:%M:%S  ", timeinfo);
    return buffer;
}