#include "utils.h"

#include <cstring>
#include <sstream>

#include "fmt/format.h"
#include "glog/logging.h"
#include "curl/curl.h"

//typedef struct curl_read_userdata_
//{
//    const std::string & req_data;
//    size_t pos;
//} curl_read_userdata;

static size_t write_string_callback(const void * bufptr, size_t size, size_t nitems, void * userp)
{
    if (nullptr == bufptr || nullptr == userp)
    {
        LOG(WARNING) << "Invalid curl write callback function params bufptr and/or userp!";
        return nitems;
    }
    if (size < 1 || nitems < 1)
    {
        LOG(WARNING) << "Invalid curl write callback function params, size is '" << size
        << "', nitems is '" << nitems << "'!";
        return nitems;
    }

    auto * str = reinterpret_cast<std::string *>(userp);
    str->append(reinterpret_cast<const char *>(bufptr), size * nitems);
    return nitems;
}

//static size_t read_string_callback(char * bufptr, size_t size, size_t nitems, void * userp)
//{
//    if (nullptr == bufptr || nullptr == userp)
//    {
//        LOG(WARNING) << "Invalid curl read callback function params bufptr and/pr userp!";
//        return nitems;
//    }
//    if (size < 1 || nitems < 1)
//    {
//        LOG(WARNING) << "Invalid curl read callback function params, size is '" << size
//        << "', nitems is '" << nitems << "'!";
//        return nitems;
//    }
//
//    auto * userdata = reinterpret_cast<curl_read_userdata *>(userp);
//    if (userdata->pos >= userdata->req_data.length())
//        return 0;
//    auto bytes_left = static_cast<size_t>(userdata->req_data.length() - userdata->pos);
//    auto bytes_to_copy = bytes_left < (size * nitems) ? bytes_left : (size * nitems);
//    memcpy(bufptr, userdata->req_data.data() + userdata->pos, bytes_to_copy);
//    userdata->pos += bytes_to_copy;
//    return bytes_to_copy;
//}

std::string get_version_string()
{
#if defined(PVE_DDNS_CLIENT_VER)
    return PVE_DDNS_CLIENT_VER;
#else
    return "dev";
#endif
}

bool str_iequals(const std::string & l, const std::string & r)
{
    if (l.length() != r.length())
        return false;
#if WIN32
    return 0 == _stricmp(l.c_str(), r.c_str());
#else
    return 0 == strcasecmp(l.c_str(), r.c_str());
#endif
}

std::pair<std::string, std::string> get_sub_domain(const std::string & domain)
{
    size_t pos = std::string::npos;
    size_t dot_pos = std::string::npos;
    std::string token;
    while ((pos = domain.rfind('.', dot_pos)) != std::string::npos)
    {
        if (dot_pos != std::string::npos)
            break;
        dot_pos = pos - 1;
        pos = std::string::npos;
    }
    if (std::string::npos == pos)
        return { domain, "" };
    return { domain.substr(pos + 1), domain.substr(0, pos) };
}

size_t get_dns_service_key(const std::string & dns_type, const std::string & credentials)
{
    std::hash<std::string> str_hash;
    return str_hash(fmt::format("{}:{}", dns_type, credentials));
}

// Function to check if the given string s is IPv4 or not
bool is_ipv4(const std::string & s)
{
    // Store the count of occurrence
    // of '.' in the given string
    int cnt = 0;
  
    // Traverse the string s
    for (int i = 0; i < s.size(); i++)
    {
        if (s[i] == '.')
            cnt++;
    }
  
    // Not a valid IP address
    if (cnt != 3)
        return false;
  
    // Stores the tokens
    std::vector<std::string> tokens;
  
    // stringstream class check1
    std::stringstream check1(s);
    std::string intermediate;
  
    // Tokenizing w.r.t. '.'
    while (getline(check1, intermediate, '.'))
    {
        tokens.push_back(intermediate);
    }
  
    if (tokens.size() != 4)
        return false;
  
    // Check if all the tokenized strings
    // lies in the range [0, 255]
    for (int i = 0; i < tokens.size(); i++) 
    {
        int num = 0;
  
        // Base Case
        if (tokens[i] == "0")
            continue;
  
        if (tokens[i].empty())
            return false;
  
        for (int j = 0; j < tokens[i].size(); j++)
        {
            if (tokens[i][j] > '9' || tokens[i][j] < '0')
                return false;
  
            num *= 10;
            num += tokens[i][j] - '0';
  
            if (num == 0)
                return false;
        }
  
        // Range check for num
        if (num > 255 || num < 0)
            return false;
    }
  
    return true;
}
  
// Function to check if the string represents a hexadecimal number
bool check_hex(const std::string & s)
{
    // Size of string s
    int n = static_cast<int>(s.length());
  
    // Iterate over string
    for (int i = 0; i < n; i++)
    {
        char ch = s[i];
  
        // Check if the character is invalid
        if ((ch < '0' || ch > '9')
            && (ch < 'A' || ch > 'F')
            && (ch < 'a' || ch > 'f'))
        {
            return false;
        }
    }
  
    return true;
}
  
