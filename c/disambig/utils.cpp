#include <string>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <sstream>
#include <json/json.h>
#include "m2pp.hpp"
#include "m2pp_internal.hpp"

namespace m2pp {

namespace utils {

static int decode_uri(const char *uri, size_t length, char *ret, int always_decode_plus);

std::vector<std::string> split(const std::string& str, const std::string& sep, unsigned int count) {
	std::vector<std::string> result;
	std::string::size_type last_pos = str.find_first_not_of(sep, 0);
	std::string::size_type pos = str.find_first_of(sep, last_pos);
	int i = count;

	while (std::string::npos != pos || std::string::npos != last_pos) {
		result.push_back(str.substr(last_pos, pos - last_pos));
		last_pos = str.find_first_not_of(sep, pos);
		pos = str.find_first_of(sep, last_pos);
		if (count > 0) {
			i--;
			if (i==0) {
				result.push_back(str.substr(last_pos, str.length() - last_pos));
				break;
			}
		}
	}

	return result;
}

std::string parse_netstring(const std::string& str, std::string& rest) {
	std::vector<std::string> result = split(str, ":", 1);
	std::istringstream is(result[0]);
	unsigned int len;
	is >> len;
	rest = result[1].substr(len+1, result[1].length() - len);
	return result[1].substr(0, len);
}

void parse_json(const std::string& jsondoc, param_map_t& headers) {
	json_object * jobj = json_tokener_parse(jsondoc.c_str());

	if (jobj && json_object_is_type(jobj, json_type_object)) {
		json_object_object_foreach(jobj, key, value) {
			if (key && value && json_object_is_type(value, json_type_string)) {
        headers[key] = json_object_get_string(value);
				//hdrs.push_back(header(key, json_object_get_string(value)));
			}
		}
	}

	json_object_put(jobj); // free json object
}

void parse_query(const std::string& query, std::map<std::string,std::string>& params)
{
	char *line;
	char *argument;
	char *p;

	// No arguments - we are done
	//if (strchr(uri, '?') == NULL)
//		return;

  line = strdup(query.c_str());
  if (line == NULL)
    return;

	argument = line;

	// We already know that there has to be a ?
	//strsep(&argument, "?");

	p = argument;
	while (p != NULL && *p != '\0') {
		char *key, *value, *decoded_value;
		argument = strsep(&p, "&");

		value = argument;
		key = strsep(&value, "=");
		if (value == NULL)
			goto error;

		decoded_value = (char*) malloc(strlen(value) + 1);
		decode_uri(value, strlen(value), decoded_value, 1); // the last argument is always decode a +
    params[key] = decoded_value;
		free(decoded_value);
	}

 error:
	free(line);
}

static int decode_uri(const char *uri, size_t length, char *ret, int always_decode_plus)
{
	char c;
	int i, j, in_query = always_decode_plus;
	
	for (i = j = 0; uri[i] != '\0'; i++) {
		c = uri[i];
		if (c == '?') {
			in_query = 1;
		} else if (c == '+' && in_query) {
			c = ' ';
		} else if (c == '%' && isxdigit((unsigned char)uri[i+1]) &&
		    isxdigit((unsigned char)uri[i+2])) {
			char tmp[] = { uri[i+1], uri[i+2], '\0' };
			c = (char)strtol(tmp, NULL, 16);
			i += 2;
		}
		ret[j++] = c;
	}
	ret[j] = '\0';

	return (j);
}

}

}
