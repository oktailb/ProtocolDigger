#pragma once

#include <unordered_map>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

enum ConfigFields {e_offset, e_type, e_size, e_endian, e_mask, e_rshift, e_ratio, e_customStr};

enum DataType {e_int, e_uint, e_float, e_string, e_sint};

static std::unordered_map<std::string,DataType> const stringDataType =
    {
        {"string",  DataType::e_string},
        {"sint",    DataType::e_sint},
        {"float",   DataType::e_float},
        {"int",     DataType::e_int},
        {"uint",    DataType::e_uint}
};

static std::unordered_map<DataType, std::string> const intDataType =
    {
        {DataType::e_sint,   "sint"},
        {DataType::e_string, "string"},
        {DataType::e_float,  "float" },
        {DataType::e_int,    "int"   },
        {DataType::e_uint,   "uint"  }
};

enum DataSize {e_custom = 0, e_8 = 8, e_16 = 16, e_32 = 32, e_64 = 64};

static std::unordered_map<std::string,DataSize> const stringDataSize =
    {
        {"custom", DataSize::e_custom},
        {"64",     DataSize::e_64},
        {"32",     DataSize::e_32},
        {"16",     DataSize::e_16},
        {"8",      DataSize::e_8}
};

static std::unordered_map<DataSize, std::string> const intDataSize =
    {
        {DataSize::e_custom, "custom" },
        {DataSize::e_64,     "64"},
        {DataSize::e_32,     "32"},
        {DataSize::e_16,     "16"},
        {DataSize::e_8,      "8" }
};

enum DataEndian {e_network, e_host};

static std::unordered_map<std::string,DataEndian> const stringDataEndian =
    {
        {"network", DataEndian::e_network},
        {"host",    DataEndian::e_host}
};

static std::unordered_map<DataEndian, std::string> const intDataEndian =
    {
        {DataEndian::e_network, "network"},
        {DataEndian::e_host,    "host"}
};

typedef struct  varDef_s {
    uint32_t    offset;
    DataSize    size;
    uint32_t    len;
    DataType    type;
    DataEndian  endian;
    uint64_t    mask;
    uint8_t     shift;
    double      ratio;
    std::string custom;
} varDef_t;

void extractVariablesFromConfiguration(std::map<std::string, std::string> &configuration, std::map<std::string, varDef_t> &variables);
std::string findByOffset(uint32_t offset, std::map<std::string, varDef_t> base);
std::vector<std::string> split(const std::string& s, char seperator);
