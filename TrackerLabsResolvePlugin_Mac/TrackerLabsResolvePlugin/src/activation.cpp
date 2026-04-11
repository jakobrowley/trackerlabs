#include "activation.h"
#include <ctime>
#include "auth.h"
std::vector<unsigned char> atob(const std::string& input) {
    if (input.size() % 4 != 0)
        throw std::invalid_argument("Invalid Base64 length");

    size_t padding = 0;
    if (!input.empty() && input.back() == '=') padding++;
    if (input.size() > 1 && input[input.size() - 2] == '=') padding++;

    std::vector<unsigned char> output;
    output.reserve((input.size() / 4) * 3 - padding);

    for (size_t i = 0; i < input.size(); i += 4) {
        int n = (B64_TABLE[(unsigned char)input[i]] << 18) |
            (B64_TABLE[(unsigned char)input[i + 1]] << 12) |
            ((input[i + 2] == '=' ? 0 : B64_TABLE[(unsigned char)input[i + 2]]) << 6) |
            ((input[i + 3] == '=' ? 0 : B64_TABLE[(unsigned char)input[i + 3]]));

        output.push_back((n >> 16) & 0xFF);
        if (input[i + 2] != '=') output.push_back((n >> 8) & 0xFF);
        if (input[i + 3] != '=') output.push_back(n & 0xFF);
    }

    return output;
}

