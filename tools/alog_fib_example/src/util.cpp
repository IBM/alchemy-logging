/*  *
 * IBM Confidential
 * OCO Source Materials
 *
 * 5737-C06
 * (C) Copyright IBM Corp. 2018 All Rights Reserved.
 *
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has been
 * deposited with the U.S. Copyright Office.

 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.

 *
 *  */

// Standard
#include <cstdlib>
#include <locale>
#include <sstream>

// Local
#include "util.h"

namespace util
{

std::string to_lower(const std::string& input)
{
  std::locale loc;
  std::stringstream ss;
  for (std::string::size_type i=0; i<input.length(); ++i)
    ss << std::tolower(input[i], loc);
  return ss.str();
}

std::string load_env_string(const std::string& key, const std::string& default_val)
{
  std::string out = default_val;
  if (const char* env_val = std::getenv(key.c_str()))
  {
    out = std::string(env_val);
  }
  return out;
}

bool load_env_bool(const std::string& key, bool default_val)
{
  if (const char* env_val = std::getenv(key.c_str()))
  {
    std::string lower_val = to_lower(std::string(env_val));
    return lower_val == "true" or lower_val == "1";
  }
  return default_val;
}

} // end namespace util
