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
#pragma once

// Standard
#include <string>

namespace util
{

/// Convert a string to lowercase
std::string to_lower(const std::string& input);

/// Pull a string value from the environment
std::string load_env_string(const std::string& key, const std::string& default_val);

/// Pull a bool value from the environment
bool load_env_bool(const std::string& key, bool default_val);

} // end namespace util
