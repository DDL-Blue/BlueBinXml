/*
*    ________  ________  .____      __________.__
*    \______ \ \______ \ |    |     \______   \  |  __ __   ____
*     |    |  \ |    |  \|    |      |    |  _/  | |  |  \_/ __ ",
*     |    `   \|    `   \    |___   |    |   \  |_|  |  / ___/
*    /_______  /_______  /_______ \  |______  /____/____/  \___  >
*            \/        \/        \/         \/                 \/
*/

#pragma once

#include "Convertor.hpp"

namespace BlueBinXml::Convertor{

template<typename T>
void CConvertor::Write(const T& data){
    uint8_t* rawData = reinterpret_cast<uint8_t>(&data);
    for(int i=0; i<sizeof(T); ++i){
        m_Result.push_back(rawData[i]);
    }
}


}