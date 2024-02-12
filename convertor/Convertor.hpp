/*
*    ________  ________  .____      __________.__
*    \______ \ \______ \ |    |     \______   \  |  __ __   ____
*     |    |  \ |    |  \|    |      |    |  _/  | |  |  \_/ __ ",
*     |    `   \|    `   \    |___   |    |   \  |_|  |  / ___/
*    /_______  /_______  /_______ \  |______  /____/____/  \___  >
*            \/        \/        \/         \/                 \/
*/

#pragma once

#include "../BlueBinXml.hpp"

#include "3rdParty/rapidxml/rapidxml.hpp"

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <memory>

namespace BlueBinXml::Convertor{

    // Temporary IDs that are used instead of offsets for the first conversion pass
    // Those two NEEDS to be the same type, because they are being stored in the same variables
    using TId = BlueBinXml::TOffset;

    using TXmlCharacter = char; 

    class CConvertor{
    public:
        std::vector<uint8_t> Convert(TXmlCharacter*);
        bool Deconvert(std::unique_ptr<uint8_t>& input, size_t size, std::ostream & output);  

    private:
        using TXmlNode = rapidxml::xml_node<TXmlCharacter>;
        using TXmlDocument = rapidxml::xml_document<TXmlCharacter>;

        struct SNodeDescription{
            TId m_Id;
            TXmlNode* m_Original;
        };

        struct SAttributeDescription{
            std::string m_Name;
            std::string m_Value;
        };

        template <typename T>
        void Write(const T& data);
        TOffset GetCurrentOffset() const;

        void Clear();

        void WriteNode(TId id, TXmlNode& input);
        void WriteString(TId id, const std::string& str);
        void AddObjectLocation(TId id, TOffset offset);
        void RelocateReferences(TId nodeId);
        void DecodeNode(const BlueBinXml::CNode & input, TXmlNode& output, TXmlDocument& outputDocument);

        void ReplaceIdWithOffset(TOffset base, TOffset& offset);
        TId AddString(const std::string& str);
        TId GetNextId() {return m_IdCounter++;}

        std::vector<uint8_t> m_Result;
        std::unordered_map<std::string, TId> m_Strings;
        std::unordered_map<TId, TOffset> m_ObjectOffsets;
        TId m_IdCounter;
    };


}

#include "Convertor.inl"

 int main(int argc, char const *argv[]);

