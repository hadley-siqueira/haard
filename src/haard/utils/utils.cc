// for mkdir
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <map>

#include "haard/utils/utils.h"

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

std::string colorify(std::string msg, bool no_normal) {
    std::string r;
    std::map<std::string, std::string> color_map;

    color_map["normal"] = "\033[0m";
    color_map["black"] = "\033[30m";
    color_map["red"] = "\033[91m";
    color_map["green"] = "\033[92m";
    color_map["orange"] = "\033[93m";
    color_map["blue"] = "\033[94m";
    color_map["magenta"] = "\033[95m";
    color_map["cyan"] = "\033[96m";
    color_map["white"] = "\033[97m";

    for (int i = 0; i < msg.size(); ++i) {
        if (msg[i] == '<') {
            std::string color;

            ++i;
            while (msg[i] != '>') {
                color += msg[i];
                ++i;
            }

            if (color_map.count(color) > 0) {
                r += color_map[color];
            } else {
                r += "<" + color + ">";
            }
        } else {
            r += msg[i];
        }
    }

    if (!no_normal) {
        r += color_map["normal"];
    }

    return r;
}

std::string get_line_from_file(std::string path, int line) {
    std::ifstream f(path);
    char c = '\0';
    int counter = 1;

    while (line != counter) {
        f.get(c);

        if (c == '\n') {
            ++counter;
        }
    }

    c = '\0';
    std::string buffer;

    while (c != '\n') {
        buffer += c;
        f.get(c);
    }

    return buffer;
}

std::string build_message(std::string path, int line, int column, std::string msg) {
    std::string contents = get_line_from_file(path, line);
    std::stringstream msg2;
    std::stringstream line_size;

    line_size << "  " << line << " | ";

    for (int i = 0; i < line_size.str().size() - 3; ++i) {
        msg2 << ' ';
    }

    msg2 << "--> " << path << '\n' << line_size.str() << contents << '\n';

    for (int i = 0; i < column + line_size.str().size() - 1; ++i) {
        msg2 << ' ';
    }

    msg2 << '^' << " " << msg;
    return msg2.str();
}
