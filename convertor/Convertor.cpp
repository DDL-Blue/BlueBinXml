/*
*    ________  ________  .____      __________.__
*    \______ \ \______ \ |    |     \______   \  |  __ __   ____
*     |    |  \ |    |  \|    |      |    |  _/  | |  |  \_/ __ ",
*     |    `   \|    `   \    |___   |    |   \  |_|  |  / ___/
*    /_______  /_______  /_______ \  |______  /____/____/  \___  >
*            \/        \/        \/         \/                 \/
*/

#include "Convertor.hpp"

#include "3rdParty/rapidxml/rapidxml_print.hpp"

#include <iostream>
#include <cassert>
#include <iomanip>
#include <cstring>
#include <fstream>

/*======================================================================*/

/*----------------------------------------------------------------------*/
void PrintIndention(std::ostream& stream, unsigned int indention){
    static constexpr int levels = 7;
    static const char* spaces[levels] = 
        {" ", 
         "  ",
         "    ",
         "        ",
         "                ",
         "                                ",
         "                                                                "};

    if (indention > 127){
        indention = 127;
    }

    for (unsigned int i = 0; i < levels && indention != 0; i++)
    {
        if (indention & 1){
            stream << spaces[i];
        }
        indention >>= 1;
    }
}

/*----------------------------------------------------------------------*/
void PrintNode(const BlueBinXml::CNode& node, int indent = 0){
    PrintIndention(std::cout, indent);
    std::cout<< "<" << node.GetName();

    for(unsigned int i=0; i< node.GetAttributeCount(); ++i){
        auto attr = node.GetAttribute(i);
        std::cout << " " << attr.m_Name << " = \"" << attr.m_Value << "\"";
    }

    std::cout << ">" << std::endl;
    PrintIndention(std::cout, indent);
    std::cout << "Content: \"" << node.GetContent() << "\"" << std::endl;
    

    for(unsigned int i=0; i< node.GetChildernCound(); ++i){
        auto* subnode = node.GetChild(i);
        PrintNode(*subnode, indent + 1);
    } 

}

/*----------------------------------------------------------------------*/
void MemoryDump(uint8_t* mem, size_t size){
    std::cout << "------------" << std::endl;
    std::cout << " 0  1  2  3  4  5  6  7    8  9  A  B  C  D  E  F" << std::endl;
    for(size_t i =0; i<size; ++i){
        
        if ((i&7) == 0 && (i&15) != 0) {
            std::cout << "  ";
        }
        if ((i&15) == 0 && i!= 0) {
            std::cout << std::endl;
        }

        std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)mem[i] << " ";
    }

    std::cout << std::dec << std::setfill(' ') << std::setw(1) << std::endl;
    std::cout << "------------" << std::endl;
}



/*======================================================================*/
namespace BlueBinXml::Convertor{

    /*----------------------------------------------------------------------*/
    std::vector<uint8_t> CConvertor::Convert(TXmlCharacter* input){
        Clear();

        rapidxml::xml_document<TXmlCharacter> document;

        try{
            document.parse<0>(input);
        }
        catch (rapidxml::parse_error& err){
            std::cerr << "XML parse error" << err.what() << std::endl;
            return std::vector<uint8_t>();
        }

        

        // Write node tree
        TId rootId = GetNextId();
        WriteNode(rootId, document);

        //MemoryDump(m_Result.data(), m_Result.size());


        //std::cout << "Write strings" << std::endl;
        // Write strings
        // TODO writing by iterating unordered table shuffles strings randomly.
        // It might be better to maintain order, so 
        //     1] string close in tree are closer in the string table as well
        //     2] more frequent strings are generally together at the beginning
        // TODO I could also detect if some string is at the end of other and group those as one
        for (auto& str : m_Strings){
            WriteString(str.second, str.first);
        }

        //MemoryDump(m_Result.data(), m_Result.size());

        // std::cout << "String table:" << std::endl;
        // for (auto & str : m_Strings){
        //     std::cout << "  " << str.second << "\t-> \"" << str.first << "\"" << std::endl;
        // }
        // std::cout << std::endl;
        

        //std::cout << "Relocating references" << std::endl;

        // Relocate all references
        RelocateReferences(rootId);

        //MemoryDump(m_Result.data(), m_Result.size());

        std::cout << "Final size: " << m_Result.size() << std::endl;

        return m_Result;
    }

