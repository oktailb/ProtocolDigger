#include "variables.h"
#include <algorithm>
#include <charconv>
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
            std::string soffset = tokensVal[ConfigFields::e_offset];
            uint32_t offset = 0;
            std::from_chars(soffset.c_str() + 2, soffset.c_str() + soffset.size(), offset, 16);
            DataType type = stringDataType.at(tokensVal[ConfigFields::e_type]);
            DataSize size = DataSize::e_custom;
            uint32_t len = 0;
            uint64_t mask = 0;
            uint8_t rshift = 0;
            double ratio = 0.0;
            DataEndian endian = DataEndian::e_network;
            std::string custom = "";

            switch (type)
            {
            case DataType::e_sint:
            case DataType::e_string:
            {
                std::string slen = tokensVal[ConfigFields::e_size];
                std::from_chars(slen.c_str(), slen.c_str() + slen.size(), len);
                if (tokensVal.size() > ConfigFields::e_mask)
                    custom = tokensVal[ConfigFields::e_mask];
                break;
            }
            default:
            {
                size  = stringDataSize.at(tokensVal[ConfigFields::e_size]);
                std::string smask = (tokensVal[ConfigFields::e_mask]);
                std::from_chars(smask.c_str() + 2, smask.c_str() + smask.size(), mask, 16);
                std::string srshift = tokensVal[ConfigFields::e_rshift];
                std::from_chars(srshift.c_str(), srshift.c_str() + srshift.size(), rshift);
                std::string sratio = tokensVal[ConfigFields::e_ratio];
                std::from_chars(sratio.c_str(), sratio.c_str() + sratio.size(), ratio);
                endian = stringDataEndian.at(tokensVal[ConfigFields::e_endian]);
                if (tokensVal.size() > ConfigFields::e_customStr)
                    custom = tokensVal[ConfigFields::e_customStr];
            }
            }

            variables[name] = {offset, size, len, type, endian, mask, rshift, ratio, custom};
        }
    }
}
