#include "pve_api_client.h"

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

bool PveApiClient::init(std::string api_host, std::string api_token)
{
    int resp_code;
    std::string resp_data;
    req("https://pve:8006/api2/json//nodes/xhpve/network/vmbr0", "", resp_code, resp_data);
    LOG(INFO) << resp_data;
    return true;
}

bool PveApiClient::req(const std::string & api_url, const std::string & req_data,
                       int & resp_code, std::string & resp_data)
{
    CURL * curl = curl_easy_init();
    if (nullptr == curl)
    {
        LOG(ERROR) << "Failed to curl_easy_init!";
        return false;
    }
    curl_easy_setopt(curl, CURLoption::CURLOPT_URL, api_url.c_str());
    curl_easy_setopt(curl, CURLoption::CURLOPT_TIMEOUT_MS, _req_timeout);
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
//        Authorization: PVEAPIToken=USER@REALM!TOKENID=UUID
        curl_slist * plist = curl_slist_append(
            nullptr, "Authorization: PVEAPIToken=root@pam!ddns=111"
        );
//        plist = curl_slist_append(plist, "Accept: application/json");
//        plist = curl_slist_append(plist, "Content-Type: application/json");
//        plist = curl_slist_append(plist, "charset: utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
        CURLcode curl_ret = curl_easy_perform(curl);
        if (curl_ret != CURLcode::CURLE_OK)
        {
            LOG(WARNING) << "curl_easy_perform fail, curl code is '" << curl_ret << "', error is '" << errbuf
                << "', url is '" << api_url << "'!";
            break;
        }
        ret = true;

        long code = 0;
        curl_ret = curl_easy_getinfo(curl, CURLINFO::CURLINFO_RESPONSE_CODE, code);
        if (CURLcode::CURLE_OK != curl_ret)
        {
            LOG(WARNING) << "curl_easy_getinfo fail, curl_code is '" << curl_ret << "'!";
            break;
        }
        resp_code = static_cast<int>(code);
        if (resp_code != 200)
            LOG(WARNING) << "'" << api_url << "' request failed, response code is '" << resp_code << "'!";
    } while (false);

    curl_easy_cleanup(curl);

    return ret;
}
