// for mkdir
#include <cstdlib>
#include <unistd.h>

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

std::string uppercase(std::string s) {
    std::string r;

    for (int i = 0; i < s.size(); ++i) {
        r += uppercase(s[i]);
    }

    return r;
}

char uppercase(char c) {
    if (c >= 'a' && c <= 'z') {
        return 'A' + (c - 'a');
    }

    return c;
}

char lowercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return 'a' + (c - 'A');
    }

    return c;
}

std::string lowercase(std::string s) {
    std::string r;

    for (int i = 0; i < s.size(); ++i) {
        r += lowercase(s[i]);
    }

    return r;
}

std::string join(std::vector<std::string> v, std::string sep) {
    int i;
    std::string r;

    if (v.size() > 0) {
        for (i = 0; i < v.size() - 1; ++i) {
            r += v[i] + sep;
        }

        r += v[i];
    }

    return r;
}

void mkdir(std::string path) {
    std::string cmd = "mkdir " + path;
    system(cmd.c_str());
}

void rmdir(std::string path) {
    std::string cmd = "rm -rf " + path;
    system(cmd.c_str());
}

std::string indent(std::string s, int ws) {
    std::string r;

    for (int i = 0; i < ws; ++i) {
        r += ' ';
    }

    for (int i = 0; i < s.size(); ++i) {
        r += s[i];

        if (s[i] == '\n' && i < s.size() - 1) {
            for (int j = 0; j < ws; ++j) {
                r += ' ';
            }
        }
    }

    return r;
}
