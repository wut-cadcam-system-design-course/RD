#pragma once

#include <string>
#include <vector>

void train(const std::string &modelPath, const std::vector<std::string> &objPaths);
void predict(const std::string &modelPath, const std::string &inputOBJ, const std::string &outputOBJ);