// Function to check if the given string S is IPv6 or not
bool is_ipv6(const std::string & s)
{
    // Store the count of occurrence
    // of ':' in the given string
    int cnt = 0;
  
    for (int i = 0; i < s.size(); i++)
    {
        if (s[i] == ':')
            cnt++;
    }
  
    // Not a valid IP Address
    if (cnt != 7)
        return false;
  
    // Stores the tokens
    std::vector<std::string> tokens;
  
    // stringstream class check1
    std::stringstream check1(s);
    std::string intermediate;
  
    // Tokenizing w.r.t. ':'
    while (getline(check1, intermediate, ':'))
    {
        tokens.push_back(intermediate);
    }
  
    if (tokens.size() != 8)
        return false;
  
    // Check if all the tokenized strings
    // are in hexadecimal format
    for (int i = 0; i < tokens.size(); i++)
    {
        int len = static_cast<int>(tokens[i].size());
  
        if (!check_hex(tokens[i]) || len > 4 || len < 1)
        {
            return false;
        }
    }
    return true;
}

bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers,
              int & resp_code, std::string & resp_data)
{
    return http_req(url, req_data, timeout_ms, custom_headers, "", resp_code, resp_data);
}

bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers, const std::string & method,
              int & resp_code, std::string & resp_data)
{
    CURL * curl = curl_easy_init();
    if (nullptr == curl)
    {
        LOG(ERROR) << "Failed to curl_easy_init!";
        return false;
    }
//    curl_read_userdata read_userdata = { req_data, 0 };
    curl_easy_setopt(curl, CURLoption::CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLoption::CURLOPT_TIMEOUT_MS, timeout_ms);
    if (!req_data.empty())
    {
        if ("put" == method)
        {
            curl_easy_setopt(curl, CURLoption::CURLOPT_CUSTOMREQUEST, "PUT");
//            curl_easy_setopt(curl, CURLoption::CURLOPT_UPLOAD, 1L);
//            curl_easy_setopt(curl, CURLoption::CURLOPT_READFUNCTION, read_string_callback);
//            curl_easy_setopt(curl, CURLoption::CURLOPT_READDATA, static_cast<void *>(&read_userdata));
//            curl_easy_setopt(curl, CURLoption::CURLOPT_INFILESIZE_LARGE, req_data.length());
        }
        else if ("delete" == method)
            curl_easy_setopt(curl, CURLoption::CURLOPT_CUSTOMREQUEST, "DELETE");
        else
            curl_easy_setopt(curl, CURLoption::CURLOPT_POST, 1L);

        curl_easy_setopt(curl, CURLoption::CURLOPT_POSTFIELDS, req_data.c_str());
        curl_easy_setopt(curl, CURLoption::CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(req_data.length()));
    }
    else
    {
        if ("put" == method)
            curl_easy_setopt(curl, CURLoption::CURLOPT_CUSTOMREQUEST, "PUT");
        else if ("delete" == method)
            curl_easy_setopt(curl, CURLoption::CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    curl_easy_setopt(curl, CURLoption::CURLOPT_WRITEFUNCTION, write_string_callback);
    curl_easy_setopt(curl, CURLoption::CURLOPT_WRITEDATA, static_cast<void *>(&resp_data));

    bool ret = false;
    do
    {
        char errbuf[CURL_ERROR_SIZE] = {};
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
//#ifndef NDEBUG
//        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//#endif
        curl_slist * http_headers = nullptr;
        for (const auto & custom_header : custom_headers)
        {
            http_headers = curl_slist_append(http_headers, custom_header.c_str());
//            http_headers = curl_slist_append(http_headers, "Accept: application/json");
//            http_headers = curl_slist_append(http_headers, "Content-Type: application/json");
//            http_headers = curl_slist_append(http_headers, "charset: utf-8");
        }
        if (nullptr != http_headers)
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);

        CURLcode curl_ret = curl_easy_perform(curl);
        if (curl_ret != CURLcode::CURLE_OK)
        {
            LOG(WARNING) << "curl_easy_perform fail, curl code is '" << curl_ret << "', error is '" << errbuf
            << "', url is '" << url << "'!";
            break;
        }
        ret = true;

        long code = 0;
        curl_ret = curl_easy_getinfo(curl, CURLINFO::CURLINFO_RESPONSE_CODE, &code);
        if (CURLcode::CURLE_OK != curl_ret)
        {
            LOG(WARNING) << "curl_easy_getinfo fail, curl_code is '" << curl_ret << "', error is '" << errbuf << "'!";
            break;
        }
        resp_code = static_cast<int>(code);
        if (resp_code != 200)
            LOG(WARNING) << "'" << url << "' request failed, response code is '" << resp_code << "'!";
    } while (false);

    curl_easy_cleanup(curl);

    return ret;
}