std::map<std::string, std::string> parse_json(const std::string& s) {
    std::map<std::string, std::string> result;
    size_t i = 0;

    while ((i = s.find('"', i)) != std::string::npos) {
        size_t j = s.find('"', i + 1);
        std::string key = s.substr(i + 1, j - i - 1);

        i = s.find(':', j) + 1;
        size_t k = s.find_first_of(",}", i);
        std::string value = s.substr(i, k - i);

        result[key] = value;
        i = k;
    }
    return result;
}
std::map<std::string, std::string> parse_json_license(const std::string& s,char delimiter) {
    std::map<std::string, std::string> result;
    size_t i = 0;

    while ((i = s.find(delimiter, i)) != std::string::npos) {
        size_t j = s.find(delimiter, i + 1);
        std::string key = s.substr(i + 1, j - i - 1);

        i = s.find(':', j) + 1;
        
        if (s.substr(i, 1) != "{")
        {
            size_t k = s.find_first_of(",", i);
            std::string value = s.substr(i, k - i);
            result[key] = value;
            i = k;
        }
    }
    return result;
}
std::time_t parseISO8601(const std::string& s) {
    std::tm tm = {};
    sscanf(s.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
        &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

    tm.tm_year -= 1900; // tm_year is years since 1900
    tm.tm_mon -= 1;     // tm_mon is 0-based

    // Convert to UTC time_t
#ifdef _WIN32
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}
std::string getUserData() {
#ifdef _WIN32
    const char* appData = std::getenv("APPDATA");
    return appData ? ((std::string)appData+"\\EditLab\\plugins\\dt.txt") : "";
#elif __APPLE__
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + "/Library/Application Support/EditLab/plugins/dt.txt" : "";
#else // Linux / Unix
    const char* xdgDataHome = std::getenv("XDG_DATA_HOME");
    const char* home = std::getenv("HOME");
    if (xdgDataHome) return xdgDataHome;
    if (home) return std::string(home) + "/.local/share";
    return "";
#endif
}
std::string getLicensePath()
{
#ifdef _WIN32
    const char* appData = std::getenv("APPDATA");
    return appData ? ((std::string)appData + "\\com.tracker.lic") : "";
#elif __APPLE__
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + "/Library/Application Support/com.tracker.lic" : "";
#else // Linux / Unix
    const char* xdgDataHome = std::getenv("XDG_DATA_HOME");
    const char* home = std::getenv("HOME");
    if (xdgDataHome) return xdgDataHome;
    if (home) return std::string(home) + "/.local/share";
    return "";
#endif
}

// Standard Alphabet for reference
const std::string ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// The Table (Cipher Alphabet) - Must be 26 unique characters
const std::string TABLE = "QWERTYUIOPASDFGHJKLZXCVBNM";

std::string encrypt(std::string text) {
    for (char& c : text) {
        size_t pos = ALPHABET.find(std::toupper(c));
        if (pos != std::string::npos) {
            c = TABLE[pos];
        }
    }
    return text;
}

std::string decrypt(std::string text) {
    for (char& c : text) {
        size_t pos = TABLE.find(std::toupper(c));
        if (pos != std::string::npos) {
            c = ALPHABET[pos];
        }
    }
    return text;
}
int useVerifyOnline(const char* license,bool activate)
{
    char url[1024];

    if(activate) sprintf(url, "https://tinytapes.com/wp-json/lmfwc/v2/licenses/activate/%s", license);
    else sprintf(url, "https://tinytapes.com/wp-json/lmfwc/v2/licenses/validate/%s", license);

    std::string request_result = fetchWithAuthSync(url, "ck_60bbfd050bb532fc54354a7cd5104f09a203b2d0", "cs_8ea328e5927e16aab8472579b122491cf4defcff");
    
    
    std::map<std::string, std::string> obj = parse_json_license(request_result,'"');
    if (obj.find("lmfwc_rest_data_error") != obj.end())
    {
        if (obj["lmfwc_rest_data_error"].find("reached maximum activation count.") != std::string::npos)
        {
            return 509;
        }
        if (obj["lmfwc_rest_data_error"].find("could not be found.") != std::string::npos)
        {
            return 502;
        }
    }
    else if (obj.find("timesActivated") != obj.end() && obj.find("timesActivatedMax") != obj.end())
    {
        int timesActivated = 0;
        int timesActivatedMax = 0;
        sscanf(obj["timesActivated"].c_str(), "%d", &timesActivated);
        sscanf(obj["timesActivatedMax"].c_str(), "%d", &timesActivatedMax);
        if (timesActivated <= timesActivatedMax)
        {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm* timePtr = std::localtime(&now_c);
            int year = timePtr->tm_year + 1900;
            int month = timePtr->tm_mon + 1;
            int day = timePtr->tm_mday;
            int hour = timePtr->tm_hour;
            int min = timePtr->tm_min;
            int sec = timePtr->tm_sec;

            char date[1024];
            sprintf(date, "%d-%d-%d-%d-%d-%d", year, month, day, hour, min, sec);

            char result[1024];
            sprintf(result, "'License':%s,'Date':%s", license, date);
            std::string encrypted_result= encrypt(result);
            std::string license_path = getLicensePath();

            FILE* fp = fopen(license_path.c_str(), "wb");
            if (fp)
            {
                fwrite(encrypted_result.c_str(), encrypted_result.size(),1, fp);
                fclose(fp);
            }
            
            return 0;
        }
        return 509;
    }
    /*
    W3Client client;

    if (client.Connect("https://tinytapes.com", "ck_60bbfd050bb532fc54354a7cd5104f09a203b2d0", "cs_8ea328e5927e16aab8472579b122491cf4defcff"))
    {
        

        if (client.Request(url, W3Client::reqGet))
        {
            char buf[1024] = "\0";
            while (client.Response(reinterpret_cast<unsigned char*>(buf), 1024) > 0)
            {
                
            }

        }
        else
        {
            //Network Error
            return 503;
        }
    }
     */
    return 500;
}
int useVerifyOffline()
{
    std::string license_path = getLicensePath();
    FILE* fp = fopen(license_path.c_str(), "rb");
    if (fp)
    {
        fseek(fp, 0, 2);
        int buf_size = ftell(fp);
        fseek(fp, 0, 0);
        char* buf = new char[buf_size + 1];
        buf[buf_size] = 0;
        fread(buf, buf_size, 1, fp);
        fclose(fp);

        std::string decrypted_result = decrypt(buf);

        free(buf);

        std::map<std::string, std::string> obj = parse_json_license(decrypted_result.c_str(), '\'');       
        return useVerifyOnline(obj["LICENSE"].c_str(), false);
    }
    return 501;
}
