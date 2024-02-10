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
#include <cassert>


namespace BlueBinXml::Convertor{

    /*----------------------------------------------------------------------*/
    std::vector<uint8_t> CConvertor::Convert(TXmlCharacter* input){
        Clear();

        rapidxml::xml_document<TXmlCharacter> document;
        document.parse<0>(input);

        // Write node tree
        TId rootId = GetNextId();
        WriteNode(rootId, document);

        // Write strings
        // TODO writing by iterating unordered table shuffles strings randomly.
        // It might be better to maintain order, so 
        //     1] string close in tree are closer in the string table as well
        //     2] more frequent strings are generally together at the beginning
        for (auto& str : m_Strings){
            WriteString(str.second, str.first);
        }

        // Relocate all references
        RelocateReferences(rootId);

        return m_Result;
    }

    /*----------------------------------------------------------------------*/
    void CConvertor::Clear(){
        m_Result.clear();
        m_Strings.clear();
        m_ObjectOffsets.clear();
        m_IdCounter = 0;
    }
    /*----------------------------------------------------------------------*/
    TOffset CConvertor::GetCurrentOffset() const{
        return static_cast<TOffset> (m_Result.size());
    }
    
    /*----------------------------------------------------------------------*/
    void CConvertor::WriteNode(TId id, TXmlNode& input){
        
        std::vector<SAttributeDescription> attributes;
        std::vector<SNodeDescription> childern;

        // collect attributes
        auto* attributeSrc = input.first_attribute();
        while(attributeSrc != nullptr){       
            attributes.emplace_back(
                attributeSrc->name(),
                attributeSrc->value());

            attributeSrc = attributeSrc->next_attribute();
        }

        // collect sub-nodes
        auto* nodeSrc = input.first_node();
        while(nodeSrc != nullptr){
            rapidxml::node_type type = nodeSrc->type();
            // We only collect Element nodes
            if (type != rapidxml::node_type::node_element){
                nodeSrc->next_sibling();
                continue;
            }

            childern.emplace_back(GetNextId(), nodeSrc);
            nodeSrc->next_sibling();
        }

        // write myself
        AddObjectLocation(id, m_Result.size());
        BlueBinXml::CNode node;
        node.m_Name = AddString(input.name());
        node.m_Content = AddString(input.value());
        node.m_AttributeCount = attributes.size();
        node.m_ChildCount = childern.size();
        Write(node);
        
        // Write attributes
        for(auto & attr : attributes){
            TId nameId = AddString(attr.m_Name);
            TId valueId = AddString(attr.m_Value);
            Write(nameId);
            Write(valueId);
        }

        // Wirite child node references
        for(auto& child : childern){
            Write(child.m_Id);
        }

        // Recursively write subnodes
        for (auto & child : childern){
            assert(child.m_Original != nullptr);
            WriteNode(child.m_Id, *child.m_Original);
        }        
    }

    /*----------------------------------------------------------------------*/
    void CConvertor::WriteString(TId id, const std::string& str){
        AddObjectLocation(id, m_Result.size());
        // TODO write in less stupid and wasteful way
        for(auto ch : str)
        {
            Write(ch);
        }
        // TODO is this needed?
        Write('\0');
    }

    /*----------------------------------------------------------------------*/
    void CConvertor::AddObjectLocation(TId id, TOffset offset){
        assert(m_ObjectOffsets.find(id) == m_ObjectOffsets.end());
        m_ObjectOffsets[id] = offset;
    }

    /*----------------------------------------------------------------------*/
    void CConvertor::RelocateReferences(TId nodeId){
        assert(m_ObjectOffsets.find(nodeId) != m_ObjectOffsets.end());
        TOffset nodeOffset = m_ObjectOffsets[nodeId];

        BlueBinXml::CNode* node = reinterpret_cast<BlueBinXml::CNode*>(m_Result.data() + nodeOffset);

        ReplaceIdWithOffset(nodeOffset, node->m_Name);
        ReplaceIdWithOffset(nodeOffset, node->m_Content);

        // todo relocate rest

    }

    /*----------------------------------------------------------------------*/
    void CConvertor::ReplaceIdWithOffset(TOffset base, TOffset& offset){
        assert(m_ObjectOffsets.find(offset) != m_ObjectOffsets.end());

        TOffset absoluteOffset = m_ObjectOffsets[offset];
        assert(base < absoluteOffset);
        offset = absoluteOffset - base;
    }

    /*----------------------------------------------------------------------*/
    TId CConvertor::AddString(const std::string& str){
        auto it = m_Strings.find(str);

        if (it == m_Strings.end()){
            TId strId = GetNextId();
            m_Strings[str] = strId;
            return strId;
        }

        return it->second;
    }

}

int main(int argc, char const *argv[]){
    
    std::cout << "lulz" << std::endl;
    
    return 0;
}