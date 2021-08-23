#include "utils.h"

#include <cstring>

#include "glog/logging.h"
#include "curl/curl.h"

static size_t write_string_callback(const void * ptr, size_t size, size_t n, void * s)
{
    if (nullptr == ptr || nullptr == s)
    {
        LOG(WARNING) << "Invalid curl write callback function params ptr and/or s!";
        return n;
    }
    if (size < 1 || n < 1)
    {
        LOG(WARNING) << "Invalid curl write callback function params, size is '" << size
        << "', n is '" << n << "'!";
        return n;
    }

    auto * str = reinterpret_cast<std::string *>(s);
    str->append(reinterpret_cast<const char *>(ptr), size * n);
    return n;
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

bool http_req(const std::string & url, const std::string & req_data, long timeout_ms,
              const std::vector<std::string> & custom_headers,
              int & resp_code, std::string & resp_data)
{
    CURL * curl = curl_easy_init();
    if (nullptr == curl)
    {
        LOG(ERROR) << "Failed to curl_easy_init!";
        return false;
    }
    curl_easy_setopt(curl, CURLoption::CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLoption::CURLOPT_TIMEOUT_MS, timeout_ms);
    if (!req_data.empty())
    {
        curl_easy_setopt(curl, CURLoption::CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLoption::CURLOPT_POSTFIELDS, req_data.c_str());
        curl_easy_setopt(curl, CURLoption::CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(req_data.length()));
    }
    curl_easy_setopt(curl, CURLoption::CURLOPT_WRITEFUNCTION, write_string_callback);
    curl_easy_setopt(curl, CURLoption::CURLOPT_WRITEDATA, static_cast<void *>(&resp_data));

    bool ret = false;
    do
    {
        char errbuf[CURL_ERROR_SIZE] = {};
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
#ifndef NDEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
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
