/*
*    ________  ________  .____      __________.__
*    \______ \ \______ \ |    |     \______   \  |  __ __   ____
*     |    |  \ |    |  \|    |      |    |  _/  | |  |  \_/ __ ",
*     |    `   \|    `   \    |___   |    |   \  |_|  |  / ___/
*    /_______  /_______  /_______ \  |______  /____/____/  \___  >
*            \/        \/        \/         \/                 \/
*/

#include "Convertor.hpp"

#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>


/*----------------------------------------------------------------------*/
bool Code(const char* inputFile, const char* outputFile){
    std::ifstream input(inputFile);

    if (!input.is_open() || !input.good()){
        std::cerr << "Failed to open input file" << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    input.close();
    BlueBinXml::Convertor::CConvertor convertor;
    auto result = convertor.Convert(buffer.str().data());

    if (result.size() == 0){
        return false;
    }

    std::ofstream output(outputFile, std::ios::out | std::ios::binary);
    if (!output.is_open() || !output.good()){
        std::cerr << "Failed to open create file" << std::endl;
        return false;
    }

    output.write(reinterpret_cast<const char*>(result.data()), result.size());
    output.close();

    return true;
}

/*----------------------------------------------------------------------*/
bool Decode(const char* inputFile, const char* outputFile){
     std::ifstream input(inputFile, std::ios_base::binary);

    input.seekg(0, std::ios_base::end);
    auto length = input.tellg();
    input.seekg(0, std::ios_base::beg);
    std::vector<std::byte> buffer(length);
    input.read(reinterpret_cast<char*>(buffer.data()), length);
    input.close();

    if (buffer.size() == 0){
        std::cerr << "Failed to open input file" << std::endl;
        return false;
    }

    std::unique_ptr<uint8_t> uniqueBuffer;
    uniqueBuffer.reset(new uint8_t[buffer.size()]);
    size_t size = buffer.size();
    std::memcpy(uniqueBuffer.get(), buffer.data(), size);
    buffer.clear();

    BlueBinXml::Convertor::CConvertor convertor;

    std::ofstream output(outputFile, std::ios::out);
    if (!output.is_open() || !output.good()){
        std::cerr << "Failed to open create file" << std::endl;
        return false;
    }

    convertor.Deconvert(uniqueBuffer, size, output);

    return true;
}

/*----------------------------------------------------------------------*/
int main(int argc, char const *argv[]){
    if (argc != 4 || (*(argv[1]) != 'c' && *(argv[1]) != 'd')){
        std::cerr << "Usage:\nConvert to binary: convertor c input.xml output.bbxml\n"
                             "Convert to xml:    convertor d input.bbxml output.xml" << std::endl;
                             return 1;
    }

    bool ok = false;

    if (*(argv[1]) == 'c'){
        ok = Code(argv[2], argv[3]);
    }else{
        ok = Decode(argv[2], argv[3]);
    }

    if (!ok){
        std::cerr << "Action failed" << std::endl;
        return 2;
    }

    return 0;

    // -------------------------------------------------

    std::ifstream t("convertor/test/cd.xml");
    std::stringstream buffer;
    buffer << t.rdbuf();

    BlueBinXml::Convertor::CConvertor convertor;
    std::cout << "Convert" << std::endl;
    auto result = convertor.Convert(buffer.str().data());

    //MemoryDump(result.data(), result.size());

    std::unique_ptr<uint8_t> resultBin;
    resultBin.reset(new uint8_t[result.size()]);
    std::memcpy(resultBin.get(), result.data(), result.size());

    //BlueBinXml::CDocument document(std::move(resultBin), result.size());
    //auto* root = document.GetRootNode();

    // std::cout << "\n\n ----- DUMP -----" << std::endl;
    // if (root == nullptr)
    // {
    //     std::cout << "root is null" << std::endl;
    // }else{
    //     //PrintNode(*root);
    // }

    std::cout << "outputing decoded" << std::endl;
    convertor.Deconvert(resultBin, result.size(), std::cout);

    return 0;
}