    /*----------------------------------------------------------------------*/
    bool CConvertor::Deconvert(std::unique_ptr<uint8_t>& input, size_t size, std::ostream & output){
        
        BlueBinXml::CDocument inputDoc(std::move(input), size);
        TXmlDocument outputDoc;

        const BlueBinXml::CNode* rootNode = inputDoc.GetRootNode();
        if (rootNode == nullptr){
            return false;
        }

        DecodeNode(*rootNode, outputDoc, outputDoc);
        output << outputDoc;

        return true;
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
        
        //std::cout << "Writing node " << input.name() << std::endl;

        std::vector<SAttributeDescription> attributes;
        std::vector<SNodeDescription> childern;

        // collect attributes
        //std::cout << "  Collecting attributes..." << std::endl;
        auto* attributeSrc = input.first_attribute();
        while(attributeSrc != nullptr){       
            attributes.emplace_back(
                attributeSrc->name(),
                attributeSrc->value());

            attributeSrc = attributeSrc->next_attribute();
        }

        // collect sub-nodes
        //std::cout << "  Collecting subnodes..." << std::endl;
        auto* nodeSrc = input.first_node();
        while(nodeSrc != nullptr){
            rapidxml::node_type type = nodeSrc->type();
            // We only collect Element nodes
            if (type != rapidxml::node_type::node_element){
                nodeSrc = nodeSrc->next_sibling();
                continue;
            }

            childern.emplace_back(GetNextId(), nodeSrc);
            nodeSrc = nodeSrc->next_sibling();
        }

        //std::cout << "  Write myself" << std::endl;

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
            //std::cout << "  Write child id " << child.m_Id << std::endl;
            Write(child.m_Id);
        }

        // Recursively write subnodes
        for (auto & child : childern){
            assert(child.m_Original != nullptr);
            WriteNode(child.m_Id, *child.m_Original);
        }        

        // TODO delete duplicate subtrees. sha256 https://github.com/okdshin/PicoSHA2/tree/master
        // - change offsets to signed values
        // - maintain map subtree hash -> nodeID
        // - add ability to "rollback" result - delete some end of it
        // - when subtree returns known hash, rollback what it wrote and use ID of the original   

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

        //std::cout << "Relocating node " << node->GetName() << " id " << nodeId << std::endl;
        //std::cout << "  ... name" << std::endl;
        ReplaceIdWithOffset(nodeOffset, node->m_Name);
        //std::cout << "  ... content" << std::endl;
        ReplaceIdWithOffset(nodeOffset, node->m_Content);

        for(TCount i=0; i<node->m_AttributeCount; ++i){
            BlueBinXml::CNode::SAttributeDef* def = node->GetObject<BlueBinXml::CNode::SAttributeDef>(node->GetAttributeDefOffset(i));
            //std::cout << "  ...attr " << i << " name" << std::endl;
            ReplaceIdWithOffset(nodeOffset, def->m_Name);
            //std::cout << "  ...attr value" << std::endl;
            ReplaceIdWithOffset(nodeOffset, def->m_Value);
        }

        for(TCount i=0; i<node->m_ChildCount; ++i){
            //std::cout << "  ...child ref " << i << std::endl;
            //std::cout << "      ... with offset" << node->GetChildRefOffset(i) << std::endl;
            TOffset* def = node->GetObject<TOffset>(node->GetChildRefOffset(i));
            TId childId = *def;
            ReplaceIdWithOffset(nodeOffset, *def);
            RelocateReferences(childId);
        }

    }

    /*----------------------------------------------------------------------*/
    void CConvertor::DecodeNode(const BlueBinXml::CNode & input, TXmlNode& output, TXmlDocument& outputDocument){
        
        // Attributes
        for(TCount i = 0; i<input.GetAttributeCount(); ++i){
            auto attr = input.GetAttribute(i);
            auto* outAttr = outputDocument.allocate_attribute(attr.m_Name, attr.m_Value);
            output.append_attribute(outAttr);
        }

        // Childern
        for(TCount i = 0; i<input.GetChildernCound(); ++i){
            auto* child = input.GetChild(i);
            auto* outputChild = outputDocument.allocate_node(rapidxml::node_type::node_element, child->GetName(), child->GetContent());
            DecodeNode(*child, *outputChild, outputDocument);
            output.append_node(outputChild);
        }        
    }

    /*----------------------------------------------------------------------*/
    void CConvertor::ReplaceIdWithOffset(TOffset base, TOffset& offset){
        //std::cout << "   > Relocating id " << offset << " with base " << base << std::endl; 

        assert(m_ObjectOffsets.find(offset) != m_ObjectOffsets.end());
        assert(offset < m_Result.size());

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

/*======================================================================*/

