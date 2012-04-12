/** 
 * @file base64.h
 * @brief base64 en/de-coding
 *
 * Based on the Pion Network Library http://www.pion.org/files/pion-net-4.0.9.tar.bz2
 *
 * Copyright (C) 2007-2011 Atomic Labs, Inc.  (http://www.atomiclabs.com)
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 *
 * Minor changes Copyright (C) Armin.Weatherwax (at) googlemail.com
 */

#ifndef BASE_64_H
#define BASE_64_H
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include "llpreprocessor.h"
class LL_COMMON_API Base64
{
public:
            /** decodes base64-encoded strings
             *
             * @param input base64 encoded string
             * @param output decoded string ( may include non-text chars)
             * @return true if successful, false if input string contains non-base64 symbols
             */
              static bool const decode(std::string const &input, std::vector<unsigned char>& output);
        
            /** encodes strings using base64
             *
             * @param input arbitrary string ( may include non-text chars)
             * @param output base64 encoded string
             * @return true if successful
             */
             static bool const encode(std::vector<unsigned char> const &input, std::string& output);

             static bool const encode(unsigned int input_length, const unsigned char * input_ptr, std::string& output);
};

#endif // BASE_64_H

