#ifndef HAARD_UTILS_H
#define HAAAD_UTILS_H

#include <vector>
#include <string>

char uppercase(char c);
char lowercase(char c);

std::string uppercase(std::string s);
std::string lowercase(std::string s);

std::vector<std::string> split(std::string s, char sep);

std::string join(std::vector<std::string> v, std::string sep);

void mkdir(std::string path);
void rmdir(std::string path);

std::string indent(std::string s, int ws=4);
std::string colorify(std::string msg, bool no_normal=false);


#endif
