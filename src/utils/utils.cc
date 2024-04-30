#include "utils/utils.h"

std::vector<std::string> split(std::string s, char sep) {
    std::string tmp;
    std::vector<std::string> r;

    for (int i = 0; i < s.size(); ++i) {
        if (s[i] == sep) {
            r.push_back(tmp);
            tmp = "";
        } else {
            tmp += s[i];
        }
    }

    r.push_back(tmp);
    return r;
}
