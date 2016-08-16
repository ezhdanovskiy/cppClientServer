#pragma once

#include <iostream>

#define LOG(chain) std::cout << chain << "\t\t" << __FILE__ << ":" << __LINE__ <<std::endl;
#define LOG1(var) std::cout << #var << "=" << var << "\t\t" << __FILE__ << ":" << __LINE__ <<std::endl;
#define CHECK_INT_ERR(var) if (var < 0) { err(1, "%s = %d\t%s:%d", #var, var, __FILE__, __LINE__); }
#define CHECK_LONG_ERR(var) if (var < 0) { err(1, "%s = %ld\t%s:%d", #var, var, __FILE__, __LINE__); }
