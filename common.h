/*
 * common.h
 *
 *  Created on: Dec 21, 2012
 *      Author: akavo
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <string>

struct Exc : public std::exception
{
   std::string s;
   Exc(std::string ss);
   virtual ~Exc() throw();
   const char* what() const throw();
};

bool isField(std::string field);


#endif /* COMMON_H_ */
