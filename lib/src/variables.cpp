#include "variables.h"
#include <algorithm>
#include <stdint.h>
#include <string>
#include <vector>

std::vector<std::string> split(const std::string& s, char seperator)
{
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;

    while((pos = s.find(seperator, pos)) != std::string::npos)
    {
        std::string substring( s.substr(prev_pos, pos-prev_pos) );
        output.push_back(substring);
        prev_pos = ++pos;
    }
    output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word

    return output;
}

std::string findByOffset(uint32_t offset, std::map<std::string, varDef_t> base)
{
    for(auto candidate : base)
        if (candidate.second.offset == offset)
            return candidate.first;
    return "";
}

void tolower(std::string &data)
{
    std::transform(data.begin(), data.end(), data.begin(),
                   [](unsigned char c){ return std::tolower(c); });
}

void extractVariablesFromConfiguration(std::map<std::string, std::string> &configuration, std::map<std::string, varDef_t> &variables)
{
    for(auto var : configuration)
    {
        std::vector<std::string> tokensVar = split(var.first, '/');
        if (tokensVar[0].compare("Vars") == 0)
        {
            std::string name = tokensVar[1];
            std::vector<std::string> tokensVal = split(var.second, ',');
            tolower(tokensVal[ConfigFields::e_offset]);
            uint32_t offset = std::stoi(tokensVal[ConfigFields::e_offset], 0, 16);
            DataType type = stringDataType.at(tokensVal[ConfigFields::e_type]);
            DataSize size = DataSize::e_custom;
            uint32_t len = 0;
            uint64_t mask = 0;
            uint8_t rshift = 0;
            double ratio = 0.0;
            DataEndian endian = DataEndian::e_network;
            switch (type)
            {
            case DataType::e_sint:
            case DataType::e_string:
            {
                len  = std::stoi(tokensVal[ConfigFields::e_size]);
                break;
            }
            default:
            {
                size  = stringDataSize.at(tokensVal[ConfigFields::e_size]);
                tolower(tokensVal[ConfigFields::e_mask]);
                mask = std::stoul((tokensVal[ConfigFields::e_mask]), 0, 16);
                rshift = std::stoi(tokensVal[ConfigFields::e_rshift]);
                ratio = std::stof(tokensVal[ConfigFields::e_ratio]);
                endian = stringDataEndian.at(tokensVal[ConfigFields::e_endian]);
            }
            }

            variables[name] = {offset, size, len, type, endian, mask, rshift, ratio};
        }
    }
}
